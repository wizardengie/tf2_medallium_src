# Source SDK 2013

Source code for Source SDK 2013.

Contains the game code for Half-Life 2, HL2: DM and TF2.

**Now including Team Fortress 2! ✨**

## Build instructions

Clone the repository using the following command:

`git clone https://github.com/ValveSoftware/source-sdk-2013`

### Windows

Requirements:
 - Source SDK 2013 Multiplayer installed via Steam
 - Visual Studio 2022 with the following workload and components:
   - Desktop development with C++:
     - MSVC v143 - VS 2022 C++ x64/x86 build tools (Latest)
     - Windows 11 SDK (10.0.22621.0) or Windows 10 SDK (10.0.19041.1)
 - Python 3.13 or later

Inside the cloned directory, navigate to `src`, run:
```bat
createallprojects.bat
```
This will generate the Visual Studio project `everything.sln` which will be used to build your mod.

Then, on the menu bar, go to `Build > Build Solution`, and wait for everything to build.

You can then select the `Client (Mod Name)` project you wish to run, right click and select `Set as Startup Project` and hit the big green `> Local Windows Debugger` button on the tool bar in order to launch your mod.

The default launch options should be already filled in for the `Release` configuration.

### Linux

Requirements:
 - Source SDK 2013 Multiplayer installed via Steam
 - podman

Inside the cloned directory, navigate to `src`, run:
```bash
./buildallprojects
```

This will build all the projects related to the SDK and your mods automatically against the Steam Runtime.

You can then, in the root of the cloned directory, you can navigate to `game` and run your mod by launching the build launcher for your mod project, eg:
```bash
./mod_tf
```

*Mods that are distributed on Steam MUST be built against the Steam Runtime, which the above steps will automatically do for you.*

## Distributing your Mod

There is guidance on distributing your mod both on and off Steam available at the following link:

https://partner.steamgames.com/doc/sdk/uploading/distributing_source_engine

## Engineer Robot Squads (tf2_medallium)

Outside of Mann vs. Machine, an Engineer's **teleporter exit** can double as a robot factory. Once it is built, the exit sends a squad of TFBot robots (the MvM robot models) through, one at a time, to fight for the Engineer's team. When a robot dies, its place is refilled after a delay. Squads are defined in a popfile-style script, so you can write your own.

### Turning it on

Robot squads are off by default. Set this **before the map loads** (so robot models and sounds get precached), e.g. in `server.cfg` or on the command line:

```
tf_robot_squad_enabled 1
```

What they need:

- **A nav mesh** for the map, as for any TFBot.
- **Free player slots.** Robots are real players. The squad size is capped by `tf_robot_squad_max_per_team` (default 10, shared by every Engineer on the team) and by free slots minus `tf_robot_squad_reserve_slots` (default 2). Big squads want a big server, e.g. `-maxplayers 64`.
- **The squad file** `scripts/robot_squads.txt` in the game folder (plus optional per-map files, below). The `game/` folder is not part of this source repository.
- **Robot assets** in the mod's `custom/` folder (we ship them as `custom/robot_assets/`): `models/bots/` (per-class robot models, the `*_boss` giant models, `skeleton_sniper`, `bot_worker`, `saxtron`, etc.), `materials/models/bots/` and `sound/`. Squads can name any other model with `Model`; those files must be installed on the server and clients too.

Summoned robots do not count toward team balance, so they never keep humans off a team.

### Playing an Engineer

1. **Pick a squad.** With the build PDA out, right-click to open the Robot Factory menu, or use `tf_robot_squad_select` (opens the menu) or `tf_robot_squad_select <Name>`, e.g. `tf_robot_squad_select pyro_mob`. Until you pick, you get `default`.
2. **Build a teleporter exit.** About 5 seconds later (`tf_robot_squad_first_spawn_delay`) it starts sending robots through. Upgrading the exit sends robots faster (level 2 takes 20% less time per robot, level 3 takes 40% less) and brings in members whose `MinLevel` is higher. Picking up and redeploying the exit keeps its squad.
3. **Give orders.** Use the Engineer voice commands, or `tf_robot_squad_order <attack|defend|follow|cycle>`:

   | Order | Voice command | What the squad does |
   |---|---|---|
   | `attack` (default) | Go Go Go! (`Z` `3`) | Play the objective like a normal bot |
   | `defend` | Teleporter Here! (`X` `4`) | Guard your teleporter exit |
   | `follow` | Move Up! (`Z` `4`) | Escort you; fall back to the objective while you are dead |

4. **Watch the HUD.** The teleporter's panel in the building status HUD shows `Squad alive/size`, a `+Ns` countdown to the next robot arriving, and the current order.

Robots belong to their Engineer: if the Engineer leaves or changes team, their robots are removed. Summoned Engineers don't get squads of their own.

### How the robots behave

- Every time a member comes through the exit it picks one of its loadouts, favoring those that counter what the enemy team is currently fielding (classes, Sentries, Giants).
- Medics, Engineers, rifle Snipers and Spies always do their own jobs. The other classes obey orders.
- Robots with `Action Mobber` skip the objective and hunt the nearest enemy (or whoever is shooting at them), optionally circling to their back. They only do this under the `attack` order; `defend` and `follow` still guard and escort.
- Every summoned robot turns on whoever is hurting it, and still retreats when badly hurt no matter how many kills it has.
- Robots stuck on map geometry are sent back to their exit after `tf_robot_squad_stuck_time` seconds.
- During Halloween / Full Moon the squads show up as undead instead of robots.

### Server settings

| Command / cvar | Default | Meaning |
|---|---|---|
| `tf_robot_squad_enabled` | 0 | Master switch. Set before the map loads |
| `tf_robot_squad_file` | `scripts/robot_squads.txt` | Squad file to load |
| `tf_robot_squad_spawn_interval` | 2 | Seconds to send each robot through (when the squad doesn't say) |
| `tf_robot_squad_giant_spawn_time` | 8 | Same, for giants |
| `tf_robot_squad_respawn_time` | 10 | Seconds before a dead robot's place is refilled (when the squad doesn't say) |
| `tf_robot_squad_first_spawn_delay` | 5 | Seconds after an exit is built before robots arrive |
| `tf_robot_squad_max_per_team` | 10 | Max summoned robots per team |
| `tf_robot_squad_reserve_slots` | 2 | Player slots kept free for humans |
| `tf_robot_squad_spawn_uber` | 2 | Seconds of spawn invulnerability |
| `tf_robot_squad_stuck_time` | 5 | Seconds stuck before being returned to the exit (0 = off) |
| `tf_robot_squad_reload` | | Reload the squad files (cheat; server admin) |
| `tf_robot_squad_select [name]`, `tf_robot_squad_order <order>` | | Player commands, above |

### Writing squads

`scripts/robot_squads.txt` has `Templates` (reusable TFBot blocks such as `T_GiantPyro` and `T_Demoknight`, and per-class `*Kit` templates that hold a list of loadouts) and `Squad` blocks, and it is commented throughout. The squads it ships with are `default` (Robot Infantry), `pyro_mob`, `scout_rush`, `demoknights`, `heavy_brigade`, `soldier_battalion`, `giant_escort`, `gibby_gang`, `goliatron` and `specialists`.

```
"Squad"
{
	"Name"			"pyro_mob"
	"DisplayName"	"Pyro Mob"
	"RespawnDelay"	"8"
	"TFBot"
	{
		"Template"	"T_PyroKit"
		"Skill"		"Normal"
		"Count"		"3"
		"Action"	"Mobber"
	}
	"TFBot"
	{
		"Template"	"T_GiantPyro"
		"MinLevel"	"3"
	}
}
```

Per-map files `scripts/robot_squads/<mapname>.txt` use the same format. Their templates and squads are added to the global ones, and a squad with the same `Name` replaces the global one on that map. Put `"ReplaceGlobalSquads" "1"` at the top to use only the map's squads. Reload with `tf_robot_squad_reload` or change maps. Models added to the files only show up after a map change.

**Squad keys**

| Key | Meaning |
|---|---|
| `Name` | Id used by `tf_robot_squad_select` (`default` is the starting squad) |
| `DisplayName` | Name in the menu (keep under ~30 characters) |
| `SpawnInterval` | Seconds the exit takes per robot, else `tf_robot_squad_spawn_interval` |
| `RespawnDelay` | Seconds before a dead robot is replaced, else `tf_robot_squad_respawn_time` |
| `TFBot { }` | One squad member |

**Member keys** (`TFBot`): the usual MvM popfile keys (`Class`, `Name`, `Health`, `Scale`, `Skill`, `WeaponRestrictions`, `Attributes`, `Item`, `ItemAttributes`, `CharacterAttributes`, `Tag`, `MaxVisionRange`, `Template`), plus:

| Key | Meaning |
|---|---|
| `Count` | How many of this member (default 1) |
| `MinLevel` / `MaxLevel` | Teleporter levels it appears at (default 1 to 3) |
| `SpawnTime` | Seconds the exit takes to send this member through |
| `Model` / `UseCustomModel` | Custom robot model, e.g. `models/bots/soldier/bot_soldier_gray.mdl` |
| `UndeadModel` | Model for Halloween / Full Moon |
| `Loadout { }` | A loadout this member can carry. List several |

**Loadout keys**: `Name` (shown with `developer 1` when picked), `Counters` (class names, `Sentry`, `Giant`; repeat a name to count it more), `Orders` (`attack`, `defend`, `follow`), `Weight` (default 1), `Model`, and `Item`, `ItemAttributes`, `CharacterAttributes`, `Attributes`, which are added on top of the member's own.

**AI keys** (in `TFBot` or `Loadout`; a loadout's override its member's). The names match the popfile extensions used on some community MvM servers:

| Key | Meaning |
|---|---|
| `Action` | `Mobber`: hunt enemies instead of playing the objective (attack order only). Default plays the objective |
| `MoveBehindEnemy` | `1`: mobbers circle around to their victim's back |
| `DesiredAttackRange` | How close to get before fighting (not for melee weapons) |
| `MeleeWhenClose` | Pull out the melee weapon when an enemy is this close, e.g. `250` |
| `AimAt` | `Head`, `Body` or `Feet`, for bullets and melee (projectiles aim on their own) |
| `AimOffset` | `"x y z"` added to where it aims |
| `AimTrackingInterval` | Seconds between aim updates, lower is sharper. Default from `Skill`: Expert 0.033, Hard 0.05, Normal 0.08, Easy 0.15 |

Popfile templates can be pasted in: MvM-only fields like `ClassIcon` are skipped, and fields from server plugins are skipped with a warning in the console.

## Additional Resources

- [Valve Developer Wiki](https://developer.valvesoftware.com/wiki/Source_SDK_2013)

## License

The SDK is licensed to users on a non-commercial basis under the [SOURCE 1 SDK LICENSE](LICENSE), which is contained in the [LICENSE](LICENSE) file in the root of the repository.

For more information, see [Distributing your Mod](#markdown-header-distributing-your-mod).
