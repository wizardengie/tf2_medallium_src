//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_mobber.h
// Engineer robot squads: ignore the objective and hunt down enemy players

#ifndef TF_BOT_MOBBER_H
#define TF_BOT_MOBBER_H

#include "Path/NextBotChasePath.h"


//-------------------------------------------------------------------------------
class CTFBotMobber : public Action< CTFBot >
{
public:
	CTFBotMobber( void );
	virtual ~CTFBotMobber() { }

	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );
	virtual ActionResult< CTFBot >	OnResume( CTFBot *me, Action< CTFBot > *interruptingAction );

	virtual EventDesiredResult< CTFBot > OnStuck( CTFBot *me );
	virtual EventDesiredResult< CTFBot > OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason );

	virtual const char *GetName( void ) const	{ return "Mobber"; };

private:
	bool IsValidVictim( CTFBot *me, CTFPlayer *victim ) const;
	CTFPlayer *SelectVictim( CTFBot *me ) const;
	void ForgetPath( void );

	CHandle< CTFPlayer > m_victim;
	CountdownTimer m_retargetTimer;		// we look around for a better victim now and then
	CountdownTimer m_repathTimer;
	PathFollower m_path;				// for getting behind our victim
	ChasePath m_chasePath;
};


#endif // TF_BOT_MOBBER_H
