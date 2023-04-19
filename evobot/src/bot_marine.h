//
// EvoBot - Neoptolemus' Natural Selection bot, based on Botman's HPB bot template
//
// bot_gorge.h
// 
// Contains gorge-related functions. Needs refactoring into helper function file
//

#pragma once

#ifndef BOT_MARINE_H
#define BOT_MARINE_H

#include "bot_structs.h"

void MarineThink(bot_t* pBot);

bool MarineCombatThink(bot_t* pBot);
void MarineHuntEnemy(bot_t* pBot, enemy_status* TrackedEnemy);

// Helper function to pick the best weapon for any given situation and target type.
NSWeapon BotMarineChooseBestWeapon(bot_t* pBot, edict_t* target);
// Sub function for BotMarineChooseBestWeapon if target is a structure to pick the best weapon for attacking it
NSWeapon BotMarineChooseBestWeaponForStructure(bot_t* pBot, edict_t* target);
NSWeapon BotAlienChooseBestWeaponForStructure(bot_t* pBot, edict_t* target);

void MarineCheckWantsAndNeeds(bot_t* pBot);

void MarineProgressCapResNodeTask(bot_t* pBot, bot_task* Task);
void MarineProgressWeldTask(bot_t* pBot, bot_task* Task);

void MarineSetSecondaryTask(bot_t* pBot, bot_task* Task);

void MarineSweeperSetPrimaryTask(bot_t* pBot, bot_task* Task);
void MarineCapperSetPrimaryTask(bot_t* pBot, bot_task* Task);
void MarineAssaultSetPrimaryTask(bot_t* pBot, bot_task* Task);

void MarineSweeperSetSecondaryTask(bot_t* pBot, bot_task* Task);
void MarineCapperSetSecondaryTask(bot_t* pBot, bot_task* Task);
void MarineAssaultSetSecondaryTask(bot_t* pBot, bot_task* Task);

// Checks if the marine's current task is valid
bool UTIL_IsMarineTaskStillValid(bot_t* pBot, bot_task* Task);

bool UTIL_IsMarineCapResNodeTaskStillValid(bot_t* pBot, bot_task* Task);
bool UTIL_IsWeldTaskStillValid(bot_t* pBot, bot_task* Task);
bool UTIL_IsAmmoPickupTaskStillValid(bot_t* pBot, bot_task* Task);
bool UTIL_IsHealthPickupTaskStillValid(bot_t* pBot, bot_task* Task);

// Determines the individual bot's most appropriate role at this moment based on the state of play.
BotRole MarineGetBestBotRole(const bot_t* pBot);

#endif