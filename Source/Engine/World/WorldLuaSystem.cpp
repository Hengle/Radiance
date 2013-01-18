// WorldLua.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "../App.h"
#include "../Engine.h"
#include "../Game/Game.h"
#include "World.h"
#include "Lua/T_Precache.h"
#include "Lua/T_Spawn.h"
#include "Lua/T_TempSpawn.h"
#include "../Persistence.h"
#include "../StringTable.h"
#include "WorldLuaCommon.h"

namespace world {

int WorldLua::lua_System_Platform(lua_State *L) {
	enum {
		PlatMac,
		PlatWin,
		PlatIPad,
		PlatIPhone,
		PlatXBox360,
		PlatPS3
	};

#if defined(RAD_OPT_OSX)
	lua_pushinteger(L, PlatMac);
#elif defined(RAD_OPT_WIN)
	lua_pushinteger(L, PlatWin);
#elif defined(RAD_OPT_IOS)
	lua_pushinteger(L, (App::Get()->deviceFamily == plat::kDeviceFamily_iPhone) ? PlatIPhone : PlatIPad);
#else
	#error RAD_ERROR_UNSUP_PLAT
#endif

	return 1;
}

int WorldLua::lua_System_ScreenSize(lua_State *L) {
	LOAD_SELF

	int vpx, vpy, vpw, vph;
	self->m_world->game->Viewport(vpx, vpy, vpw, vph);

	lua_createtable(L, 0, 2);
	lua_pushinteger(L, vpw);
	lua_setfield(L, -2, "width");
	lua_pushinteger(L, vph);
	lua_setfield(L, -2, "height");

	r::VidMode m(vpw, vph, 32, 60, true);
	if (m.Is4x3()) {
		lua_pushstring(L, "4x3");
	} else if (m.Is16x9()) {
		lua_pushstring(L, "16x9");
	} else {
		lua_pushstring(L, "16x10");
	}
	lua_setfield(L, -2, "aspect");

	return 1;
}

int WorldLua::lua_System_SystemLanguage(lua_State *L) {
	lua_pushinteger(L, (int)App::Get()->langId.get());
	return 1;
}

int WorldLua::lua_System_GetLangString(lua_State *L) {
	LOAD_SELF

	const StringTable *stringTable = self->m_world->game->stringTable;

	if (stringTable) {
		const char *id = luaL_checkstring(L, 1);
		int lang = (int)luaL_checkinteger(L, 2);
		if (lang < StringTable::LangId_EN || lang >= StringTable::LangId_MAX)
			luaL_error(L, "System.GetLangString() invalid language id %d", lang);
		const String *s = stringTable->Find(id, (StringTable::LangId)lang);
		if (s) {
			lua_pushstring(L, s->c_str);
			return 1;
		}
	}

	return 0;
}

int WorldLua::lua_System_LaunchURL(lua_State *L) {
	const char *sz = luaL_checkstring(L, 1);
	App::Get()->LaunchURL(sz);
	return 0;
}

int WorldLua::lua_System_Fullscreen(lua_State *L) {
	bool fullscreen = false;
	App *app = App::Get();

	if (app->activeDisplay.get())
		fullscreen = app->activeDisplay->curVidMode->fullscreen;

	lua_pushboolean(L, fullscreen);
	return 1;
}

int WorldLua::lua_System_CreatePrecacheTask(lua_State *L) {
	LOAD_SELF

	Entity *e = EntFramePtr(L, 1, true);

	T_Precache::Ref task = T_Precache::New(
		self->m_world,
		*App::Get()->engine.get(), 
		luaL_checkstring(L, 2), 
		self->m_world->pkgZone,
		lua_toboolean(L, 3) ? true : false,
		(int)lua_tointeger(L, 4)
	);
	
	if (task) {
		if (lua_toboolean(L, 5) == 1) {
			e->QueueScriptTask(boost::static_pointer_cast<Entity::Tickable>(task));
		} else {
			// tick until loaded!
			while (task->Tick(*e, 0.001f, xtime::TimeSlice::Infinite, 0) == TickNext) {}
		}
		task->Push(L);
		return 1;
	}

	return 0;
}

int WorldLua::lua_System_CreateSpawnTask(lua_State *L) {
	LOAD_SELF

	Entity *e = EntFramePtr(L, 1, true);

	Keys keys;
	ParseKeysTable(L, keys, 2, true);
	
	T_Spawn::Ref spawn = T_Spawn::New(self->m_world, keys);
	if (spawn) {
		e->QueueScriptTask(boost::static_pointer_cast<Entity::Tickable>(spawn));
		spawn->Push(L);
		return 1;
	}

	return 0;
}

int WorldLua::lua_System_CreateTempSpawnTask(lua_State *L) {
	LOAD_SELF

	Entity *e = EntFramePtr(L, 1, true);

	Keys keys;
	ParseKeysTable(L, keys, 2, true);
	
	T_TempSpawn::Ref spawn = T_TempSpawn::New(self->m_world, keys);
	if (spawn) {
		e->QueueScriptTask(boost::static_pointer_cast<Entity::Tickable>(spawn));
		spawn->Push(L);
		return 1;
	}

	return 0;
}

 int WorldLua::lua_System_COut(lua_State *L) {
	 int level = (int)luaL_checkinteger(L, 1);
	 const char *string = luaL_checkstring(L, 2);
	 COut(level) << "(Script):" << string << std::flush;
	 return 0;
 }

int WorldLua::lua_System_SaveSession(lua_State *L) {
	LOAD_SELF
	
	Keys x;
	Keys *keys = self->m_world->game->session->keys;
	
	luaL_checktype(L, 1, LUA_TTABLE);
	lua_getfield(L, 1, "keys");

	ParseKeysTable(L, x, -1, true);
	*keys = x;

	lua_pop(L, 1);
	self->m_world->game->session->Save();
	return 0;
}

int WorldLua::lua_System_LoadSession(lua_State *L) {
	LOAD_SELF
	
	luaL_checktype(L, 1, LUA_TTABLE);
	Keys *keys = self->m_world->game->session->keys;
	PushKeysTable(L, *keys);
	lua_setfield(L, 1, "keys");
	return 0;
}

int WorldLua::lua_System_SaveGlobals(lua_State *L) {
	Keys x;
	Keys *keys = App::Get()->engine->sys->globals->keys;
	
	luaL_checktype(L, 1, LUA_TTABLE);
	lua_getfield(L, 1, "keys");

	ParseKeysTable(L, x, -1, true);
	*keys = x;

	lua_pop(L, 1);
	App::Get()->engine->sys->globals->Save();
	return 0;
}

int WorldLua::lua_System_LoadGlobals(lua_State *L) {
	luaL_checktype(L, 1, LUA_TTABLE);
	Keys *keys = App::Get()->engine->sys->globals->keys;
	PushKeysTable(L, *keys);
	lua_setfield(L, 1, "keys");
	return 0;
}

int WorldLua::lua_System_CreateSaveGame(lua_State *L) {
	LOAD_SELF

	luaL_checktype(L, 1, LUA_TTABLE);
	self->m_world->game->CreateSaveGame(luaL_checkstring(L, 2));
	PushKeysTable(L, *self->m_world->game->saveGame->keys.get());
	lua_setfield(L, 1, "keys");
	return 0;
}

int WorldLua::lua_System_LoadSavedGame(lua_State *L) {
	LOAD_SELF

	luaL_checktype(L, 1, LUA_TTABLE);
	self->m_world->game->LoadSavedGame(luaL_checkstring(L, 2));
	PushKeysTable(L, *self->m_world->game->saveGame->keys.get());
	lua_setfield(L, 1, "keys");
	return 0;
}

int WorldLua::lua_System_LoadSaveTable(lua_State *L) {
	LOAD_SELF

	PushKeysTable(L, *self->m_world->game->saveGame->keys.get());
	lua_setfield(L, 1, "keys");
	return 0;
}

int WorldLua::lua_System_SaveGame(lua_State *L) {
	LOAD_SELF

	luaL_checktype(L, 1, LUA_TTABLE);
	lua_getfield(L, 1, "keys");

	Keys x;
	ParseKeysTable(L, x, -1, true);
	lua_pop(L, 1);

	*self->m_world->game->saveGame->keys.get() = x;
	self->m_world->game->SaveGame();
	return 0;
}

int WorldLua::lua_System_NumSavedGameConflicts(lua_State *L) {
	LOAD_SELF

	luaL_checktype(L, 1, LUA_TTABLE);
	lua_pushinteger(L, self->m_world->game->numSavedGameConflicts.get());
	return 1;
}

int WorldLua::lua_System_LoadSavedGameConflict(lua_State *L) {
	LOAD_SELF

	luaL_checktype(L, 1, LUA_TTABLE);
	self->m_world->game->LoadSavedGameConflict((int)luaL_checkinteger(L, 2));
	PushKeysTable(L, *self->m_world->game->saveGame->keys.get());
	lua_setfield(L, 1, "keys");
	return 0;
}

int WorldLua::lua_System_ResolveSavedGameConflict(lua_State *L) {
	LOAD_SELF

	luaL_checktype(L, 1, LUA_TTABLE);
	self->m_world->game->ResolveSavedGameConflict((int)luaL_checkinteger(L, 2));
	return 0;
}

int WorldLua::lua_System_EnableCloudStorage(lua_State *L) {
	LOAD_SELF

	luaL_checktype(L, 1, LUA_TTABLE);
	self->m_world->game->cloudStorage = lua_toboolean(L, 2) ? true : false;
	return 0;
}

int WorldLua::lua_System_CloudFileStatus(lua_State *L) {
	luaL_checktype(L, 1, LUA_TTABLE);
	if (CloudStorage::Enabled()) {
		lua_pushnumber(L, CloudStorage::FileStatus(luaL_checkstring(L, 2)));
	} else {
		lua_pushnumber(L, CloudFile::Ready);
	}
	return 1;
}

int WorldLua::lua_System_StartDownloadingLatestSaveVersion(lua_State *L) {
	luaL_checktype(L, 1, LUA_TTABLE);
	if (CloudStorage::Enabled()) {
		lua_pushboolean(L, CloudStorage::StartDownloadingLatestVersion(luaL_checkstring(L, 2))?1:0);
	} else {
		lua_pushboolean(L, 1);
	}
	return 1;
}

int WorldLua::lua_System_CloudStorageAvailable(lua_State *L) {
	luaL_checktype(L, 1, LUA_TTABLE);
	lua_pushboolean(L, CloudStorage::Enabled() ? 1 : 0);
	return 1;
}

int WorldLua::lua_System_BuildConfig(lua_State *L) {
#if defined(RAD_OPT_SHIP)
	const char *sz = "ship";
#elif defined(RAD_OPT_GOLDEN)
	const char *sz = "golden";
#else
	const char *sz = "tools";
#endif
	lua_pushstring(L, sz);
	return 1;
}

} // world
