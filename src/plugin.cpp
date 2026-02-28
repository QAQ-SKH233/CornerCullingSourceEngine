/**
 * ======================================================
 * Culling - Anti-wallhack occlusion culling for CS2
 * Migrated from CS:GO SourceMod extension to
 * CS2 Metamod:Source 2.0 plugin.
 *
 * Original author: Andrew H. (87andrewh)
 * ======================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 */

#include <stdio.h>
#include <string.h>
#include "plugin.h"
#include "iserver.h"
#include "../CornerCulling/CullingIO.h"

SH_DECL_HOOK3_void(IServerGameDLL, GameFrame, SH_NOATTRIB, 0, bool, bool, bool);
SH_DECL_HOOK4_void(IServerGameClients, ClientActive, SH_NOATTRIB, 0, CPlayerSlot, bool, const char *, uint64);
SH_DECL_HOOK5_void(IServerGameClients, ClientDisconnect, SH_NOATTRIB, 0, CPlayerSlot, ENetworkDisconnectionReason, const char *, uint64, const char *);
SH_DECL_HOOK4_void(IServerGameClients, ClientPutInServer, SH_NOATTRIB, 0, CPlayerSlot, char const *, int, uint64);

CullingPlugin g_CullingPlugin;

IServerGameDLL *server = NULL;
IServerGameClients *gameclients = NULL;
IVEngineServer *engine = NULL;
ICvar *icvar = NULL;

static CullingController cullingController;

// Maximum lookahead in milliseconds.
// A low value enforces strict culling, but laggy clients may experience popping.
// A high value will grant a greater advantage to wallhackers.
static int maxLookahead = 120;
static char currentMapName[128] = "";

// Player data arrays
static int teams[MAX_CHARACTERS + 1];
static float eyesFlat[(MAX_CHARACTERS + 2) * 3];
static float basesFlat[(MAX_CHARACTERS + 2) * 3];
static float yaws[MAX_CHARACTERS + 1];
static float pitches[MAX_CHARACTERS + 1];
static float speeds[MAX_CHARACTERS + 1];

// Per-slot tracking of active players
static bool playerActive[MAX_CHARACTERS + 1] = {false};

CGlobalVars *GetGameGlobals()
{
	INetworkGameServer *networkServer = g_pNetworkServerService->GetIGameServer();
	if (!networkServer)
		return nullptr;
	return networkServer->GetGlobals();
}

PLUGIN_EXPOSE(CullingPlugin, g_CullingPlugin);

bool CullingPlugin::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);
	GET_V_IFACE_CURRENT(GetEngineFactory, icvar, ICvar, CVAR_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetServerFactory, server, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
	GET_V_IFACE_ANY(GetServerFactory, gameclients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
	GET_V_IFACE_ANY(GetEngineFactory, g_pNetworkServerService, INetworkServerService, NETWORKSERVERSERVICE_INTERFACE_VERSION);

	g_SMAPI->AddListener(this, this);

	META_CONPRINTF("[Culling] Loading anti-wallhack occlusion culling plugin.\n");

	SH_ADD_HOOK(IServerGameDLL, GameFrame, server, SH_MEMBER(this, &CullingPlugin::Hook_GameFrame), true);
	SH_ADD_HOOK(IServerGameClients, ClientActive, gameclients, SH_MEMBER(this, &CullingPlugin::Hook_ClientActive), true);
	SH_ADD_HOOK(IServerGameClients, ClientDisconnect, gameclients, SH_MEMBER(this, &CullingPlugin::Hook_ClientDisconnect), true);
	SH_ADD_HOOK(IServerGameClients, ClientPutInServer, gameclients, SH_MEMBER(this, &CullingPlugin::Hook_ClientPutInServer), true);

	g_pCVar = icvar;
	META_CONVAR_REGISTER(FCVAR_RELEASE | FCVAR_GAMEDLL);

	// Initialize player tracking
	memset(teams, 0, sizeof(teams));
	memset(playerActive, 0, sizeof(playerActive));

	META_CONPRINTF("[Culling] Plugin loaded successfully.\n");

	return true;
}

bool CullingPlugin::Unload(char *error, size_t maxlen)
{
	SH_REMOVE_HOOK(IServerGameDLL, GameFrame, server, SH_MEMBER(this, &CullingPlugin::Hook_GameFrame), true);
	SH_REMOVE_HOOK(IServerGameClients, ClientActive, gameclients, SH_MEMBER(this, &CullingPlugin::Hook_ClientActive), true);
	SH_REMOVE_HOOK(IServerGameClients, ClientDisconnect, gameclients, SH_MEMBER(this, &CullingPlugin::Hook_ClientDisconnect), true);
	SH_REMOVE_HOOK(IServerGameClients, ClientPutInServer, gameclients, SH_MEMBER(this, &CullingPlugin::Hook_ClientPutInServer), true);

	return true;
}

void CullingPlugin::AllPluginsLoaded()
{
}

void CullingPlugin::UpdateCullingMap(const char *pMapName)
{
	strncpy(currentMapName, pMapName, sizeof(currentMapName) - 1);
	currentMapName[sizeof(currentMapName) - 1] = '\0';

	CGlobalVars *globals = GetGameGlobals();
	int tickRate = 64; // CS2 default tick rate
	if (globals)
	{
		float interval = globals->m_flIntervalPerTick;
		if (interval > 0)
			tickRate = (int)(1.0f / interval + 0.5f);
	}

	cullingController.tickRate = tickRate;
	cullingController.maxLookahead = maxLookahead;
	cullingController.BeginPlay(currentMapName);
	META_CONPRINTF("[Culling] Map loaded: %s (tickRate=%d, maxLookahead=%d)\n",
				   currentMapName, tickRate, maxLookahead);
}

void CullingPlugin::OnLevelInit(char const *pMapName,
								char const *pMapEntities,
								char const *pOldLevel,
								char const *pLandmarkName,
								bool loadGame,
								bool background)
{
	META_CONPRINTF("[Culling] OnLevelInit: %s\n", pMapName);
	UpdateCullingMap(pMapName);
}

void CullingPlugin::OnLevelShutdown()
{
	META_CONPRINTF("[Culling] OnLevelShutdown\n");
	memset(teams, 0, sizeof(teams));
	memset(playerActive, 0, sizeof(playerActive));
}

void CullingPlugin::Hook_GameFrame(bool simulating, bool bFirstTick, bool bLastTick)
{
	if (!simulating)
		return;

	// Pass the latest player data to the CullingController and run a tick.
	cullingController.UpdateCharacters(
		teams, eyesFlat, basesFlat, yaws, pitches, speeds);
	cullingController.Tick();
}

void CullingPlugin::Hook_ClientActive(CPlayerSlot slot, bool bLoadGame, const char *pszName, uint64 xuid)
{
	int index = slot.Get() + 1; // 1-based indexing
	if (index > 0 && index <= MAX_CHARACTERS)
	{
		playerActive[index] = true;
		META_CONPRINTF("[Culling] Client active: slot %d (%s)\n", slot.Get(), pszName);
	}
}

void CullingPlugin::Hook_ClientDisconnect(CPlayerSlot slot, ENetworkDisconnectionReason reason, const char *pszName, uint64 xuid, const char *pszNetworkID)
{
	int index = slot.Get() + 1;
	if (index > 0 && index <= MAX_CHARACTERS)
	{
		playerActive[index] = false;
		teams[index] = 0;
		META_CONPRINTF("[Culling] Client disconnected: slot %d (%s)\n", slot.Get(), pszName);
	}
}

void CullingPlugin::Hook_ClientPutInServer(CPlayerSlot slot, char const *pszName, int type, uint64 xuid)
{
	int index = slot.Get() + 1;
	if (index > 0 && index <= MAX_CHARACTERS)
	{
		playerActive[index] = true;
	}
}
