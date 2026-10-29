//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_mobber.cpp
// Engineer robot squads: ignore the objective and hunt down enemy players

#include "cbase.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "func_respawnroom.h"
#include "bot/tf_bot.h"
#include "bot/behavior/tf_bot_mobber.h"
#include "bot/behavior/tf_bot_seek_and_destroy.h"

#include "nav_mesh.h"


// how far behind their victim mobbers that MoveBehindEnemy stand
#define MOBBER_BEHIND_MIN_RANGE		50.0f
#define MOBBER_BEHIND_MAX_RANGE		300.0f

// mobbers only start circling around once they're this close
#define MOBBER_FLANK_START_RANGE	750.0f


//---------------------------------------------------------------------------------------------
CTFBotMobber::CTFBotMobber( void ) : m_chasePath( ChasePath::LEAD_SUBJECT )
{
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotMobber::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	m_victim = NULL;
	m_retargetTimer.Invalidate();

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotMobber::OnResume( CTFBot *me, Action< CTFBot > *interruptingAction )
{
	// things have changed while we were away - look again
	m_victim = NULL;
	m_retargetTimer.Invalidate();
	ForgetPath();

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CTFBotMobber::ForgetPath( void )
{
	m_path.Invalidate();
	m_chasePath.Invalidate();
	m_repathTimer.Invalidate();
}


//---------------------------------------------------------------------------------------------
bool CTFBotMobber::IsValidVictim( CTFBot *me, CTFPlayer *victim ) const
{
	if ( !victim || !victim->IsAlive() || victim->GetTeamNumber() != GetEnemyTeam( me->GetTeamNumber() ) )
		return false;

	// spies we haven't caught out are left to the rest of the team
	if ( ( victim->m_Shared.IsStealthed() || victim->m_Shared.InCond( TF_COND_DISGUISED ) ) && !me->IsKnownSpy( victim ) )
		return false;

	// no camping their spawn
	if ( PointInRespawnRoom( victim, victim->WorldSpaceCenter() ) )
		return false;

	return true;
}


//---------------------------------------------------------------------------------------------
// Whoever is hurting us, then whoever we're fighting, then the closest enemy
CTFPlayer *CTFBotMobber::SelectVictim( CTFBot *me ) const
{
	CTFPlayer *pRevengeTarget = me->GetRobotRevengeTarget();
	if ( IsValidVictim( me, pRevengeTarget ) )
		return pRevengeTarget;

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat && threat->IsVisibleRecently() )
	{
		CTFPlayer *pThreat = ToTFPlayer( threat->GetEntity() );
		if ( IsValidVictim( me, pThreat ) )
			return pThreat;
	}

	CUtlVector< CTFPlayer * > enemyVector;
	CollectPlayers( &enemyVector, GetEnemyTeam( me->GetTeamNumber() ), COLLECT_ONLY_LIVING_PLAYERS );

	CTFPlayer *pClosest = NULL;
	float flClosestRangeSq = FLT_MAX;

	FOR_EACH_VEC( enemyVector, i )
	{
		CTFPlayer *pEnemy = enemyVector[i];
		if ( !IsValidVictim( me, pEnemy ) )
			continue;

		float flRangeSq = me->GetRangeSquaredTo( pEnemy );
		if ( flRangeSq < flClosestRangeSq )
		{
			flClosestRangeSq = flRangeSq;
			pClosest = pEnemy;
		}
	}

	return pClosest;
}


//---------------------------------------------------------------------------------------------
// head aiming and weapon firing is handled elsewhere - we just pick who to go after and get to them
ActionResult< CTFBot >	CTFBotMobber::Update( CTFBot *me, float interval )
{
	CTFPlayer *victim = m_victim;

	if ( !IsValidVictim( me, victim ) || m_retargetTimer.IsElapsed() )
	{
		CTFPlayer *pNewVictim = SelectVictim( me );
		if ( pNewVictim != victim )
		{
			ForgetPath();
		}

		m_victim = victim = pNewVictim;
		m_retargetTimer.Start( RandomFloat( 4.0f, 6.0f ) );
	}
	else
	{
		// someone closer in our face takes priority over whoever we were after
		const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
		if ( threat && threat->IsVisibleRecently() && threat->GetEntity() != victim )
		{
			CTFPlayer *pThreat = ToTFPlayer( threat->GetEntity() );
			if ( IsValidVictim( me, pThreat ) && me->GetRangeSquaredTo( pThreat ) < me->GetRangeSquaredTo( victim ) )
			{
				m_victim = victim = pThreat;
				ForgetPath();
			}
		}
	}

	if ( !victim )
	{
		return SuspendFor( new CTFBotSeekAndDestroy( 5.0f ), "Nobody to hunt - roaming" );
	}

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	me->EquipBestWeaponForThreat( threat );

	const CKnownEntity *known = me->GetVisionInterface()->GetKnown( victim );
	bool isVisible = known && known->IsVisibleRecently();

	CTFWeaponBase *myWeapon = me->m_Shared.GetActiveTFWeapon();
	bool isUsingCloseRangeWeapon = ( myWeapon && ( myWeapon->IsWeapon( TF_WEAPON_FLAMETHROWER ) || myWeapon->IsMeleeWeapon() ) );

	float attackRange = me->GetDesiredAttackRange();

	if ( me->GetRobotAI().m_moveBehindEnemy > 0 && isVisible && me->IsRangeLessThan( victim, MOBBER_FLANK_START_RANGE ) )
	{
		// circle around to their back
		Vector forward;
		victim->EyeVectors( &forward );
		forward.z = 0.0f;
		forward.NormalizeInPlace();

		float behindRange = clamp( attackRange, MOBBER_BEHIND_MIN_RANGE, MOBBER_BEHIND_MAX_RANGE );
		Vector behindSpot = victim->GetAbsOrigin() - behindRange * forward;

		if ( m_repathTimer.IsElapsed() )
		{
			m_repathTimer.Start( RandomFloat( 0.3f, 0.5f ) );

			CTFBotPathCost cost( me, FASTEST_ROUTE );
			if ( !m_path.Compute( me, behindSpot, cost ) )
			{
				m_path.Invalidate();
			}
		}

		if ( m_path.IsValid() )
		{
			m_path.Update( me );
			return Continue();
		}

		// can't get behind them - just go at them
	}

	if ( isUsingCloseRangeWeapon && isVisible && me->IsRangeLessThan( victim, 1.1f * attackRange ) )
	{
		// circle around our victim
		if ( me->TransientlyConsistentRandomValue( 3.0f ) < 0.5f )
		{
			me->PressLeftButton();
		}
		else
		{
			me->PressRightButton();
		}
	}

	if ( !isVisible || me->IsRangeGreaterThan( victim, attackRange ) || !me->IsLineOfFireClear( victim->EyePosition() ) )
	{
		CTFBotPathCost cost( me, isUsingCloseRangeWeapon ? SAFEST_ROUTE : DEFAULT_ROUTE );
		m_chasePath.Update( me, victim, cost );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotMobber::OnStuck( CTFBot *me )
{
	ForgetPath();
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotMobber::OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason )
{
	ForgetPath();
	return TryContinue();
}
