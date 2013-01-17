// WorldLua_GameNetwork.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "../App.h"
#include "../Engine.h"
#include "../Game/Game.h"
#include "World.h"
#include "WorldLuaCommon.h"

namespace world {

void WorldLua::OnLocalPlayerAuthenticated(gn::NetResult r) {
	if (r != gn::NR_Success)
		return;
	RAD_ASSERT(m_world->game->gameNetwork);

	if (!PushGlobalCall("GameNetwork.OnLocalPlayerAuthenticated"))
		return;
	lua_pushboolean(m_L->L, (m_world->game->gameNetwork->localPlayer->authenticated) ? 1 : 0);
	Call("GameNetwork.OnLocalPlayerAuthenticated", 1, 0, 0);
}

void WorldLua::OnShowLeaderboard(bool show) {
	if (!PushGlobalCall("GameNetwork.OnShowLeaderboard"))
		return;
	lua_pushboolean(m_L->L, show ? 1 : 0);
	Call("GameNetwork.OnShowLeaderboard", 1, 0, 0);
}

void WorldLua::OnShowAchievements(bool show) {
	if (!PushGlobalCall("GameNetwork.OnShowAchievements"))
		return;
	lua_pushboolean(m_L->L, show ? 1 : 0);
	Call("GameNetwork.OnShowAchievements", 1, 0, 0);
}

int WorldLua::lua_gnCreate(lua_State *L) {
	LOAD_SELF

	lua_pushboolean(L, self->m_world->game->CreateGameNetwork() ? 1 : 0);
	return 1;
}

int WorldLua::lua_gnAuthenticateLocalPlayer(lua_State *L) {
	LOAD_SELF

	gn::GameNetwork *network = self->m_world->game->gameNetwork;
	if (network)
		network->AuthenticateLocalPlayer();

	return 0;
}

int WorldLua::lua_gnLocalPlayerId(lua_State *L) {
	LOAD_SELF

	gn::GameNetwork *network = self->m_world->game->gameNetwork;
	if (network) {
		const char *id = network->localPlayer->id;
		if (id) {
			lua_pushstring(L, id);
			return 1;
		}
	}

	return 0;
}

int WorldLua::lua_gnSendScore(lua_State *L) {
	LOAD_SELF

	gn::GameNetwork *network = self->m_world->game->gameNetwork;
	if (network && network->localPlayer->authenticated) {
		const char *leaderboardId = luaL_checkstring(L, 1);
		int score = (int)luaL_checknumber(L, 2);
		network->SendScore(leaderboardId, score);
	}

	return 0;
}

int WorldLua::lua_gnSendAchievement(lua_State *L) {
	LOAD_SELF

	gn::GameNetwork *network = self->m_world->game->gameNetwork;
	if (network && network->localPlayer->authenticated) {
		const char *achievementId = luaL_checkstring(L, 1);
		float percent = (lua_gettop(L) > 1) ? (float)luaL_checknumber(L, 2) : 100.f;
		network->SendAchievement(achievementId, percent);
	}

	return 0;
}

int WorldLua::lua_gnShowLeaderboard(lua_State *L) {
	LOAD_SELF

	gn::GameNetwork *network = self->m_world->game->gameNetwork;
	if (network) {
		const char *leaderboardId = luaL_checkstring(L, 1);
		network->ShowLeaderboard(leaderboardId);
	}

	return 0;
}
	
int WorldLua::lua_gnShowAchievements(lua_State *L) {
	LOAD_SELF
	
	gn::GameNetwork *network = self->m_world->game->gameNetwork;
	if (network)
		network->ShowAchievements();
	
	return 0;
}

int WorldLua::lua_gnLogEvent(lua_State *L) {
	LOAD_SELF

	gn::GameNetwork *network = self->m_world->game->gameNetwork;
	if (network) {
		const char *eventName = luaL_checkstring(L, 1);
		Keys *keys = 0;
		Keys _keys;

		if (lua_gettop(L) > 1) {
			ParseKeysTable(L, _keys, 2, true);
			keys = &_keys;
		}

		bool timed = (lua_gettop(L) > 2) ? (lua_toboolean(L, 3) ? true : false) : false;

		network->LogEvent(eventName, keys, timed);
	}

	return 0;
}

int WorldLua::lua_gnEndTimedEvent(lua_State *L) {
	LOAD_SELF

	gn::GameNetwork *network = self->m_world->game->gameNetwork;
	if (network) {
		const char *eventName = luaL_checkstring(L, 1);
		
		Keys *keys = 0;
		Keys _keys;

		if (lua_gettop(L) > 1) {
			ParseKeysTable(L, _keys, 2, true);
			keys = &_keys;
		}

		network->EndTimedEvent(eventName, keys);
	}

	return 0;
}

int WorldLua::lua_gnLogError(lua_State *L) {
	LOAD_SELF

	gn::GameNetwork *network = self->m_world->game->gameNetwork;
	if (network) {
		const char *eventName = luaL_checkstring(L, 1);
		const char *message = luaL_checkstring(L, 2);
		network->LogError(eventName, message);
	}

	return 0;
}

int WorldLua::lua_gnSessionReportOnAppClose(lua_State *L) {
	LOAD_SELF

	gn::GameNetwork *network = self->m_world->game->gameNetwork;
	if (network) {
		lua_pushboolean(L, network->sessionReportOnAppClose ? 1 : 0);
	} else {
		lua_pushboolean(L, 0);
	}

	return 1;
}

int WorldLua::lua_gnSetSessionReportOnAppClose(lua_State *L) {
	LOAD_SELF

	gn::GameNetwork *network = self->m_world->game->gameNetwork;
	if (network)
		network->sessionReportOnAppClose = lua_toboolean(L, 1) ? true : false;

	return 0;
}

int WorldLua::lua_gnSessionReportOnAppPause(lua_State *L) {
	LOAD_SELF

	gn::GameNetwork *network = self->m_world->game->gameNetwork;
	if (network) {
		lua_pushboolean(L, network->sessionReportOnAppPause ? 1 : 0);
	} else {
		lua_pushboolean(L, 0);
	}

	return 1;
}

int WorldLua::lua_gnSetSessionReportOnAppPause(lua_State *L) {
	LOAD_SELF

	gn::GameNetwork *network = self->m_world->game->gameNetwork;
	if (network)
		network->sessionReportOnAppPause = lua_toboolean(L, 1) ? true : false;

	return 0;
}

} // world
