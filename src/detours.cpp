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

#include "cdetour.h"
#include "common.h"
#include "module.h"
#include "addresses.h"
#include "commands.h"
#include "interfaces/cs2_interfaces.h"
#include "detours.h"
#include "ctimer.h"
#include "irecipientfilter.h"
#include "entity/ccsplayercontroller.h"
#include "entity/ccsplayerpawn.h"
#include "entity/cbasemodelentity.h"
#include "entity/ccsweaponbase.h"
#include "entity/ctriggerpush.h"
#include "playermanager.h"

#include "tier0/memdbgon.h"

extern CGlobalVars *gpGlobals;
extern CEntitySystem *g_pEntitySystem;

DECLARE_DETOUR(Host_Say, Detour_Host_Say, &modules::server);
DECLARE_DETOUR(UTIL_SayTextFilter, Detour_UTIL_SayTextFilter, &modules::server);
DECLARE_DETOUR(UTIL_SayText2Filter, Detour_UTIL_SayText2Filter, &modules::server);

void FASTCALL Detour_UTIL_SayTextFilter(IRecipientFilter &filter, const char *pText, CCSPlayerController *pPlayer, uint64 eMessageType)
{
	int entindex = filter.GetRecipientIndex(0).Get();
	CCSPlayerController *target = (CCSPlayerController *)g_pEntitySystem->GetBaseEntity((CEntityIndex)entindex);

	if (pPlayer)
		return UTIL_SayTextFilter(filter, pText, pPlayer, eMessageType);

	char buf[256];
	V_snprintf(buf, sizeof(buf), "%s %s", " \7CONSOLE:\4", pText + sizeof("Console:"));

	UTIL_SayTextFilter(filter, buf, pPlayer, eMessageType);
}

void FASTCALL Detour_UTIL_SayText2Filter(
	IRecipientFilter &filter,
	CCSPlayerController *pEntity,
	uint64 eMessageType,
	const char *msg_name,
	const char *param1,
	const char *param2,
	const char *param3,
	const char *param4)
{
	int entindex = filter.GetRecipientIndex(0).Get() + 1;
	CCSPlayerController *target = (CCSPlayerController *)g_pEntitySystem->GetBaseEntity((CEntityIndex)entindex);

#ifdef _DEBUG
	if (target)
		Message("Chat from %s to %s: %s\n", param1, target->GetPlayerName(), param2);
#endif

	UTIL_SayText2Filter(filter, pEntity, eMessageType, msg_name, param1, param2, param3, param4);
}

void FASTCALL Detour_Host_Say(CCSPlayerController *pController, CCommand &args, bool teamonly, int unk1, const char *unk2)
{
	if (args.ArgC() < 2 || *args[1] != '/')
		Host_Say(pController, args, teamonly, unk1, unk2);

	if (*args[1] == '!' || *args[1] == '/' || *args[1] == '.')
		ParseChatCommand(args[1], pController);
}

void Detour_Log()
{
	return;
}

bool FASTCALL Detour_IsChannelEnabled(LoggingChannelID_t channelID, LoggingSeverity_t severity)
{
	return false;
}

CDetour<decltype(Detour_Log)> g_LoggingDetours[] =
{
	CDetour<decltype(Detour_Log)>(&modules::tier0, Detour_Log, "Msg" ),
	//CDetour<decltype(Detour_Log)>( modules::tier0, Detour_Log, "?ConMsg@@YAXPEBDZZ" ),
	//CDetour<decltype(Detour_Log)>( modules::tier0, Detour_Log, "?ConColorMsg@@YAXAEBVColor@@PEBDZZ" ),
	CDetour<decltype(Detour_Log)>( &modules::tier0, Detour_Log, "ConDMsg" ),
	CDetour<decltype(Detour_Log)>( &modules::tier0, Detour_Log, "DevMsg" ),
	CDetour<decltype(Detour_Log)>( &modules::tier0, Detour_Log, "Warning" ),
	CDetour<decltype(Detour_Log)>( &modules::tier0, Detour_Log, "DevWarning" ),
	//CDetour<decltype(Detour_Log)>( modules::tier0, Detour_Log, "?DevWarning@@YAXPEBDZZ" ),
	CDetour<decltype(Detour_Log)>( &modules::tier0, Detour_Log, "LoggingSystem_Log" ),
	CDetour<decltype(Detour_Log)>( &modules::tier0, Detour_Log, "LoggingSystem_LogDirect" ),
	CDetour<decltype(Detour_Log)>( &modules::tier0, Detour_Log, "LoggingSystem_LogAssert" ),
	//CDetour<decltype(Detour_Log)>( modules::tier0, Detour_IsChannelEnabled, "LoggingSystem_IsChannelEnabled" ),
};

void ToggleLogs()
{
	static bool bBlock = false;

	if (!bBlock)
	{
		for (int i = 0; i < sizeof(g_LoggingDetours) / sizeof(*g_LoggingDetours); i++)
			g_LoggingDetours[i].EnableDetour();
	}
	else
	{
		for (int i = 0; i < sizeof(g_LoggingDetours) / sizeof(*g_LoggingDetours); i++)
			g_LoggingDetours[i].DisableDetour();
	}

	bBlock = !bBlock;
}

CUtlVector<CDetourBase *> g_vecDetours;

void InitDetours()
{
	g_vecDetours.PurgeAndDeleteElements();

	for (int i = 0; i < sizeof(g_LoggingDetours) / sizeof(*g_LoggingDetours); i++)
		g_LoggingDetours[i].CreateDetour();

	UTIL_SayTextFilter.CreateDetour();
	UTIL_SayTextFilter.EnableDetour();

	UTIL_SayText2Filter.CreateDetour();
	UTIL_SayText2Filter.EnableDetour();

	Host_Say.CreateDetour();
	Host_Say.EnableDetour();

	/*IsHearingClient.CreateDetour();
	IsHearingClient.EnableDetour();
*/
	/*CSoundEmitterSystem_EmitSound.CreateDetour();
	CSoundEmitterSystem_EmitSound.EnableDetour();*/

	//CCSWeaponBase_Spawn.CreateDetour();
	//CCSWeaponBase_Spawn.EnableDetour();

	//TriggerPush_Touch.CreateDetour();
	//TriggerPush_Touch.EnableDetour();
}

void FlushAllDetours()
{
	g_vecDetours.Purge();
}
