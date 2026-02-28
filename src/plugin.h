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

#ifndef _INCLUDE_CULLING_PLUGIN_H_
#define _INCLUDE_CULLING_PLUGIN_H_

#include <ISmmPlugin.h>
#include <igameevents.h>
#include <sh_vector.h>
#include "version_gen.h"
#include "../CornerCulling/CullingController.h"

class CullingPlugin : public ISmmPlugin, public IMetamodListener
{
public:
	bool Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late);
	bool Unload(char *error, size_t maxlen);
	void AllPluginsLoaded();

public: // hooks
	void OnLevelInit(char const *pMapName,
					 char const *pMapEntities,
					 char const *pOldLevel,
					 char const *pLandmarkName,
					 bool loadGame,
					 bool background);
	void OnLevelShutdown();
	void Hook_GameFrame(bool simulating, bool bFirstTick, bool bLastTick);
	void Hook_ClientActive(CPlayerSlot slot, bool bLoadGame, const char *pszName, uint64 xuid);
	void Hook_ClientDisconnect(CPlayerSlot slot, ENetworkDisconnectionReason reason, const char *pszName, uint64 xuid, const char *pszNetworkID);
	void Hook_ClientPutInServer(CPlayerSlot slot, char const *pszName, int type, uint64 xuid);

public:
	const char *GetAuthor() { return PLUGIN_AUTHOR; }
	const char *GetName() { return PLUGIN_DISPLAY_NAME; }
	const char *GetDescription() { return PLUGIN_DESCRIPTION; }
	const char *GetURL() { return PLUGIN_URL; }
	const char *GetLicense() { return PLUGIN_LICENSE; }
	const char *GetVersion() { return PLUGIN_FULL_VERSION; }
	const char *GetDate() { return __DATE__; }
	const char *GetLogTag() { return PLUGIN_LOGTAG; }

private:
	void UpdateCullingMap(const char *pMapName);
};

extern CullingPlugin g_CullingPlugin;

PLUGIN_GLOBALVARS();

#endif // _INCLUDE_CULLING_PLUGIN_H_
