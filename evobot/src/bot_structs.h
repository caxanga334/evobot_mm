//
// EvoBot - Neoptolemus' Natural Selection bot, based on Botman's HPB bot template
//
// bot_structs.h
// 
// Contains all helper functions for making tactical decisions
//

#pragma once

#ifndef BOT_STRUCTS_H
#define BOT_STRUCTS_H

#include "bot_math.h"
#include "NS_Constants.h"

constexpr auto BOT_NAME_LEN = 32; // Max length of a bot's name
constexpr auto MAX_PATH_SIZE = 512; // Maximum number of points allowed in a path (this should be enough for any sized map)

constexpr auto MAX_ACTION_PRIORITIES = 12; // How many levels of priority the commander can assign actions to
constexpr auto MAX_PRIORITY_ACTIONS = 20; // How many actions at each priority level the commander can have at one time



// Bot's role on the team.
enum BotRole
{
	BOT_ROLE_NONE,

	BOT_ROLE_COMMAND,		 // Will attempt to take command
	BOT_ROLE_FIND_RESOURCES, // Will hunt for uncapped resource nodes and cap them. Will attack enemy resource towers
	BOT_ROLE_RES_CAPPER,	 // Will hunt for uncapped nodes or ones held by the enemy and cap them
	BOT_ROLE_GUARD_BASE,	 // Will stay behind in the base and guard/build stuff
	BOT_ROLE_ATTACK_HIVE,	 // Not used
	BOT_ROLE_BUILDER,		 // Will focus on building chambers and hives. Stays gorge most of the time
	BOT_ROLE_HARASS,		 // Focuses on taking down enemy resource nodes and structures
	BOT_ROLE_LERK,			 // Not in use yet, will go lerk
	BOT_ROLE_FADE,			 // Acts like a harasser, but will go Fade once it has enough resources
	BOT_ROLE_ONOS			 // Acts like a harasser, but will go Onos once it has enough resources
};

// Affects the bot's pathfinding choices
enum BotMoveStyle
{
	MOVESTYLE_NORMAL, // Most direct route to target
	MOVESTYLE_AMBUSH, // Prefer wall climbing and vents
	MOVESTYLE_HIDE // Prefer crouched areas like vents
};

// Defines bot behaviour. Handled through console command "evobot debug testnav/drone/stop"
typedef enum _EVODEBUGMODE
{
	EVO_DEBUG_NONE = 0, // Bot plays game normally
	EVO_DEBUG_TESTNAV, // Bot randomly navigates between resource nodes and comm chair, will not enagage enemies
	EVO_DEBUG_DRONE // Bot does nothing unless given task by player, will not engage enemies unless given attack order
} EvobotDebugMode;

// Type of goal the commander wants to achieve
typedef enum _COMMANDERACTIONTYPE
{
	ACTION_NONE = 0,
	ACTION_BUILD,
	ACTION_UPGRADE,
	ACTION_RESEARCH,
	ACTION_RECYCLE,
	ACTION_DROPITEM,
	ACTION_GIVEORDER

} CommanderActionType;

// Some commander actions are multi-step (e.g. click to select building, release to complete selection, input recycle command etc). Tracks where the commander is in the process
typedef enum _COMMANDERACTIONSTEP
{
	ACTION_STEP_NONE = 0,
	ACTION_STEP_BEGIN_SELECT, // Click mouse button down to start select
	ACTION_STEP_END_SELECT, // Release mouse button to complete select

} CommanderActionStep;

// The list of potential task types for the bot_task structure
typedef enum
{
	TASK_NONE,
	TASK_GET_HEALTH,
	TASK_GET_AMMO,
	TASK_GET_WEAPON,
	TASK_GET_EQUIPMENT,
	TASK_BUILD,
	TASK_ATTACK,
	TASK_MOVE,
	TASK_CAP_RESNODE,
	TASK_DEFEND,
	TASK_GUARD,
	TASK_HEAL,
	TASK_WELD,
	TASK_RESUPPLY,
	TASK_EVOLVE,
	TASK_GRENADE,
}
BotTaskType;

typedef struct
{
	char szClassname[64];
	int  iClipSize;
	int  iAmmo1;     // ammo index for primary ammo
	int  iAmmo1Max;  // max primary ammo
	int  iAmmo2;     // ammo index for secondary ammo
	int  iAmmo2Max;  // max secondary ammo
	int  iSlot;      // HUD slot (0 based)
	int  iPosition;  // slot position
	int  iId;        // weapon ID
	int  iFlags;     // flags???
	float MinRefireTime;
} bot_weapon_t;

typedef struct _BOT_CURRENT_WEAPON_T
{
	int  iId = 0;     // weapon ID
	int  iClip = 0;   // amount of ammo in the clip
	int  iClipMax = 0; // Max clip size
	int  iAmmo1 = 0;  // amount of ammo in primary reserve
	int	iAmmo1Max = 0; // Max ammo in primary reserve
	int  iAmmo2 = 0;  // amount of ammo in secondary reserve (not used in NS)
	int  iAmmo2Max = 0; // Max ammo in secondary reserve (not used in NS)
	float MinRefireTime = 0.0f; // For non-automatic weapons, how frequently (in seconds) should the bot fire. 0 for automatic weapons.
	float LastFireTime = 0.0f; // When bot last pressed the fire button. Only used if MinRefireTime > 0.
} bot_current_weapon_t;

// Bot path node. A path will be several of these strung together to lead the bot to its destination
typedef struct _BOT_PATH_NODE
{
	Vector Location = ZERO_VECTOR; // Location to move to
	float requiredZ = 0.0f; // If climbing a up ladder or wall, how high should they aim to get before dismounting.
	unsigned char flag = 0; // Is this a ladder movement, wall climb, walk etc
	unsigned char area = 0; // Is this a crouch area, normal walking area etc
	unsigned int poly = 0; // The nav mesh poly this point resides on
} bot_path_node;

// Contains the bot's current navigation info, such as current path
typedef struct _NAV_STATUS
{
	bot_path_node CurrentPath[MAX_PATH_SIZE]; // Bot's path nodes

	Vector TargetDestination = ZERO_VECTOR; // Desired destination
	Vector ActualMoveDestination = ZERO_VECTOR; // Actual destination on nav mesh

	Vector LastNavMeshPosition = ZERO_VECTOR; // Tracks the last place the bot was on the nav mesh. Useful if accidentally straying off it

	Vector LastPathFollowPosition = ZERO_VECTOR; // Tracks the last point where the bot was happily following a path

	int PathSize = 0; // How many path nodes the bot's current path has
	int CurrentPathPoint = 0; // Which point in the path the bot is on
	int CurrentMoveType = MOVETYPE_NONE; // Tracks the edict's current movement type

	unsigned int CurrentPoly = 0; // Which nav mesh poly the bot is currently on

	float LastStuckCheckTime = 0.0f; // Last time the bot checked if it had successfully moved
	float TotalStuckTime = 0.0f; // Total time the bot has spent stuck
	float LastDistanceFromDestination = 0.0f; // How far from its destination was it last stuck check

	Vector StuckCheckMoveLocation = ZERO_VECTOR; // Where is the bot trying to go that we're checking if they're stuck?
	Vector UnstuckMoveLocation = ZERO_VECTOR; // If the bot is unable to find a path, blindly move here to try and fix the problem
	Vector UnstuckMoveStartLocation = ZERO_VECTOR; // So the bot can track where it was when it started the unstuck movement
	float UnstuckMoveLocationStartTime = 0.0f; // When did the bot start trying to move to UnstuckMoveLocation? Give up after certain amount of time

	float LandedTime = 0.0f; // When the bot last landed after a fall/jump.
	float LeapAttemptedTime = 0.0f; // When the bot last attempted to leap/blink. Avoid spam that sends it flying around too fast
	bool bIsJumping = false; // Is the bot in the air from a jump? Will duck so it can duck-jump
	bool IsOnGround = true; // Is the bot currently on the ground, or on a ladder?
	bool bHasAttemptedJump = false; // Last frame, the bot tried a jump. If the bot is still on the ground, it probably tried to jump in a vent or something

	BotMoveStyle MoveStyle = MOVESTYLE_NORMAL; // Current desired move style (e.g. normal, ambush, hide). Will trigger new path calculations if this changes
	float LastPathCalcTime = 0.0f; // When the bot last calculated a path, to limit how frequently it can recalculate
	int LastMoveProfile = -1; // The last navigation profile used by the bot. Will trigger new path calculations if this changes (e.g. changed class, changed move style)

} nav_status;

// Represents a bot's current understanding of an enemy player's status
typedef struct _ENEMY_STATUS
{
	edict_t* EnemyEdict = nullptr; // Reference to the enemy player edict
	Vector LastSeenLocation = ZERO_VECTOR; // The last visibly-confirmed location of the player
	Vector LastSeenVelocity = ZERO_VECTOR; // Last visibly-confirmed movement direction of the player
	Vector TrackedLocation = ZERO_VECTOR; // If tracked by parasite or motion-tracking, last pinged location of player
	float LastSeenTime = 0.0f; // Last time the bot saw the player (not tracked)
	float LastTrackedTime = 0.0f; // Last time the player was pinged to the bot if parasited/motion-tracked
	bool bCurrentlyVisible = false; // Is the player directly visible to the bot
	bool bIsValidTarget = false; // Should the bot care about this enemy player? Will be false if too far away or not seen for long enough
	bool bIsTracked = false; // Is this enemy currently parasited or motion-tracked?
} enemy_status;

// Pending message a bot wants to say. Allows for a delay in sending a message to simulate typing, or prevent too many messages on the same frame
typedef struct _BOT_MSG
{
	char msg[64]; // Message to send
	float SendTime = 0.0f; // When the bot should send this message
	bool bIsPending = false; // Represents a valid pending message
	bool bIsTeamSay = false; // Is this a team-only message?
} bot_msg;

// Used by the AI commander instead of bot_task. Has data specifically to handle commander-specific stuff
typedef struct _COMMANDER_ACTION
{
	bool bIsActive = false;
	CommanderActionType ActionType = ACTION_NONE; // What action to perform (e.g. build, recycle, drop item etc)
	CommanderActionStep ActionStep = ACTION_STEP_NONE; // Used for multi-stage processes such as selecting a building, issuing recycle command etc.
	NSStructureType StructureToBuild = STRUCTURE_NONE; // What structure to build if build action
	Vector BuildLocation = ZERO_VECTOR; // Where to build the structure
	Vector DesiredCommanderLocation = ZERO_VECTOR; // To perform this action, where does the commander's view need to be? For building, usually directly above location, but could be off to side if obstructed by geometry
	Vector LastAttemptedCommanderLocation = ZERO_VECTOR; // The position of the commander's view at the last action attempt
	Vector LastAttemptedCommanderAngle = ZERO_VECTOR; // The click angle of the last action attempt
	int AssignedPlayer = -1; // Which player index is assigned to perform the action (e.g. build structure)? Will send orders to that player (move here, build this structure etc.)
	edict_t* StructureOrItem = nullptr; // Reference the structure edict. If a structure has been successfully placed but not yet fully built, it will be referenced here
	edict_t* ActionTarget = nullptr; // Mostly used for dropping health packs and ammo for players where the drop location might be moving around
	bool bHasAttemptedAction = false; // Has the commander tried placing a structure or item at the build location? If so, and it didn't appear, will try to adjust view around until it works
	float StructureBuildAttemptTime = 0.0f; // When the commander tried placing a structure. Commander will wait a short while to confirm if the building appeared or if it should try again
	int NumActionAttempts = 0; // Commander will give up after a certain number of attempts to place structure/item
	NSResearch ResearchId = RESEARCH_NONE; // What research to perform if research action
	NSDeployableItem ItemToDeploy = ITEM_NONE; // What item to drop if drop item action

} commander_action;

// A bot task is a goal the bot wants to perform, such as attacking a structure, placing a structure etc. NOT USED BY COMMANDER
typedef struct _BOT_TASK
{
	BotTaskType TaskType = TASK_NONE; // Task Type (e.g. build, attack, defend, heal etc)
	Vector TaskLocation = ZERO_VECTOR; // Task location, if task needs one (e.g. where to place structure for TASK_BUILD)
	edict_t* TaskTarget = NULL; // Reference to a target, if task needs one (e.g. TASK_ATTACK)
	NSStructureType StructureType = STRUCTURE_NONE; // For Gorges, what structure to build (if TASK_BUILD)
	float TaskStartedTime = 0.0f; // When the bot started this task. Helps time-out if the bot gets stuck trying to complete it
	bool bIssuedByCommander = false; // Was this task issued by the commander? Top priority if so
	bool bTargetIsPlayer = false; // Is the TaskTarget a player?
	bool bOrderIsUrgent = false; // Determines whether this task is prioritised over others if bot has multiple
	bool bIsWaitingForBuildLink = false; // If true, Gorge has sent the build impulse and is waiting to see if the building materialised
	float LastBuildAttemptTime = 0.0f; // When did the Gorge last try to place a structure?
	int BuildAttempts = 0; // How many attempts the Gorge has tried to place it, so it doesn't keep trying forever
	int Evolution = 0; // Used by TASK_EVOLVE to determine what to evolve into

} bot_task;


// Tracks what orders have been given to which players
typedef struct _COMMANDER_ORDER
{
	bool bIsActive = false;
	AvHOrderType OrderType = ORDERTYPE_UNDEFINED;
	Vector MoveLocation = ZERO_VECTOR;
	edict_t* Target = nullptr;
	float LastReminderTime = 0.0f;
	float LastPlayerDistance = 0.0f;

} commander_order;



// Bot data structure. Holds all things a growing bot needs
typedef struct _BOT_T
{
	bool is_used = false;

	float map_max_extent = 0.0f; // The commander's Z height when in top-down view. Set by the SetupMap network message

	int respawn_state = 0;
	edict_t* pEdict = NULL;
	char name[BOT_NAME_LEN + 1];
	int not_started = 0;

	float LastUseTime = 0.0f; // When the bot hit the use key last. Used if bContinuous is false in BotUseObject

	// things from pev in CBasePlayer...
	int bot_team = 0;

	NSPlayerClass bot_ns_class = CLASS_NONE;

	float ForwardMove = 0.0f;
	float SideMove = 0.0f;
	float UpMove = 0.0f;

	bot_current_weapon_t current_weapon;  // one current weapon for the bot
	int m_clipAmmo[MAX_AMMO_SLOTS]; // Track ammo amounts in the clip (if weapon uses clips)
	int m_rgAmmo[MAX_AMMO_SLOTS];  // total ammo amounts (1 array for each bot)

	nav_status BotNavInfo; // Bot's movement information, their current path, where in the path they are etc.

	frustum_plane_t viewFrustum[6]; // Bot's view frustum. Essentially, their "screen" for determining visibility of stuff

	edict_t* visiblePlayers[32]; // List of all potentially visible players that bot might track. Right now, is only interested in enemies

	float f_previous_command_time = 0.0f;

	bot_msg ChatMessages[5]; // Bot can have up to 5 chat messages pending

	Vector desiredMovementDir = ZERO_VECTOR; // Bot's desired movement direction. Used by BotMovementInputs()

	FILE* logFile = nullptr;

	Vector CurrentFloorPosition = ZERO_VECTOR; // Traces down to the floor from origin
	Vector CollisionHullTopLocation = ZERO_VECTOR; // Top of their head. Takes crouching into consideration
	Vector CollisionHullBottomLocation = ZERO_VECTOR; // Very bottom of their collision box

	Vector CurrentLadderNormal = ZERO_VECTOR; // If the bot is on a ladder, what direction is the ladder facing

	BotRole CurrentRole = BOT_ROLE_NONE; // Bot's role. Builder, harasser, commander etc.

	int resources = 0; // For marines, this will be the team's resources

	byte impulse = 0;

	float next_commander_action_time = 0.0f; // Throttle how quickly the commander can perform actions to make it a little more realistic

	edict_t* CommanderCurrentlySelectedBuilding = nullptr; // Has the commander clicked on a building and selected it?

	edict_t* ActionLinkedItems[256]; // Track which items in the world already have been linked to an action. This needs cleaning up

	int NumActionLinkedItems = 0;

	commander_action CurrentCommanderActions[MAX_ACTION_PRIORITIES][MAX_PRIORITY_ACTIONS]; // All the current actions a commander can queue. Their "to-do" list
	commander_order LastPlayerOrders[32]; // All the orders the commander has issued to players

	bot_task* CurrentTask = nullptr; // Bot's current task they're performing
	bot_task PrimaryBotTask;
	bot_task SecondaryBotTask;
	bot_task WantsAndNeedsTask;
	bot_task PendingTask; // Used to decide if something should override the existing primary/secondary tasks

	Vector DesiredLookDirection = ZERO_VECTOR; // What view angle is the bot currently turning towards
	Vector InterpolatedLookDirection = ZERO_VECTOR; // Used to smoothly interpolate the bot's view rather than snap instantly like an aimbot
	edict_t* LookTarget = nullptr; // Used to work out what view angle is needed to look at the desired entity
	Vector LookTargetLocation = ZERO_VECTOR; // This is the bot's current desired look target. Could be an enemy (see LookTarget), or point of interest
	Vector MoveLookLocation = ZERO_VECTOR; // If the bot has to look somewhere specific for movement (e.g. up for a ladder or wall-climb), this will override LookTargetLocation so the bot doesn't get distracted and mess the move up
	float LastTargetTrackUpdate = 0.0f; // Add a delay to how frequently a bot can track a target's movements
	float ViewInterpolationSpeed = 0.0f; // How fast should the bot turn its view? Depends on distance to turn
	float ViewInterpStartedTime = 0.0f; // Used for interpolation

	Vector CurrentEyePosition = ZERO_VECTOR; // Represents the world view location for the bot

	
	float Adrenaline = 0.0f; // For alien abilities

	// All of the below is for guarding an area. This needs reworking to be honest.

	Vector CurrentGuardLocation = ZERO_VECTOR;
	Vector GuardPoints[8];
	Vector GuardLookLocation = ZERO_VECTOR;
	int NumGuardPoints = 0;
	float GuardStartLookTime = 0.0f;
	float ThisGuardLookTime = 0.0f;
	// How long to spend guarding the current area
	float GuardLengthTime = 0.0f;
	float GuardStartedTime = 0.0f;

	float AimingDelay = 0.1f; // How frequently should the bot adjust its view and react to moving targets?

	float LastJumpTime = 0.0f; // Used to prevent the bot bunny-hopping and killing its momentum as they're not pros and can't bunny hop properly.

	float LastCommanderRequestTime = 0.0f; // The last time this bot requested something from the commander. Prevents spam

	float CommanderLastScanTime = 0.0f; // When the commander last dropped a scan, so it doesn't spam them

	NSWeapon DesiredCombatWeapon = WEAPON_NONE; // Bot's desired weapon at any given time. Will switch to it at the end of BotThink
	NSWeapon DesiredMoveWeapon = WEAPON_NONE; // If the bot needs a particular weapon for movement (e.g. blink) then this overrides DesiredCombatWeapon so it doesn't interrupt the movement

	enemy_status TrackedEnemies[32];
	int CurrentEnemy = -1;
	edict_t* CurrentEnemyRef = nullptr;

	char PathStatus[128]; // Debug used to help figure out what's going on with a bot's path finding
	char MoveStatus[128]; // Debug used to help figure out what's going on with a bot's steering

	float LastCombatTime = 0.0f; // Last time the bot was fighting or hunting an enemy

	bool bBotThinkPaused = false;

	float ViewUpdateRate = 0.2f; // How frequently the bot can react to new sightings of enemies etc.
	float LastViewUpdateTime = 0.0f; // Used to throttle view updates based on ViewUpdateRate

	bool bIsPendingKill = false; // Has the bot issued a "kill" command and it waiting for oblivion?

} bot_t;




#endif