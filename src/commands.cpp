/**
 * =============================================================================
 * CS2Fixes
 * Copyright (C) 2023 Source2ZE
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "detours.h"
#include "common.h"
#include "utlstring.h"
#include "recipientfilters.h"
#include "commands.h"
#include "utils/entity.h"
#include "entity/cbaseentity.h"
#include "entity/ccsweaponbase.h"
#include "entity/ccsplayercontroller.h"
#include "entity/ccsplayerpawn.h"
#include "entity/cbasemodelentity.h"
#include "playermanager.h"
#include "adminsystem.h"
#include "ctimer.h"

#include "tier0/memdbgon.h"


extern CEntitySystem *g_pEntitySystem;
extern IVEngineServer2 *g_pEngineServer2;

extern bool practiceMode;

void ParseChatCommand(const char *pMessage, CCSPlayerController *pController)
{
	if (!pController)
		return;

	CCommand args;
	args.Tokenize(pMessage + 1);

	uint16 index = g_CommandList.Find(hash_32_fnv1a_const(args[0]));

	if (g_CommandList.IsValidIndex(index))
	{
		g_CommandList[index](args, pController);
	}
}

void ClientPrintAll(int hud_dest, const char *msg, ...)
{
	va_list args;
	va_start(args, msg);

	char buf[256];
	V_vsnprintf(buf, sizeof(buf), msg, args);

	va_end(args);

	addresses::UTIL_ClientPrintAll(hud_dest, buf, nullptr, nullptr, nullptr, nullptr);
}

void ClientPrint(CBasePlayerController *player, int hud_dest, const char *msg, ...)
{
	va_list args;
	va_start(args, msg);

	char buf[256];
	V_vsnprintf(buf, sizeof(buf), msg, args);

	va_end(args);

	addresses::ClientPrint(player, hud_dest, buf, nullptr, nullptr, nullptr, nullptr);
}

bool match_paused = false;
bool ct_ready = true;
bool t_ready = true;

CON_COMMAND_CHAT(pause, "Request pause")
{
	if (!player)
		return;

	int iPlayer = player->GetPlayerSlot();

	CBasePlayerController* pPlayer = (CBasePlayerController*)g_pEntitySystem->GetBaseEntity((CEntityIndex)(player->GetPlayerSlot() + 1));

	g_pEngineServer2->ServerCommand("mp_pause_match");

	ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX"%s requested a pause", player->GetPlayerName());

	match_paused = true;
	ct_ready = false;
	t_ready = false;	
}

CON_COMMAND_CHAT(unpause, "Request unpause")
{
	if (!player)
		return;

	if(!match_paused)
		return;
	
	CBasePlayerController* pPlayer = (CBasePlayerController*)g_pEntitySystem->GetBaseEntity((CEntityIndex)(player->GetPlayerSlot() + 1));
	
	int teamSide = pPlayer->m_iTeamNum();
	if( teamSide == CS_TEAM_T && !t_ready){
		t_ready = true;
	}else if( teamSide == CS_TEAM_CT && !ct_ready){
		ct_ready = true;
	}

	if(ct_ready && !t_ready){
		ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX"CT ready, type .unpause");
		return;
	}else if(!ct_ready && t_ready){
		ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX"T ready, type .unpause");
		return;
	}

	match_paused = false;
	ClientPrintAll(HUD_PRINTTALK, CHAT_PREFIX"Match \2unpaused");
	g_pEngineServer2->ServerCommand("mp_unpause_match");
}

CON_COMMAND_CHAT(spawn, "teleport to desired spawn")
{
	if (!player)
		return;
	
	if (!practiceMode){
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"Only available on practice mode");
		return;
	}

	if (args.ArgC() < 2)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX "Usage: !spawn <spawn number>");
		return;
	}

	char teamName[256];
	if(player->m_iTeamNum == CS_TEAM_T){
		V_snprintf(teamName, sizeof(teamName), "info_player_terrorist");
	}else if(player->m_iTeamNum == CS_TEAM_CT){
		V_snprintf(teamName, sizeof(teamName), "info_player_counterterrorist");
	}else{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"You cannot teleport in spectator!");
		return;
	}

	//Count spawnpoints (info_player_counterterrorist & info_player_terrorist)
	SpawnPoint* spawn = nullptr;
	CUtlVector<SpawnPoint*> spawns;
	while (nullptr != (spawn = (SpawnPoint*)UTIL_FindEntityByClassname(spawn, teamName)))
	{
		if (spawn->m_bEnabled())
		{
			// ClientPrint(player, HUD_PRINTTALK, "Spawn %i: %f / %f / %f", spawns.Count(), spawn->GetAbsOrigin().x, spawn->GetAbsOrigin().y, spawn->GetAbsOrigin().z);
			spawns.AddToTail(spawn);
		}
	}

	//Pick and get position of random spawnpoint
	//Spawns selection from 1 to spawns.Count()
	int targetSpawn = atoi(args[1]) - 1;
	int spawnIndex = targetSpawn % spawns.Count();
	Vector spawnpos = spawns[spawnIndex]->GetAbsOrigin();

	//Here's where the mess starts
	CBasePlayerPawn *pPawn = player->GetPawn();
	if (!pPawn)
	{
		return;
	}
	if (pPawn->m_iHealth() <= 0)
	{
		ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"You cannot teleport when dead!");
		return;
	}

	int totalSpawns = spawns.Count();

	pPawn->SetAbsOrigin(spawnpos);

	ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"You have been teleported to spawn. %i/%i", spawnIndex +1, totalSpawns);			
}

/*

CON_COMMAND_CHAT(getorigin, "get your origin")
{
	if (!player)
		return;

	Vector vecAbsOrigin = player->GetPawn()->GetAbsOrigin();

	ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"Your origin is %f %f %f", vecAbsOrigin.x, vecAbsOrigin.y, vecAbsOrigin.z);
}

CON_COMMAND_CHAT(setorigin, "set your origin")
{
	if (!player)
		return;

	CBasePlayerPawn *pPawn = player->GetPawn();
	Vector vecNewOrigin;
	V_StringToVector(args.ArgS(), vecNewOrigin);

	pPawn->SetAbsOrigin(vecNewOrigin);

	ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"Your origin is now %f %f %f", vecNewOrigin.x, vecNewOrigin.y, vecNewOrigin.z);
}

CON_COMMAND_CHAT(getstats, "get your stats")
{
	if (!player)
		return;

	CSMatchStats_t *stats = &player->m_pActionTrackingServices->m_matchStats();

	ClientPrint(player, HUD_PRINTCENTER, 
		"Kills: %i\n"
		"Deaths: %i\n"
		"Assists: %i\n"
		"Damage: %i"
		, stats->m_iKills.Get(), stats->m_iDeaths.Get(), stats->m_iAssists.Get(), stats->m_iDamage.Get());

	ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"Kills: %d", stats->m_iKills.Get());
	ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"Deaths: %d", stats->m_iDeaths.Get());
	ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"Assists: %d", stats->m_iAssists.Get());
	ClientPrint(player, HUD_PRINTTALK, CHAT_PREFIX"Damage: %d", stats->m_iDamage.Get());
}

*/
// Lookup a weapon classname in the weapon map and "initialize" it.
// Both m_bInitialized and m_iItemDefinitionIndex need to be set for a weapon to be pickable and not crash clients,
// and m_iItemDefinitionIndex needs to be the correct ID from weapons.vdata so the gun behaves as it should.

