//
// EvoBot - Neoptolemus' Natural Selection bot, based on Botman's HPB bot template
//
// bot_marine.cpp
// 
// Contains marine-specific logic.
//

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>

#include "bot_marine.h"
#include "bot_tactical.h"
#include "bot_navigation.h"
#include "bot_config.h"

extern edict_t* clients[32];

void MarineThink(bot_t* pBot)
{
	edict_t* pEdict = pBot->pEdict;

	if (pBot->CurrentEnemy > -1 && pBot->CurrentRole != BOT_ROLE_COMMAND)
	{
		if (MarineCombatThink(pBot))
		{
			if (pBot->DesiredCombatWeapon == WEAPON_NONE)
			{
				pBot->DesiredCombatWeapon = BotMarineChooseBestWeapon(pBot, pBot->TrackedEnemies[pBot->CurrentEnemy].EnemyEdict);
			}

			return;
		}
	}

	UpdateAndClearTasks(pBot);

	if (pBot->PrimaryBotTask.TaskType == TASK_NONE || (!pBot->PrimaryBotTask.bOrderIsUrgent && !pBot->PrimaryBotTask.bIssuedByCommander))
	{
		BotRole RequiredRole = MarineGetBestBotRole(pBot);

		if (pBot->CurrentRole != RequiredRole)
		{
			UTIL_ClearBotTask(pBot, &pBot->PrimaryBotTask);
			UTIL_ClearBotTask(pBot, &pBot->SecondaryBotTask);

			pBot->CurrentRole = RequiredRole;
			pBot->CurrentTask = &pBot->PrimaryBotTask;
		}

		BotMarineSetPrimaryTask(pBot, &pBot->PrimaryBotTask);

	}

	if (pBot->SecondaryBotTask.TaskType == TASK_NONE || !pBot->SecondaryBotTask.bOrderIsUrgent)
	{
		MarineSetSecondaryTask(pBot, &pBot->SecondaryBotTask);
		//BotMarineSetSecondaryTask(pBot, &pBot->SecondaryBotTask);
	}

	MarineCheckWantsAndNeeds(pBot);

	pBot->CurrentTask = BotGetNextTask(pBot);

	if (pBot->CurrentTask && pBot->CurrentTask->TaskType != TASK_NONE)
	{
		BotProgressTask(pBot, pBot->CurrentTask);
	}

	if (pBot->DesiredCombatWeapon == WEAPON_NONE)
	{
		pBot->DesiredCombatWeapon = BotMarineChooseBestWeapon(pBot, nullptr);
	}

}

void MarineSweeperSetPrimaryTask(bot_t* pBot, bot_task* Task)
{
	if (Task->TaskType == TASK_GUARD) { return; }

	if (UTIL_GetNumBuiltStructuresOfType(STRUCTURE_MARINE_PHASEGATE) < 2)
	{
		Task->TaskType = TASK_GUARD;
		Task->TaskLocation = UTIL_GetRandomPointOnNavmeshInRadius(MARINE_REGULAR_NAV_PROFILE, UTIL_GetCommChairLocation(), UTIL_MetresToGoldSrcUnits(10.0f));
		Task->bOrderIsUrgent = false;
		Task->TaskLength = frandrange(20.0f, 30.0f);
		return;
	}

	edict_t* CurrentPhase = UTIL_GetNearestStructureOfTypeInLocation(STRUCTURE_MARINE_PHASEGATE, pBot->pEdict->v.origin, UTIL_MetresToGoldSrcUnits(10.0f), true, false);
	edict_t* RandPhase = UTIL_GetRandomStructureOfType(STRUCTURE_MARINE_PHASEGATE, CurrentPhase, true);

	if (!FNullEnt(RandPhase))
	{
		Task->TaskType = TASK_GUARD;
		Task->TaskLocation = UTIL_GetRandomPointOnNavmeshInRadius(MARINE_REGULAR_NAV_PROFILE, RandPhase->v.origin, UTIL_MetresToGoldSrcUnits(5.0f));
		Task->bOrderIsUrgent = false;
		Task->TaskLength = frandrange(20.0f, 30.0f);
		return;
	}
		
}

void MarineCapperSetPrimaryTask(bot_t* pBot, bot_task* Task)
{
	if (Task->TaskType == TASK_CAP_RESNODE) { return; }

	const resource_node* UnclaimedResourceNode = UTIL_MarineFindUnclaimedResNodeNearestLocation(pBot, pBot->pEdict->v.origin, UTIL_MetresToGoldSrcUnits(5.0f));

	if (UnclaimedResourceNode)
	{
		UTIL_ClearBotTask(pBot, Task);
		Task->TaskType = TASK_CAP_RESNODE;
		Task->TaskTarget = nullptr;
		Task->TaskLocation = UnclaimedResourceNode->origin;
		Task->bOrderIsUrgent = false;
		Task->TaskLength = 0.0f;
		return;
	}

	edict_t* EnemyResTower = UTIL_GetNearestStructureOfTypeInLocation(STRUCTURE_ALIEN_RESTOWER, pBot->pEdict->v.origin, UTIL_MetresToGoldSrcUnits(100.0f), true, true);

	if (!FNullEnt(EnemyResTower))
	{
		UTIL_ClearBotTask(pBot, Task);
		Task->TaskType = TASK_CAP_RESNODE;
		Task->TaskTarget = EnemyResTower;
		Task->TaskLocation = EnemyResTower->v.origin;
		Task->bOrderIsUrgent = false;
		Task->TaskLength = 0.0f;
		return;
	}
}

void MarineAssaultSetPrimaryTask(bot_t* pBot, bot_task* Task)
{
	const hive_definition* SiegedHive = UTIL_GetNearestHiveUnderSiege(pBot->pEdict->v.origin);

	if (SiegedHive)
	{
		edict_t* Phasegate = UTIL_GetNearestUnbuiltStructureOfTypeInLocation(STRUCTURE_MARINE_PHASEGATE, SiegedHive->Location, UTIL_MetresToGoldSrcUnits(30.0f));

		if (!FNullEnt(Phasegate))
		{
			float Dist = vDist2D(pBot->pEdict->v.origin, Phasegate->v.origin) - 1.0f;

			int NumMarinesNearby = UTIL_GetNumPlayersOfTeamInArea(Phasegate->v.origin, Dist, MARINE_TEAM, pBot->pEdict, CLASS_NONE, false);

			if (NumMarinesNearby < 1)
			{
				Task->TaskType = TASK_BUILD;
				Task->TaskTarget = Phasegate;
				Task->TaskLocation = Phasegate->v.origin;
				Task->bOrderIsUrgent = true;
				return;
			}
		}

		edict_t* TurretFactory = UTIL_GetNearestUnbuiltStructureOfTypeInLocation(STRUCTURE_MARINE_TURRETFACTORY, SiegedHive->Location, UTIL_MetresToGoldSrcUnits(30.0f));

		if (!FNullEnt(TurretFactory))
		{
			float Dist = vDist2D(pBot->pEdict->v.origin, TurretFactory->v.origin) - 1.0f;

			int NumMarinesNearby = UTIL_GetNumPlayersOfTeamInArea(TurretFactory->v.origin, Dist, MARINE_TEAM, pBot->pEdict, CLASS_NONE, false);

			if (NumMarinesNearby < 1)
			{
				Task->TaskType = TASK_BUILD;
				Task->TaskTarget = TurretFactory;
				Task->TaskLocation = TurretFactory->v.origin;
				Task->bOrderIsUrgent = true;
				return;
			}
		}

		edict_t* SiegeTurret = UTIL_GetNearestUnbuiltStructureOfTypeInLocation(STRUCTURE_MARINE_SIEGETURRET, SiegedHive->Location, UTIL_MetresToGoldSrcUnits(30.0f));

		if (!FNullEnt(SiegeTurret))
		{
			float Dist = vDist2D(pBot->pEdict->v.origin, SiegeTurret->v.origin) - 1.0f;

			int NumMarinesNearby = UTIL_GetNumPlayersOfTeamInArea(SiegeTurret->v.origin, Dist, MARINE_TEAM, pBot->pEdict, CLASS_NONE, false);

			if (NumMarinesNearby < 1)
			{
				Task->TaskType = TASK_BUILD;
				Task->TaskTarget = SiegeTurret;
				Task->TaskLocation = SiegeTurret->v.origin;
				Task->bOrderIsUrgent = true;
				return;
			}
		}

		edict_t* Observatory = UTIL_GetNearestUnbuiltStructureOfTypeInLocation(STRUCTURE_MARINE_OBSERVATORY, SiegedHive->Location, UTIL_MetresToGoldSrcUnits(30.0f));

		if (!FNullEnt(Observatory))
		{
			float Dist = vDist2D(pBot->pEdict->v.origin, Observatory->v.origin) - 1.0f;

			int NumMarinesNearby = UTIL_GetNumPlayersOfTeamInArea(Observatory->v.origin, Dist, MARINE_TEAM, pBot->pEdict, CLASS_NONE, false);

			if (NumMarinesNearby < 1)
			{
				Task->TaskType = TASK_BUILD;
				Task->TaskTarget = Observatory;
				Task->TaskLocation = Observatory->v.origin;
				Task->bOrderIsUrgent = true;
				return;
			}
		}

		edict_t* Armoury = UTIL_GetNearestUnbuiltStructureOfTypeInLocation(STRUCTURE_MARINE_ARMOURY, SiegedHive->Location, UTIL_MetresToGoldSrcUnits(30.0f));

		if (!FNullEnt(Armoury))
		{
			float Dist = vDist2D(pBot->pEdict->v.origin, Armoury->v.origin) - 1.0f;

			int NumMarinesNearby = UTIL_GetNumPlayersOfTeamInArea(Armoury->v.origin, Dist, MARINE_TEAM, pBot->pEdict, CLASS_NONE, false);

			if (NumMarinesNearby < 1)
			{
				Task->TaskType = TASK_BUILD;
				Task->TaskTarget = Armoury;
				Task->TaskLocation = Armoury->v.origin;
				Task->bOrderIsUrgent = true;
				return;
			}
		}

		Task->TaskType = TASK_ATTACK;
		Task->TaskTarget = SiegedHive->edict;
		Task->TaskLocation = SiegedHive->FloorLocation;
		Task->bOrderIsUrgent = true;
		return;
	}

	const hive_definition* BuiltHive = UTIL_GetNearestBuiltHiveToLocation(pBot->pEdict->v.origin);

	if (BuiltHive)
	{
		if (UTIL_ResearchIsComplete(RESEARCH_OBSERVATORY_PHASETECH))
		{
			// We're already waiting patiently to start a siege
			if (Task->TaskType == TASK_GUARD && vDist2DSq(Task->TaskLocation, BuiltHive->FloorLocation) <= sqrf(UTIL_MetresToGoldSrcUnits(25.0f))) { return; }

			Vector WaitPoint = UTIL_GetRandomPointOnNavmeshInDonut(MARINE_REGULAR_NAV_PROFILE, BuiltHive->FloorLocation, UTIL_MetresToGoldSrcUnits(15.0f), UTIL_MetresToGoldSrcUnits(20.0f));

			if (WaitPoint != ZERO_VECTOR && !UTIL_QuickTrace(pBot->pEdict, WaitPoint + Vector(0.0f, 0.0f, 10.0f), BuiltHive->Location))
			{
				Task->TaskType = TASK_GUARD;
				Task->TaskLocation = WaitPoint;
				Task->bOrderIsUrgent = false;
				Task->TaskLength = 30.0f;
				return;
			}
		}

		// Bot has a proper weapon equipped
		if (!BotHasWeapon(pBot, WEAPON_MARINE_MG))
		{
			Task->TaskType = TASK_ATTACK;
			Task->TaskTarget = BuiltHive->edict;
			Task->TaskLocation = BuiltHive->FloorLocation;
			Task->bOrderIsUrgent = false;
			return;
		}
	}

	edict_t* ResourceTower = UTIL_GetNearestStructureOfTypeInLocation(STRUCTURE_ALIEN_RESTOWER, pBot->pEdict->v.origin, UTIL_MetresToGoldSrcUnits(100.0f), true, true);

	if (!FNullEnt(ResourceTower))
	{
		Task->TaskType = TASK_ATTACK;
		Task->TaskTarget = ResourceTower;
		Task->TaskLocation = ResourceTower->v.origin;
		Task->bOrderIsUrgent = false;
		return;
	}

	if (Task->TaskType != TASK_MOVE)
	{
		// Randomly patrol the map I guess...
		Vector NewMoveLocation = UTIL_GetRandomPointOfInterest();

		if (NewMoveLocation != ZERO_VECTOR)
		{
			Task->TaskType = TASK_MOVE;
			Task->TaskLocation = NewMoveLocation;
			Task->bOrderIsUrgent = false;
			return;
		}
	}
}

void MarineSetSecondaryTask(bot_t* pBot, bot_task* Task)
{
	if (Task->TaskType == TASK_BUILD) { return; }

	edict_t* UnbuiltStructure = UTIL_FindClosestMarineStructureUnbuiltWithoutBuilders(pBot, 2, pBot->pEdict->v.origin, UTIL_MetresToGoldSrcUnits(30.0f), true);

	if (!FNullEnt(UnbuiltStructure))
	{
		Task->TaskType = TASK_BUILD;
		Task->TaskTarget = UnbuiltStructure;
		Task->TaskLocation = UnbuiltStructure->v.origin;
		Task->bOrderIsUrgent = false;
		return;
	}

	if (Task->TaskType == TASK_DEFEND) { return; }

	edict_t* AttackedStructure = UTIL_GetNearestUndefendedStructureOfTypeUnderAttack(pBot, STRUCTURE_ANY_MARINE_STRUCTURE);

	if (!FNullEnt(AttackedStructure))
	{
		NSStructureType AttackedStructureType = UTIL_GetStructureTypeFromEdict(AttackedStructure);

		// Critical structure if it's in base, or it's a turret factory or phase gate
		bool bCriticalStructure = (vDist2DSq(AttackedStructure->v.origin, UTIL_GetCommChairLocation()) <= sqrf(UTIL_MetresToGoldSrcUnits(15.0f))) || (UTIL_StructureTypesMatch(AttackedStructureType, STRUCTURE_MARINE_ANYTURRETFACTORY) || UTIL_StructureTypesMatch(AttackedStructureType, STRUCTURE_MARINE_PHASEGATE));

		// Always defend if it's critical structure regardless of distance
		if (bCriticalStructure || UTIL_GetPhaseDistanceBetweenPointsSq(pBot->pEdict->v.origin, AttackedStructure->v.origin) <= sqrf(UTIL_MetresToGoldSrcUnits(30.0f)))
		{
			Task->TaskType = TASK_DEFEND;
			Task->TaskTarget = AttackedStructure;
			Task->TaskLocation = AttackedStructure->v.origin;
			Task->bOrderIsUrgent = true;
			return;
		}
	}

	edict_t* EnemyOffenceChamber = BotGetNearestDangerTurret(pBot, UTIL_MetresToGoldSrcUnits(15.0f));

	if (!FNullEnt(EnemyOffenceChamber))
	{
		Task->TaskType = TASK_ATTACK;
		Task->TaskTarget = EnemyOffenceChamber;
		Task->TaskLocation = EnemyOffenceChamber->v.origin;
		Task->bOrderIsUrgent = false;
		return;
	}

	if (Task->TaskType == TASK_WELD) { return; }

	if (BotHasWeapon(pBot, WEAPON_MARINE_WELDER))
	{
		edict_t* HurtPlayer = UTIL_GetClosestPlayerNeedsHealing(pBot->pEdict->v.origin, pBot->pEdict->v.team, UTIL_MetresToGoldSrcUnits(10.0f), pBot->pEdict, true);

		if (!FNullEnt(HurtPlayer) && HurtPlayer->v.armorvalue < GetPlayerMaxArmour(HurtPlayer))
		{
			UTIL_ClearBotTask(pBot, Task);
			Task->TaskType = TASK_WELD;
			Task->TaskTarget = HurtPlayer;
			Task->TaskLocation = HurtPlayer->v.origin;
			Task->bOrderIsUrgent = false;
			return;
		}

		edict_t* DamagedStructure = UTIL_FindClosestDamagedStructure(pBot->pEdict->v.origin, MARINE_TEAM, UTIL_MetresToGoldSrcUnits(30.0f), true);

		if (!FNullEnt(DamagedStructure))
		{
			UTIL_ClearBotTask(pBot, Task);
			Task->TaskType = TASK_WELD;
			Task->TaskTarget = DamagedStructure;
			Task->TaskLocation = DamagedStructure->v.origin;
			Task->bOrderIsUrgent = false;
			return;
		}
	}
}

void MarineSweeperSetSecondaryTask(bot_t* pBot, bot_task* Task)
{

	if (Task->TaskType == TASK_DEFEND) { return; }

	edict_t* AttackedStructure = UTIL_GetNearestUndefendedStructureOfTypeUnderAttack(pBot, STRUCTURE_ANY_MARINE_STRUCTURE);

	if (!FNullEnt(AttackedStructure))
	{
		if (UTIL_GetPhaseDistanceBetweenPointsSq(pBot->pEdict->v.origin, AttackedStructure->v.origin) <= sqrf(UTIL_MetresToGoldSrcUnits(30.0f)))
		{
			Task->TaskType = TASK_DEFEND;
			Task->TaskTarget = AttackedStructure;
			Task->TaskLocation = AttackedStructure->v.origin;
			Task->bOrderIsUrgent = true;
			return;
		}
	}

	if (Task->TaskType == TASK_BUILD) { return; }

	edict_t* UnbuiltStructure = UTIL_FindClosestMarineStructureUnbuilt(pBot->pEdict->v.origin, UTIL_MetresToGoldSrcUnits(30.0f), true);

	if (!FNullEnt(UnbuiltStructure))
	{
		UTIL_ClearBotTask(pBot, Task);
		Task->TaskType = TASK_BUILD;
		Task->TaskTarget = UnbuiltStructure;
		Task->TaskLocation = UnbuiltStructure->v.origin;
		Task->bOrderIsUrgent = false;
		return;
	}

	edict_t* EnemyOffenceChamber = BotGetNearestDangerTurret(pBot, UTIL_MetresToGoldSrcUnits(15.0f));

	if (!FNullEnt(EnemyOffenceChamber))
	{
		Task->TaskType = TASK_ATTACK;
		Task->TaskTarget = EnemyOffenceChamber;
		Task->TaskLocation = EnemyOffenceChamber->v.origin;
		Task->bOrderIsUrgent = false;
		return;
	}

	if (Task->TaskType == TASK_WELD) { return; }

	if (BotHasWeapon(pBot, WEAPON_MARINE_WELDER))
	{
		edict_t* HurtPlayer = UTIL_GetClosestPlayerNeedsHealing(pBot->pEdict->v.origin, pBot->pEdict->v.team, UTIL_MetresToGoldSrcUnits(10.0f), pBot->pEdict, true);

		if (!FNullEnt(HurtPlayer) && HurtPlayer->v.armorvalue < GetPlayerMaxArmour(HurtPlayer))
		{
			UTIL_ClearBotTask(pBot, Task);
			Task->TaskType = TASK_WELD;
			Task->TaskTarget = HurtPlayer;
			Task->TaskLocation = HurtPlayer->v.origin;
			Task->bOrderIsUrgent = false;
			return;
		}

		edict_t* DamagedStructure = UTIL_FindClosestDamagedStructure(pBot->pEdict->v.origin, MARINE_TEAM, UTIL_MetresToGoldSrcUnits(30.0f), true);

		if (!FNullEnt(DamagedStructure))
		{
			UTIL_ClearBotTask(pBot, Task);
			Task->TaskType = TASK_WELD;
			Task->TaskTarget = DamagedStructure;
			Task->TaskLocation = DamagedStructure->v.origin;
			Task->bOrderIsUrgent = false;
			return;
		}
	}

}

void MarineCapperSetSecondaryTask(bot_t* pBot, bot_task* Task)
{
	edict_t* EnemyOffenceChamber = BotGetNearestDangerTurret(pBot, UTIL_MetresToGoldSrcUnits(15.0f));

	if (!FNullEnt(EnemyOffenceChamber))
	{
		Task->TaskType = TASK_ATTACK;
		Task->TaskTarget = EnemyOffenceChamber;
		Task->TaskLocation = EnemyOffenceChamber->v.origin;
		Task->bOrderIsUrgent = false;
		return;
	}

	edict_t* UnbuiltStructure = UTIL_FindClosestMarineStructureUnbuilt(pBot->pEdict->v.origin, UTIL_MetresToGoldSrcUnits(10.0f), false);

	if (!FNullEnt(UnbuiltStructure))
	{
		float Dist = vDist2D(pBot->pEdict->v.origin, UnbuiltStructure->v.origin) - 1.0f;

		int NumBuilders = UTIL_GetNumPlayersOfTeamInArea(UnbuiltStructure->v.origin, Dist, MARINE_TEAM, pBot->pEdict, CLASS_NONE, false);

		if (NumBuilders < 1)
		{
			Task->TaskType = TASK_BUILD;
			Task->TaskTarget = UnbuiltStructure;
			Task->TaskLocation = UnbuiltStructure->v.origin;
			Task->bOrderIsUrgent = UTIL_GetStructureTypeFromEdict(UnbuiltStructure) == STRUCTURE_MARINE_RESTOWER;
			return;
		}

		if (NumBuilders < 2)
		{
			if (Task->TaskType != TASK_GUARD)
			{
				Task->TaskType = TASK_GUARD;
				Task->TaskTarget = UnbuiltStructure;
				Task->TaskLocation = UnbuiltStructure->v.origin;
				Task->TaskLength = 10.0f;
				return;
			}
		}
	}

	edict_t* ResourceTower = UTIL_GetNearestUndefendedStructureOfTypeUnderAttack(pBot, STRUCTURE_MARINE_RESTOWER);

	if (!FNullEnt(ResourceTower))
	{
		Task->TaskType = TASK_DEFEND;
		Task->TaskTarget = ResourceTower;
		Task->TaskLocation = UTIL_GetEntityGroundLocation(ResourceTower);
		Task->bOrderIsUrgent = true;

		return;
	}

	if (BotHasWeapon(pBot, WEAPON_MARINE_WELDER))
	{
		edict_t* DamagedStructure = UTIL_FindClosestDamagedStructure(pBot->pEdict->v.origin, MARINE_TEAM, UTIL_MetresToGoldSrcUnits(20.0f), false);

		if (!FNullEnt(DamagedStructure))
		{
			UTIL_ClearBotTask(pBot, Task);
			Task->TaskType = TASK_WELD;
			Task->TaskTarget = DamagedStructure;
			Task->TaskLocation = DamagedStructure->v.origin;
			Task->bOrderIsUrgent = false;
			return;
		}
	}
}

void MarineAssaultSetSecondaryTask(bot_t* pBot, bot_task* Task)
{
	edict_t* EnemyOffenceChamber = BotGetNearestDangerTurret(pBot, UTIL_MetresToGoldSrcUnits(15.0f));

	if (!FNullEnt(EnemyOffenceChamber))
	{
		Task->TaskType = TASK_ATTACK;
		Task->TaskTarget = EnemyOffenceChamber;
		Task->TaskLocation = EnemyOffenceChamber->v.origin;
		Task->bOrderIsUrgent = false;
		return;
	}

	edict_t* UnbuiltStructure = UTIL_FindClosestMarineStructureUnbuilt(pBot->pEdict->v.origin, UTIL_MetresToGoldSrcUnits(10.0f), false);

	if (!FNullEnt(UnbuiltStructure))
	{
		float Dist = vDist2D(pBot->pEdict->v.origin, UnbuiltStructure->v.origin) - 1.0f;

		int NumBuilders = UTIL_GetNumPlayersOfTeamInArea(UnbuiltStructure->v.origin, Dist, MARINE_TEAM, pBot->pEdict, CLASS_NONE, false);

		if (NumBuilders < 1)
		{
			Task->TaskType = TASK_BUILD;
			Task->TaskTarget = UnbuiltStructure;
			Task->bOrderIsUrgent = true;
			return;
		}
	}

	edict_t* AttackedPhaseGate = UTIL_GetNearestUndefendedStructureOfTypeUnderAttack(pBot, STRUCTURE_MARINE_PHASEGATE);

	if (!FNullEnt(AttackedPhaseGate))
	{
		float Dist = UTIL_GetPhaseDistanceBetweenPointsSq(pBot->pEdict->v.origin, AttackedPhaseGate->v.origin);

		if (Dist < sqrf(UTIL_MetresToGoldSrcUnits(30.0f)))
		{
			Task->TaskType = TASK_DEFEND;
			Task->TaskTarget = AttackedPhaseGate;
			Task->bOrderIsUrgent = true;
			return;
		}
	}

	edict_t* AttackedTurretFactory = UTIL_GetNearestUndefendedStructureOfTypeUnderAttack(pBot, STRUCTURE_MARINE_ANYTURRETFACTORY);

	if (!FNullEnt(AttackedTurretFactory))
	{
		float Dist = UTIL_GetPhaseDistanceBetweenPoints(pBot->pEdict->v.origin, AttackedTurretFactory->v.origin);

		if (Dist < UTIL_MetresToGoldSrcUnits(20.0f))
		{
			Task->TaskType = TASK_DEFEND;
			Task->TaskTarget = AttackedTurretFactory;
			Task->bOrderIsUrgent = true;
			return;
		}
	}
		
	edict_t* ResourceTower = nullptr;
	
	if (UTIL_GetNearestHiveUnderSiege(pBot->pEdict->v.origin) != nullptr)
	{
		ResourceTower = UTIL_GetNearestUndefendedStructureOfTypeUnderAttack(pBot, STRUCTURE_MARINE_RESTOWER);
	}

	edict_t* WeldTargetStructure = nullptr;
	edict_t* WeldTargetPlayer = nullptr;

	if (BotHasWeapon(pBot, WEAPON_MARINE_WELDER))
	{
		edict_t* HurtPlayer = UTIL_GetClosestPlayerNeedsHealing(pBot->pEdict->v.origin, pBot->pEdict->v.team, UTIL_MetresToGoldSrcUnits(5.0f), pBot->pEdict, true);

		if (!FNullEnt(HurtPlayer) && HurtPlayer->v.armorvalue < GetPlayerMaxArmour(HurtPlayer))
		{
			WeldTargetPlayer = HurtPlayer;
		}

		edict_t* DamagedStructure = UTIL_FindClosestDamagedStructure(pBot->pEdict->v.origin, MARINE_TEAM, UTIL_MetresToGoldSrcUnits(10.0f), false);

		if (!FNullEnt(DamagedStructure) && UTIL_PointIsDirectlyReachable(pBot->pEdict->v.origin, DamagedStructure->v.origin))
		{
			WeldTargetStructure = DamagedStructure;
		}
	}
	
	if (!FNullEnt(WeldTargetPlayer))
	{
		if (WeldTargetPlayer->v.armorvalue < GetPlayerMaxArmour(WeldTargetPlayer) / 2)
		{
			Task->TaskType = TASK_WELD;
			Task->TaskTarget = WeldTargetPlayer;
			Task->TaskLength = 10.0f;
			return;
		}
	}

	if (!FNullEnt(WeldTargetStructure))
	{
		if (FNullEnt(ResourceTower))
		{
			Task->TaskType = TASK_WELD;
			Task->TaskTarget = WeldTargetStructure;
			Task->TaskLength = 20.0f;
			return;
		}

		if (WeldTargetStructure->v.health < WeldTargetStructure->v.max_health * 0.5f)
		{
			Task->TaskType = TASK_WELD;
			Task->TaskTarget = WeldTargetStructure;
			Task->TaskLength = 20.0f;
			return;
		}
	}

	if (!FNullEnt(ResourceTower))
	{
		float DistToStructure = vDist2D(pBot->pEdict->v.origin, ResourceTower->v.origin);

		// Assault won't defend structures unless they're close by, so they don't get too distracted from their attacking duties
		if (DistToStructure < UTIL_MetresToGoldSrcUnits(10.0f))
		{
			int NumPotentialDefenders = UTIL_GetNumPlayersOfTeamInArea(ResourceTower->v.origin, DistToStructure, MARINE_TEAM, pBot->pEdict, CLASS_NONE, false);

			if (NumPotentialDefenders < 1)
			{
				Task->TaskType = TASK_DEFEND;
				Task->TaskTarget = ResourceTower;
				Task->TaskLength = 20.0f;
				return;
			}
		}
	}
	
}

bool MarineCombatThink(bot_t* pBot)
{
	edict_t* pEdict = pBot->pEdict;

	if (pBot->CurrentEnemy < 0) { return false; }

	edict_t* CurrentEnemy = pBot->TrackedEnemies[pBot->CurrentEnemy].EnemyEdict;
	enemy_status* TrackedEnemyRef = &pBot->TrackedEnemies[pBot->CurrentEnemy];

	int NavProfileIndex = UTIL_GetMoveProfileForBot(pBot, MOVESTYLE_NORMAL);

	// ENEMY IS OUT OF SIGHT

	if (!TrackedEnemyRef->bCurrentlyVisible && !UTIL_QuickTrace(pEdict, pBot->CurrentEyePosition, CurrentEnemy->v.origin))
	{

		// If we're out of primary ammo or badly hurt, then use opportunity to disengage and head to the nearest armoury to resupply
		if ((BotGetCurrentWeaponClipAmmo(pBot) < BotGetCurrentWeaponMaxClipAmmo(pBot) && BotGetPrimaryWeaponAmmoReserve(pBot) == 0) || pBot->pEdict->v.health < 50.0f)
		{
			edict_t* Armoury = UTIL_GetNearestStructureOfTypeInLocation(STRUCTURE_MARINE_ANYARMOURY, pBot->pEdict->v.origin, UTIL_MetresToGoldSrcUnits(50.0f), true, true);

			if (!FNullEnt(Armoury))
			{
				if (UTIL_PlayerInUseRange(pBot->pEdict, Armoury))
				{
					pBot->DesiredCombatWeapon = UTIL_GetBotMarinePrimaryWeapon(pBot);

					if (UTIL_GetBotCurrentWeapon(pBot) == pBot->DesiredCombatWeapon)
					{
						BotUseObject(pBot, Armoury, true);
					}
				}
				else
				{
					MoveTo(pBot, Armoury->v.origin, MOVESTYLE_NORMAL);
				}
				return true;
			}
		}

		MarineHuntEnemy(pBot, TrackedEnemyRef);
		return true;
	}

	// ENEMY IS VISIBLE

	pBot->DesiredCombatWeapon = BotMarineChooseBestWeapon(pBot, CurrentEnemy);

	if (UTIL_GetBotCurrentWeapon(pBot) != pBot->DesiredCombatWeapon) { return true; }

	NSPlayerClass EnemyClass = UTIL_GetPlayerClass(CurrentEnemy);
	float DistFromEnemySq = vDist2DSq(pEdict->v.origin, CurrentEnemy->v.origin);
	float MaxWeaponRange = UTIL_GetMaxIdealWeaponRange(UTIL_GetBotCurrentWeapon(pBot));
	float MinWeaponRange = UTIL_GetMinIdealWeaponRange(UTIL_GetBotCurrentWeapon(pBot));

	if (UTIL_GetBotCurrentWeapon(pBot) != WEAPON_MARINE_KNIFE && BotGetCurrentWeaponClipAmmo(pBot) == 0)
	{
		LookAt(pBot, CurrentEnemy);
		pEdict->v.button |= IN_RELOAD;

		Vector EnemyOrientation = UTIL_GetVectorNormal2D(pBot->pEdict->v.origin - CurrentEnemy->v.origin);

		Vector RetreatLocation = pBot->pEdict->v.origin + (EnemyOrientation * 100.0f);

		MoveTo(pBot, RetreatLocation, MOVESTYLE_NORMAL);

		return true;
	}

	// If we're really low on ammo then retreat to the nearest armoury while continuing to engage
	if (BotGetPrimaryWeaponClipAmmo(pBot) < BotGetPrimaryWeaponMaxClipSize(pBot) && BotGetPrimaryWeaponAmmoReserve(pBot) == 0 && BotGetSecondaryWeaponAmmoReserve(pBot) == 0)
	{
		edict_t* Armoury = UTIL_GetNearestStructureOfTypeInLocation(STRUCTURE_MARINE_ANYARMOURY, pBot->pEdict->v.origin, UTIL_MetresToGoldSrcUnits(50.0f), true, true);

		if (!FNullEnt(Armoury))
		{
			if (UTIL_PlayerInUseRange(pBot->pEdict, Armoury))
			{
				pBot->DesiredCombatWeapon = UTIL_GetBotMarinePrimaryWeapon(pBot);

				if (UTIL_GetBotCurrentWeapon(pBot) == pBot->DesiredCombatWeapon)
				{
					BotUseObject(pBot, Armoury, true);
				}

				return true;
			}
			else
			{
				MoveTo(pBot, Armoury->v.origin, MOVESTYLE_NORMAL);
			}
		}

		if (UTIL_GetBotCurrentWeapon(pBot) == WEAPON_MARINE_KNIFE || BotGetPrimaryWeaponClipAmmo(pBot) > 0)
		{
			BotAttackTarget(pBot, CurrentEnemy);
		}
		return true;
	}

	if (EnemyClass == CLASS_GORGE || IsPlayerGestating(CurrentEnemy))
	{
		BotAttackTarget(pBot, CurrentEnemy);
		if (DistFromEnemySq > sqrf(MaxWeaponRange))
		{
			MoveTo(pBot, CurrentEnemy->v.origin, MOVESTYLE_NORMAL);
		}
		else
		{

			Vector EngagementLocation = pBot->BotNavInfo.TargetDestination;

			float EngagementDist = vDist2DSq(EngagementLocation, CurrentEnemy->v.origin);

			if (!EngagementLocation || EngagementDist > sqrf(MaxWeaponRange) || EngagementDist < sqrf(MinWeaponRange) || !UTIL_QuickTrace(pBot->pEdict, EngagementLocation + Vector(0.0f, 0.0f, 10.0f), CurrentEnemy->v.origin))
			{
				float MinMaxDiff = MaxWeaponRange - MinWeaponRange;

				Vector EnemyOrientation = UTIL_GetVectorNormal2D(pBot->pEdict->v.origin - CurrentEnemy->v.origin);
				float MidDist = MinWeaponRange + (MinMaxDiff * 0.5f);

				Vector MidPoint = CurrentEnemy->v.origin + (EnemyOrientation * MidDist);

				MidPoint = UTIL_ProjectPointToNavmesh(MidPoint, NavProfileIndex);

				if (MidPoint != ZERO_VECTOR)
				{
					EngagementLocation = UTIL_GetRandomPointOnNavmeshInRadius(NavProfileIndex, MidPoint, MinMaxDiff);
				}
				else
				{
					EngagementLocation = UTIL_GetRandomPointOnNavmeshInRadius(NavProfileIndex, CurrentEnemy->v.origin, MinWeaponRange);
				}

				if (!UTIL_QuickTrace(pBot->pEdict, EngagementLocation + Vector(0.0f, 0.0f, 10.0f), CurrentEnemy->v.origin))
				{
					EngagementLocation = ZERO_VECTOR;
				}
			}

			if (EngagementLocation != ZERO_VECTOR)
			{
				MoveTo(pBot, EngagementLocation, MOVESTYLE_NORMAL);
			}

		}

		return true;
	}

	if (DistFromEnemySq > sqrf(MaxWeaponRange))
	{
		BotAttackTarget(pBot, CurrentEnemy);
		MoveTo(pBot, CurrentEnemy->v.origin, MOVESTYLE_NORMAL);

	}
	else
	{
		if (DistFromEnemySq < sqrf(MinWeaponRange))
		{
			Vector CurrentBackOffLocation = pBot->BotNavInfo.TargetDestination;

			if (!CurrentBackOffLocation || vDist2DSq(CurrentBackOffLocation, CurrentEnemy->v.origin) < sqrf(MinWeaponRange) || !UTIL_QuickTrace(pBot->pEdict, CurrentBackOffLocation + Vector(0.0f, 0.0f, 10.0f), CurrentEnemy->v.origin))
			{

				NavProfileIndex = UTIL_GetMoveProfileForBot(pBot, MOVESTYLE_NORMAL);

				float MinMaxDiff = MaxWeaponRange - MinWeaponRange;

				Vector EnemyOrientation = UTIL_GetVectorNormal2D(pBot->pEdict->v.origin - CurrentEnemy->v.origin);
				float MidDist = MinWeaponRange + (MinMaxDiff * 0.5f);

				Vector MidPoint = CurrentEnemy->v.origin + (EnemyOrientation * MidDist);

				MidPoint = UTIL_ProjectPointToNavmesh(MidPoint, NavProfileIndex);

				if (MidPoint != ZERO_VECTOR)
				{
					CurrentBackOffLocation = UTIL_GetRandomPointOnNavmeshInRadius(NavProfileIndex, MidPoint, MinMaxDiff);
				}
				else
				{
					CurrentBackOffLocation = UTIL_GetRandomPointOnNavmeshInRadius(NavProfileIndex, CurrentEnemy->v.origin, MinWeaponRange);
				}
			}

			BotAttackTarget(pBot, CurrentEnemy);
			MoveTo(pBot, CurrentBackOffLocation, MOVESTYLE_NORMAL);

			if (DistFromEnemySq < sqrf(UTIL_MetresToGoldSrcUnits(1.0f)))
			{
				BotJump(pBot);
			}

			return true;
		}

		Vector EnemyVelocity = UTIL_GetVectorNormal2D(CurrentEnemy->v.velocity);
		Vector EnemyOrientation = UTIL_GetVectorNormal2D(pBot->pEdict->v.origin - CurrentEnemy->v.origin);

		float MoveDot = UTIL_GetDotProduct2D(EnemyVelocity, EnemyOrientation);

		// Enemy is coming at us
		if (MoveDot > 0.0f)
		{
			if (!IsPlayerOnLadder(pBot->pEdict))
			{

				Vector RetreatLocation = pBot->pEdict->v.origin + (EnemyOrientation * 100.0f);

				BotAttackTarget(pBot, CurrentEnemy);
				MoveTo(pBot, RetreatLocation, MOVESTYLE_NORMAL);
			}
			else
			{
				Vector LadderTop = UTIL_GetNearestLadderTopPoint(pBot->pEdict);
				Vector LadderBottom = UTIL_GetNearestLadderBottomPoint(pBot->pEdict);

				Vector EnemyPointOnLine = vClosestPointOnLine(LadderBottom, LadderTop, CurrentEnemy->v.origin);

				bool bGoDownLadder = (vDist3DSq(EnemyPointOnLine, LadderBottom) > vDist3DSq(EnemyPointOnLine, LadderTop));

				Vector RetreatLocation = ZERO_VECTOR;

				if (bGoDownLadder)
				{
					RetreatLocation = UTIL_ProjectPointToNavmesh(LadderBottom);
				}
				else
				{
					RetreatLocation = UTIL_ProjectPointToNavmesh(LadderTop);
				}

				BotAttackTarget(pBot, CurrentEnemy);
				MoveTo(pBot, RetreatLocation, MOVESTYLE_NORMAL);
			}
		}
		else
		{
			if (!IsPlayerOnLadder(pBot->pEdict))
			{
				NavProfileIndex = UTIL_GetMoveProfileForBot(pBot, MOVESTYLE_NORMAL);

				Vector EngagementLocation = pBot->BotNavInfo.TargetDestination;

				float EngagementDist = vDist2DSq(EngagementLocation, CurrentEnemy->v.origin);

				if (!EngagementLocation || EngagementDist > sqrf(MaxWeaponRange) || EngagementDist < sqrf(MinWeaponRange) || !UTIL_QuickTrace(pBot->pEdict, EngagementLocation + Vector(0.0f, 0.0f, 10.0f), CurrentEnemy->v.origin))
				{
					EngagementLocation = UTIL_GetRandomPointOnNavmeshInRadius(NavProfileIndex, pBot->pEdict->v.origin, UTIL_MetresToGoldSrcUnits(2.0f));
				}

				BotAttackTarget(pBot, CurrentEnemy);
				MoveTo(pBot, EngagementLocation, MOVESTYLE_NORMAL);
			}
			else
			{
				Vector LadderTop = UTIL_GetNearestLadderTopPoint(pBot->pEdict);
				Vector LadderBottom = UTIL_GetNearestLadderBottomPoint(pBot->pEdict);

				Vector EnemyPointOnLine = vClosestPointOnLine(LadderBottom, LadderTop, CurrentEnemy->v.origin);

				bool bGoDownLadder = (vDist3DSq(EnemyPointOnLine, LadderBottom) < vDist3DSq(EnemyPointOnLine, LadderTop));

				Vector EngagementLocation = ZERO_VECTOR;

				if (bGoDownLadder)
				{
					EngagementLocation = UTIL_ProjectPointToNavmesh(LadderBottom);
				}
				else
				{
					EngagementLocation = UTIL_ProjectPointToNavmesh(LadderTop);
				}

				BotAttackTarget(pBot, CurrentEnemy);
				MoveTo(pBot, EngagementLocation, MOVESTYLE_NORMAL);
			}

		}

	}
	return true;
}

void MarineHuntEnemy(bot_t* pBot, enemy_status* TrackedEnemy)
{
	edict_t* CurrentEnemy = TrackedEnemy->EnemyEdict;

	if (FNullEnt(CurrentEnemy) || IsPlayerDead(CurrentEnemy)) { return; }

	Vector LastSeenLocation = (TrackedEnemy->bIsTracked) ? TrackedEnemy->TrackedLocation : TrackedEnemy->LastSeenLocation;
	float LastSeenTime = (TrackedEnemy->bIsTracked) ? TrackedEnemy->LastTrackedTime : TrackedEnemy->LastSeenTime;
	float TimeSinceLastSighting = (gpGlobals->time - LastSeenTime);

	// If the enemy is being motion tracked, or the last seen time was within the last 5 seconds, and the suspected location is close enough, then throw a grenade!
	if (BotHasGrenades(pBot) || (BotHasWeapon(pBot, WEAPON_MARINE_GL) && (BotGetPrimaryWeaponClipAmmo(pBot) > 0 || BotGetPrimaryWeaponAmmoReserve(pBot) > 0)))
	{
		if (TimeSinceLastSighting < 5.0f && vDist3DSq(pBot->pEdict->v.origin, LastSeenLocation) <= sqrf(UTIL_MetresToGoldSrcUnits(10.0f)))
		{
			Vector GrenadeThrowLocation = UTIL_GetGrenadeThrowTarget(pBot, LastSeenLocation, UTIL_MetresToGoldSrcUnits(5.0f));

			if (GrenadeThrowLocation != ZERO_VECTOR)
			{
				BotThrowGrenadeAtTarget(pBot, GrenadeThrowLocation);
				return;
			}
		}
	}

	pBot->DesiredCombatWeapon = BotMarineChooseBestWeapon(pBot, CurrentEnemy);

	if (UTIL_GetBotCurrentWeapon(pBot) != pBot->DesiredCombatWeapon) { return; }

	if (BotGetCurrentWeaponClipAmmo(pBot) < BotGetCurrentWeaponMaxClipAmmo(pBot) && BotGetCurrentWeaponReserveAmmo(pBot) > 0)
	{
		if (TrackedEnemy->bIsTracked)
		{
			if (vDist2DSq(pBot->pEdict->v.origin, LastSeenLocation) >= sqrf(UTIL_MetresToGoldSrcUnits(5.0f)))
			{
				pBot->pEdict->v.button |= IN_RELOAD;
			}
		}
		else
		{
			float ReloadTime = BotGetCurrentWeaponClipAmmo(pBot) < (BotGetCurrentWeaponMaxClipAmmo(pBot) * 0.5f) ? 2.0f : 5.0f;
			if (gpGlobals->time - LastSeenTime >= ReloadTime)
			{
				pBot->pEdict->v.button |= IN_RELOAD;
			}
		}

	}

	int NavProfileIndex = UTIL_GetMoveProfileForBot(pBot, MOVESTYLE_NORMAL);

	if (UTIL_PointIsReachable(NavProfileIndex, pBot->pEdict->v.origin, LastSeenLocation, max_player_use_reach))
	{
		MoveTo(pBot, LastSeenLocation, MOVESTYLE_NORMAL);
	}
	
	if (!TrackedEnemy->bIsTracked)
	{
		LookAt(pBot, LastSeenLocation);
	}

	return;
}

NSWeapon BotMarineChooseBestWeapon(bot_t* pBot, edict_t* target)
{

	if (!target)
	{
		if (BotGetPrimaryWeaponClipAmmo(pBot) > 0 || BotGetPrimaryWeaponAmmoReserve(pBot) > 0)
		{
			return UTIL_GetBotMarinePrimaryWeapon(pBot);
		}
		else if (BotGetSecondaryWeaponClipAmmo(pBot) > 0 || BotGetSecondaryWeaponAmmoReserve(pBot) > 0)
		{
			return UTIL_GetBotMarineSecondaryWeapon(pBot);
		}
		else
		{
			return UTIL_GetBotMarinePrimaryWeapon(pBot);
		}
	}

	if (UTIL_IsEdictPlayer(target))
	{
		float DistFromEnemy = vDist2DSq(pBot->pEdict->v.origin, target->v.origin);

		if (DistFromEnemy <= sqrf(UTIL_MetresToGoldSrcUnits(2.0f)))
		{
			if (BotGetPrimaryWeaponClipAmmo(pBot) == 0)
			{
				if (BotGetSecondaryWeaponClipAmmo(pBot) > 0)
				{
					return UTIL_GetBotMarineSecondaryWeapon(pBot);
				}
				else
				{
					return WEAPON_MARINE_KNIFE;
				}
			}
			else
			{
				return UTIL_GetBotMarinePrimaryWeapon(pBot);
			}
		}
		else
		{
			NSWeapon PrimaryWeapon = UTIL_GetBotMarinePrimaryWeapon(pBot);

			if (PrimaryWeapon == WEAPON_MARINE_SHOTGUN)
			{
				if (DistFromEnemy > sqrf(UTIL_MetresToGoldSrcUnits(10.0f)))
				{
					if (BotGetSecondaryWeaponClipAmmo(pBot) > 0 || BotGetSecondaryWeaponAmmoReserve(pBot) > 0)
					{
						return UTIL_GetBotMarineSecondaryWeapon(pBot);
					}
					else
					{
						if (BotGetPrimaryWeaponClipAmmo(pBot) > 0 || BotGetPrimaryWeaponAmmoReserve(pBot) > 0)
						{
							return PrimaryWeapon;
						}
						else
						{
							return WEAPON_MARINE_KNIFE;
						}
					}
				}
				else
				{
					if (BotGetPrimaryWeaponClipAmmo(pBot) > 0 || BotGetPrimaryWeaponAmmoReserve(pBot) > 0)
					{
						return PrimaryWeapon;
					}
					else
					{
						if (BotGetSecondaryWeaponClipAmmo(pBot) > 0)
						{
							return UTIL_GetBotMarineSecondaryWeapon(pBot);
						}
						else
						{
							return WEAPON_MARINE_KNIFE;
						}
					}
				}
			}
			else
			{
				if (DistFromEnemy > sqrf(UTIL_MetresToGoldSrcUnits(5.0f)))
				{
					if (BotGetPrimaryWeaponClipAmmo(pBot) > 0 || BotGetPrimaryWeaponAmmoReserve(pBot) > 0)
					{
						return PrimaryWeapon;
					}

					if (BotGetSecondaryWeaponClipAmmo(pBot) > 0 || BotGetSecondaryWeaponAmmoReserve(pBot) > 0)
					{
						return UTIL_GetBotMarineSecondaryWeapon(pBot);
					}

					return WEAPON_MARINE_KNIFE;
				}
				else
				{
					if (BotGetPrimaryWeaponClipAmmo(pBot) > 0 || (DistFromEnemy > sqrf(UTIL_MetresToGoldSrcUnits(5.0f)) && BotGetPrimaryWeaponAmmoReserve(pBot) > 0))
					{
						return PrimaryWeapon;
					}
					else
					{
						if (BotGetSecondaryWeaponClipAmmo(pBot) > 0)
						{
							return UTIL_GetBotMarineSecondaryWeapon(pBot);
						}
						else
						{
							return WEAPON_MARINE_KNIFE;
						}
					}
				}
			}
		}
	}
	else
	{
		return BotMarineChooseBestWeaponForStructure(pBot, target);
	}
}

NSWeapon BotAlienChooseBestWeaponForStructure(bot_t* pBot, edict_t* target)
{
	NSStructureType StructureType = UTIL_GetStructureTypeFromEdict(target);

	if (StructureType == STRUCTURE_NONE)
	{
		return UTIL_GetBotAlienPrimaryWeapon(pBot);
	}

	if (BotHasWeapon(pBot, WEAPON_GORGE_BILEBOMB))
	{
		return WEAPON_GORGE_BILEBOMB;
	}

	if (BotHasWeapon(pBot, WEAPON_SKULK_XENOCIDE))
	{
		int NumTargetsInArea = UTIL_GetNumPlayersOfTeamInArea(target->v.origin, UTIL_MetresToGoldSrcUnits(5.0f), MARINE_TEAM, NULL, CLASS_NONE, false);

		NumTargetsInArea += UTIL_GetNumPlacedStructuresOfTypeInRadius(STRUCTURE_MARINE_ANYTURRET, target->v.origin, UTIL_MetresToGoldSrcUnits(5.0f));

		NumTargetsInArea += UTIL_GetNumPlacedStructuresOfTypeInRadius(STRUCTURE_MARINE_PHASEGATE, target->v.origin, UTIL_MetresToGoldSrcUnits(5.0f));

		if (NumTargetsInArea > 2)
		{
			return WEAPON_SKULK_XENOCIDE;
		}
	}

	return UTIL_GetBotAlienPrimaryWeapon(pBot);
}

NSWeapon BotMarineChooseBestWeaponForStructure(bot_t* pBot, edict_t* target)
{
	NSStructureType StructureType = UTIL_GetStructureTypeFromEdict(target);

	if (StructureType == STRUCTURE_NONE)
	{
		if (BotGetPrimaryWeaponClipAmmo(pBot) > 0 || BotGetPrimaryWeaponAmmoReserve(pBot) > 0)
		{
			return UTIL_GetBotMarinePrimaryWeapon(pBot);
		}
		else if (BotGetSecondaryWeaponClipAmmo(pBot) > 0 || BotGetSecondaryWeaponAmmoReserve(pBot) > 0)
		{
			return UTIL_GetBotMarineSecondaryWeapon(pBot);
		}
		else
		{
			return UTIL_GetBotMarinePrimaryWeapon(pBot);
		}
	}

	if (StructureType == STRUCTURE_ALIEN_HIVE || StructureType == STRUCTURE_ALIEN_OFFENCECHAMBER)
	{
		if (UTIL_GetBotMarinePrimaryWeapon(pBot) == WEAPON_MARINE_MG)
		{
			if (BotHasGrenades(pBot))
			{
				return WEAPON_MARINE_GRENADE;
			}
		}

		if (BotGetPrimaryWeaponClipAmmo(pBot) > 0 || BotGetPrimaryWeaponAmmoReserve(pBot) > 0)
		{
			return UTIL_GetBotMarinePrimaryWeapon(pBot);
		}
		else if (BotGetSecondaryWeaponClipAmmo(pBot) > 0 || BotGetSecondaryWeaponAmmoReserve(pBot) > 0)
		{
			return UTIL_GetBotMarineSecondaryWeapon(pBot);
		}
		else
		{
			return WEAPON_MARINE_KNIFE;
		}
	}
	else
	{
		NSWeapon PrimaryWeapon = UTIL_GetBotMarinePrimaryWeapon(pBot);

		if ((PrimaryWeapon == WEAPON_MARINE_GL || PrimaryWeapon == WEAPON_MARINE_SHOTGUN) && (BotGetPrimaryWeaponClipAmmo(pBot) > 0 || BotGetPrimaryWeaponAmmoReserve(pBot) > 0))
		{
			return PrimaryWeapon;
		}

		return WEAPON_MARINE_KNIFE;
	}

	return UTIL_GetBotMarinePrimaryWeapon(pBot);
}


void MarineCheckWantsAndNeeds(bot_t* pBot)
{
	edict_t* pEdict = pBot->pEdict;

	if (FNullEnt(pEdict)) { return; }

	if (pBot->CurrentRole == BOT_ROLE_COMMAND)
	{
		UTIL_ClearBotTask(pBot, &pBot->WantsAndNeedsTask);
		return;
	}


	bool bUrgentlyNeedsHealth = (pEdict->v.health < 50.0f);

	// GL is a terrible choice to defend the base with...
	if (pBot->CurrentRole == BOT_ROLE_SWEEPER && BotHasWeapon(pBot, WEAPON_MARINE_GL))
	{
		BotDropWeapon(pBot);
	}

	if (bUrgentlyNeedsHealth)
	{
		const dropped_marine_item* HealthPackIndex = UTIL_GetNearestItemIndexOfType(ITEM_MARINE_HEALTHPACK, pEdict->v.origin, UTIL_MetresToGoldSrcUnits(15.0f));

		if (HealthPackIndex)
		{
			pBot->WantsAndNeedsTask.TaskType = TASK_GET_HEALTH;
			pBot->WantsAndNeedsTask.bOrderIsUrgent = true;
			pBot->WantsAndNeedsTask.TaskLocation = HealthPackIndex->Location;
			pBot->WantsAndNeedsTask.TaskTarget = HealthPackIndex->edict;

			return;
		}

		edict_t* NearestArmoury = UTIL_GetNearestStructureIndexOfType(pEdict->v.origin, STRUCTURE_MARINE_ANYARMOURY, UTIL_MetresToGoldSrcUnits(100.0f), true, IsPlayerMarine(pBot->pEdict));

		if (!FNullEnt(NearestArmoury))
		{
			float PhaseDist = UTIL_GetPhaseDistanceBetweenPoints(pBot->pEdict->v.origin, NearestArmoury->v.origin);

			if (PhaseDist < UTIL_MetresToGoldSrcUnits(30.0f))
			{
				pBot->WantsAndNeedsTask.TaskType = TASK_RESUPPLY;
				pBot->WantsAndNeedsTask.bOrderIsUrgent = true;
				pBot->WantsAndNeedsTask.TaskLocation = NearestArmoury->v.origin;
				pBot->WantsAndNeedsTask.TaskTarget = NearestArmoury;

				return;
			}
		}
	}

	if (UTIL_GetBotMarinePrimaryWeapon(pBot) == WEAPON_MARINE_MG)
	{
		if (pBot->WantsAndNeedsTask.TaskType != TASK_GET_WEAPON)
		{
			const dropped_marine_item* NewWeaponIndex = UTIL_GetNearestSpecialPrimaryWeapon(pEdict->v.origin, UTIL_MetresToGoldSrcUnits(15.0f), true);

			if (NewWeaponIndex)
			{
				// Don't grab the good stuff if there are humans who need it. Sweepers don't need grenade launchers
				if (!UTIL_IsAnyHumanNearLocationWithoutSpecialWeapon(NewWeaponIndex->Location, UTIL_MetresToGoldSrcUnits(10.0f)))
				{
					if (pBot->CurrentRole != BOT_ROLE_SWEEPER || NewWeaponIndex->ItemType != ITEM_MARINE_GRENADELAUNCHER)
					{
						pBot->WantsAndNeedsTask.TaskType = TASK_GET_WEAPON;
						pBot->WantsAndNeedsTask.bOrderIsUrgent = false;
						pBot->WantsAndNeedsTask.TaskLocation = NewWeaponIndex->Location;
						pBot->WantsAndNeedsTask.TaskTarget = NewWeaponIndex->edict;

						return;
					}
				}
			}
		}
		else
		{
			return;
		}
	}

	bool bNeedsAmmo = (BotGetPrimaryWeaponAmmoReserve(pBot) < (BotGetPrimaryWeaponMaxAmmoReserve(pBot) / 2));

	if (bNeedsAmmo)
	{
		const dropped_marine_item* AmmoPackIndex = UTIL_GetNearestItemIndexOfType(ITEM_MARINE_AMMO, pEdict->v.origin, UTIL_MetresToGoldSrcUnits(10.0f));

		if (AmmoPackIndex)
		{
			pBot->WantsAndNeedsTask.TaskType = TASK_GET_AMMO;
			pBot->WantsAndNeedsTask.bOrderIsUrgent = BotGetPrimaryWeaponAmmoReserve(pBot) == 0;
			pBot->WantsAndNeedsTask.TaskLocation = AmmoPackIndex->Location;
			pBot->WantsAndNeedsTask.TaskTarget = AmmoPackIndex->edict;

			return;
		}

		float DistanceWillingToTravel = (BotGetPrimaryWeaponAmmoReserve(pBot) == 0) ? UTIL_MetresToGoldSrcUnits(50.0f) : UTIL_MetresToGoldSrcUnits(15.0f);

		edict_t* NearestArmoury = UTIL_GetNearestStructureIndexOfType(pEdict->v.origin, STRUCTURE_MARINE_ANYARMOURY, UTIL_MetresToGoldSrcUnits(100.0f), true, IsPlayerMarine(pBot->pEdict));

		if (!FNullEnt(NearestArmoury))
		{
			float PhaseDist = UTIL_GetPhaseDistanceBetweenPoints(pBot->pEdict->v.origin, NearestArmoury->v.origin);

			if (PhaseDist <= DistanceWillingToTravel)
			{
				pBot->WantsAndNeedsTask.TaskType = TASK_RESUPPLY;
				pBot->WantsAndNeedsTask.bOrderIsUrgent = BotGetPrimaryWeaponAmmoReserve(pBot) == 0;
				pBot->WantsAndNeedsTask.TaskLocation = NearestArmoury->v.origin;
				pBot->WantsAndNeedsTask.TaskTarget = NearestArmoury;

				return;
			}
		}

	}

	if (!UTIL_PlayerHasWeapon(pEdict, WEAPON_MARINE_WELDER))
	{
		if (pBot->WantsAndNeedsTask.TaskType != TASK_GET_WEAPON)
		{
			const dropped_marine_item* WelderIndex = UTIL_GetNearestItemIndexOfType(ITEM_MARINE_WELDER, pEdict->v.origin, UTIL_MetresToGoldSrcUnits(15.0f));

			if (WelderIndex)
			{
				if (!UTIL_IsAnyHumanNearLocationWithoutWeapon(WEAPON_MARINE_WELDER, WelderIndex->Location, UTIL_MetresToGoldSrcUnits(5.0f)))
				{
					pBot->WantsAndNeedsTask.TaskType = TASK_GET_WEAPON;
					pBot->WantsAndNeedsTask.bOrderIsUrgent = false;
					pBot->WantsAndNeedsTask.TaskLocation = WelderIndex->Location;
					pBot->WantsAndNeedsTask.TaskTarget = WelderIndex->edict;

					return;
				}
			}
		}
		else
		{
			return;
		}
	}

	if (!UTIL_PlayerHasEquipment(pEdict))
	{
		if (pBot->WantsAndNeedsTask.TaskType != TASK_GET_EQUIPMENT)
		{
			const dropped_marine_item* EquipmentIndex = UTIL_GetNearestEquipment(pEdict->v.origin, UTIL_MetresToGoldSrcUnits(15.0f), true);

			if (EquipmentIndex)
			{
				// Don't grab the good stuff if there are humans who need it
				if (!UTIL_IsAnyHumanNearLocationWithoutEquipment(EquipmentIndex->Location, UTIL_MetresToGoldSrcUnits(10.0f)))
				{

					pBot->WantsAndNeedsTask.TaskType = TASK_GET_EQUIPMENT;
					pBot->WantsAndNeedsTask.bOrderIsUrgent = false;
					pBot->WantsAndNeedsTask.TaskLocation = EquipmentIndex->Location;
					pBot->WantsAndNeedsTask.TaskTarget = EquipmentIndex->edict;

					return;
				}
			}
		}
		else
		{
			return;
		}
	}
}

void MarineProgressCapResNodeTask(bot_t* pBot, bot_task* Task)
{
	if (!Task) { return; }

	float DistFromNode = vDist2DSq(pBot->pEdict->v.origin, Task->TaskLocation);

	if (DistFromNode > sqrf(UTIL_MetresToGoldSrcUnits(5.0f)) || !UTIL_QuickTrace(pBot->pEdict, pBot->CurrentEyePosition, (Task->TaskLocation + Vector(0.0f, 0.0f, 50.0f))))
	{
		MoveTo(pBot, Task->TaskLocation, MOVESTYLE_NORMAL);
		return;
	}

	const resource_node* ResNodeIndex = UTIL_FindNearestResNodeToLocation(Task->TaskLocation);

	if (!ResNodeIndex) { return; }

	if (ResNodeIndex->bIsOccupied && !FNullEnt(ResNodeIndex->TowerEdict))
	{
		Task->TaskTarget = ResNodeIndex->TowerEdict;
		// Cancel the waiting timeout since a tower has been placed for us
		Task->TaskLength = 0.0f;

		if (ResNodeIndex->bIsOwnedByMarines)
		{
			if (!UTIL_StructureIsFullyBuilt(ResNodeIndex->TowerEdict))
			{

				if (UTIL_PlayerHasLOSToEntity(pBot->pEdict, ResNodeIndex->TowerEdict, max_player_use_reach, true))
				{
					BotUseObject(pBot, ResNodeIndex->TowerEdict, true);
					if (vDist2DSq(pBot->pEdict->v.origin, ResNodeIndex->TowerEdict->v.origin) > sqrf(50.0f))
					{
						MoveDirectlyTo(pBot, ResNodeIndex->TowerEdict->v.origin);
					}
					return;
				}

				MoveTo(pBot, ResNodeIndex->TowerEdict->v.origin, MOVESTYLE_NORMAL);

				if (vDist2DSq(pBot->pEdict->v.origin, ResNodeIndex->TowerEdict->v.origin) < sqrf(UTIL_MetresToGoldSrcUnits(5.0f)))
				{
					LookAt(pBot, UTIL_GetCentreOfEntity(ResNodeIndex->TowerEdict));
				}

				return;
			}
		}
		else
		{
			NSWeapon AttackWeapon = BotMarineChooseBestWeaponForStructure(pBot, Task->TaskTarget);

			float MaxRange = UTIL_GetMaxIdealWeaponRange(AttackWeapon);
			bool bHullSweep = UTIL_IsMeleeWeapon(AttackWeapon);

			if (UTIL_PlayerHasLOSToEntity(pBot->pEdict, Task->TaskTarget, MaxRange, bHullSweep))
			{
				pBot->DesiredCombatWeapon = AttackWeapon;

				if (UTIL_GetBotCurrentWeapon(pBot) == AttackWeapon)
				{
					BotAttackTarget(pBot, Task->TaskTarget);
				}
			}
			else
			{
				MoveTo(pBot, Task->TaskTarget->v.origin, MOVESTYLE_NORMAL);
			}
		}
	}
	else
	{
		// Give the commander 30 seconds to drop a tower for us, or give up and move on
		if (Task->TaskLength == 0.0f)
		{
			Task->TaskStartedTime = gpGlobals->time;
			Task->TaskLength = 30.0f;
		}
		BotGuardLocation(pBot, Task->TaskLocation);
	}

	
}

void MarineProgressWeldTask(bot_t* pBot, bot_task* Task)
{
	float DistFromWeldLocation = vDist2DSq(pBot->pEdict->v.origin, Task->TaskTarget->v.origin);


	if (UTIL_PlayerInUseRange(pBot->pEdict, Task->TaskTarget))
	{
		LookAt(pBot, Task->TaskTarget);
		pBot->DesiredCombatWeapon = WEAPON_MARINE_WELDER;

		if (UTIL_GetBotCurrentWeapon(pBot) != WEAPON_MARINE_WELDER)
		{
			return;
		}

		pBot->pEdict->v.button |= IN_ATTACK;

		return;
	}

	if (vDist2DSq(pBot->pEdict->v.origin, Task->TaskTarget->v.origin) > sqrf(UTIL_MetresToGoldSrcUnits(2.0f)))
	{
		MoveTo(pBot, Task->TaskTarget->v.origin, MOVESTYLE_NORMAL);
		return;
	}

	if (!UTIL_PlayerHasLOSToEntity(pBot->pEdict, Task->TaskTarget, 9999.0f, false))
	{
		LookAt(pBot, UTIL_GetCentreOfEntity(Task->TaskTarget));

		Vector WeldLocation = pBot->BotNavInfo.TargetDestination;

		if (!WeldLocation || vDist2DSq(pBot->pEdict->v.origin, Task->TaskTarget->v.origin) > sqrf(UTIL_MetresToGoldSrcUnits(1.5f)))
		{
			int MoveProfile = UTIL_GetMoveProfileForBot(pBot, MOVESTYLE_NORMAL);
			WeldLocation = UTIL_GetRandomPointOnNavmeshInDonut(MoveProfile, Task->TaskTarget->v.origin, UTIL_MetresToGoldSrcUnits(1.0f), UTIL_MetresToGoldSrcUnits(1.5f));

			if (!WeldLocation)
			{
				WeldLocation = Task->TaskTarget->v.origin;
			}
		}

		MoveTo(pBot, WeldLocation, MOVESTYLE_NORMAL);

		return;
	}
	else
	{
		MoveTo(pBot, Task->TaskTarget->v.origin, MOVESTYLE_NORMAL);
	}
}

bool UTIL_IsMarineTaskStillValid(bot_t* pBot, bot_task* Task)
{
	if (!Task) { return false; }

	switch (Task->TaskType)
	{
	case TASK_NONE:
		return false;
	case TASK_MOVE:
		return UTIL_IsMoveTaskStillValid(pBot, Task);
	case TASK_GET_AMMO:
		return UTIL_IsAmmoPickupTaskStillValid(pBot, Task);
	case TASK_GET_HEALTH:
		return UTIL_IsHealthPickupTaskStillValid(pBot, Task);
	case TASK_GET_EQUIPMENT:
		return UTIL_IsEquipmentPickupTaskStillValid(pBot, Task);
	case TASK_GET_WEAPON:
		return UTIL_IsWeaponPickupTaskStillValid(pBot, Task);
	case TASK_RESUPPLY:
		return UTIL_IsResupplyTaskStillValid(pBot, Task);
	case TASK_ATTACK:
		return UTIL_IsAttackTaskStillValid(pBot, Task);
	case TASK_GUARD:
		return UTIL_IsGuardTaskStillValid(pBot, Task);
	case TASK_BUILD:
		return UTIL_IsBuildTaskStillValid(pBot, Task);
	case TASK_CAP_RESNODE:
		return UTIL_IsMarineCapResNodeTaskStillValid(pBot, Task);
	case TASK_DEFEND:
		return UTIL_IsDefendTaskStillValid(pBot, Task);
	case TASK_WELD:
		return UTIL_IsWeldTaskStillValid(pBot, Task);
	case TASK_GRENADE:
		return BotHasGrenades(pBot) || (BotHasWeapon(pBot, WEAPON_MARINE_GL) && (BotGetPrimaryWeaponClipAmmo(pBot) > 0 || BotGetPrimaryWeaponAmmoReserve(pBot) > 0));
	default:
		return true;
	}

	return false;
}

bool UTIL_IsMarineCapResNodeTaskStillValid(bot_t* pBot, bot_task* Task)
{
	if (!Task || !Task->TaskLocation) { return false; }

	const resource_node* ResNodeIndex = UTIL_FindNearestResNodeToLocation(Task->TaskLocation);

	if (!ResNodeIndex) { return false; }

	// Always obey commander orders even if there's a bunch of other marines already there
	if (!Task->bIssuedByCommander)
	{
		int NumMarinesNearby = UTIL_GetNumPlayersOfTeamInArea(Task->TaskLocation, UTIL_MetresToGoldSrcUnits(4.0f), MARINE_TEAM, pBot->pEdict, CLASS_MARINE_COMMANDER, false);

		if (NumMarinesNearby >= 2 && vDist2DSq(pBot->pEdict->v.origin, Task->TaskLocation) > sqrf(UTIL_MetresToGoldSrcUnits(4.0f))) { return false; }
	}

	if (ResNodeIndex->bIsOccupied)
	{
		if (ResNodeIndex->bIsOwnedByMarines && !FNullEnt(ResNodeIndex->TowerEdict))
		{
			return !UTIL_StructureIsFullyBuilt(ResNodeIndex->TowerEdict);
		}
		else
		{
			return true;
		}
	}

	return true;
}

bool UTIL_IsWeldTaskStillValid(bot_t* pBot, bot_task* Task)
{
	if (!Task) { return false; }
	if (FNullEnt(Task->TaskTarget) || Task->TaskTarget->v.deadflag != DEAD_NO) { return false; }
	if (Task->TaskTarget == pBot->pEdict) { return false; }
	if (!BotHasWeapon(pBot, WEAPON_MARINE_WELDER)) { return false; }

	if (UTIL_IsEdictPlayer(Task->TaskTarget))
	{
		if (!IsPlayerMarine(Task->TaskTarget) || IsPlayerBeingDigested(Task->TaskTarget)) { return false; }
		return (Task->TaskTarget->v.armorvalue < GetPlayerMaxArmour(Task->TaskTarget));
	}
	else
	{
		return (Task->TaskTarget->v.health < Task->TaskTarget->v.max_health);
	}
}

bool UTIL_IsAmmoPickupTaskStillValid(bot_t* pBot, bot_task* Task)
{
	if (IsPlayerDead(pBot->pEdict) || IsPlayerBeingDigested(pBot->pEdict) || FNullEnt(Task->TaskTarget) || (Task->TaskTarget->v.effects & EF_NODRAW)) { return false; }

	if (!UTIL_PointIsReachable(MARINE_REGULAR_NAV_PROFILE, pBot->pEdict->v.origin, Task->TaskTarget->v.origin, 32.0f)) { return false; }

	return (vDist2DSq(pBot->pEdict->v.origin, Task->TaskTarget->v.origin) < sqrf(UTIL_MetresToGoldSrcUnits(20.0f))) && (BotGetPrimaryWeaponAmmoReserve(pBot) < BotGetPrimaryWeaponMaxAmmoReserve(pBot));
}

bool UTIL_IsHealthPickupTaskStillValid(bot_t* pBot, bot_task* Task)
{
	if (IsPlayerDead(pBot->pEdict) || IsPlayerBeingDigested(pBot->pEdict) || FNullEnt(Task->TaskTarget) || (Task->TaskTarget->v.effects & EF_NODRAW)) { return false; }

	if (!UTIL_PointIsReachable(MARINE_REGULAR_NAV_PROFILE, pBot->pEdict->v.origin, Task->TaskTarget->v.origin, 32.0f)) { return false; }

	return ((vDist2DSq(pBot->pEdict->v.origin, Task->TaskTarget->v.origin) < sqrf(UTIL_MetresToGoldSrcUnits(20.0f))) && (pBot->pEdict->v.health < pBot->pEdict->v.max_health));
}

BotRole MarineGetBestBotRole(const bot_t* pBot)
{
	// Take command if configured to and nobody is already commanding

	CommanderMode BotCommanderMode = CONFIG_GetCommanderMode();

	if (BotCommanderMode != COMMANDERMODE_NEVER)
	{
		if (!UTIL_IsThereACommander())
		{
			if (BotCommanderMode == COMMANDERMODE_IFNOHUMAN)
			{
				if (!UTIL_IsAnyHumanOnMarineTeam() && UTIL_GetBotsWithRoleType(BOT_ROLE_COMMAND, MARINE_TEAM, pBot->pEdict) < 1)
				{
					return BOT_ROLE_COMMAND;
				}
			}
			else
			{
				if (UTIL_GetBotsWithRoleType(BOT_ROLE_COMMAND, MARINE_TEAM, pBot->pEdict) < 1)
				{
					return BOT_ROLE_COMMAND;
				}
			}
		}
	}

	// Only guard the base if there isn't a phase gate or turret factory in base
	
	int NumDefenders = UTIL_GetBotsWithRoleType(BOT_ROLE_SWEEPER, MARINE_TEAM, pBot->pEdict);

	// One marine to play sweeper at all times
	if (NumDefenders < 1)
	{
		return BOT_ROLE_SWEEPER;
	}


	int NumPlayersOnTeam = UTIL_GetNumPlayersOnTeam(MARINE_TEAM);

	if (NumPlayersOnTeam == 0) { return BOT_ROLE_ASSAULT; } // This shouldn't happen, but let's avoid a potential divide by zero later on anyway...

	int NumTotalResNodes = UTIL_GetNumResNodes();

	if (NumTotalResNodes == 0)
	{
		return BOT_ROLE_ASSAULT; // Again, shouldn't happen, but avoids potential divide by zero
	}

	int NumMarineResTowers = UTIL_GetNumPlacedStructuresOfType(STRUCTURE_MARINE_RESTOWER);

	int NumRemainingResNodes = NumTotalResNodes - NumMarineResTowers;

	int NumCappers = UTIL_GetBotsWithRoleType(BOT_ROLE_FIND_RESOURCES, MARINE_TEAM, pBot->pEdict);

	if (NumRemainingResNodes > 1 && NumCappers == 0)
	{
		return BOT_ROLE_FIND_RESOURCES;
	}

	// How much of the map do we currently dominate?
	float ResTowerRatio = ((float)NumMarineResTowers / (float)NumTotalResNodes);

	// If we own less than a third of the map, prioritise capping resource nodes
	if (ResTowerRatio < 0.30f)
	{
		return BOT_ROLE_FIND_RESOURCES;
	}

	if (ResTowerRatio <= 0.5f)
	{
		float CapperRatio = ((float)NumCappers / (float)NumPlayersOnTeam);

		if (CapperRatio < 0.2f)
		{
			return BOT_ROLE_FIND_RESOURCES;
		}
	}

	return BOT_ROLE_ASSAULT;
}