/*! \file WorldLua.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#include RADPCH
#include "../App.h"
#include "../Engine.h"
#include "../Game/Game.h"
#include "World.h"
#include "WorldLuaCommon.h"

extern "C" {
#include <Lua/lualib.h>
#if !defined(LUA_JIT)
#include <Lua/lgc.h>
#include <Lua/lstate.h>
#endif
#if LUA_VERSION_NUM >= 502
#define LUALIB_API LUAMOD_API
#endif
LUALIB_API int luaopen_bit(lua_State *L);
}

#if defined(LUA_EDIT_SUPPORT)
#include <LuaEdit/RemoteDebugger.hpp>
#endif

namespace world {

WorldLua::WorldLua(World *w) : m_world(w) {
}

WorldLua::~WorldLua()  {
#if defined(LUA_EDIT_SUPPORT)
	StopLuaEditRemoteDebugger();
#endif
}

bool WorldLua::Init() {
	m_L = lua::State::Ref(new (ZWorld) lua::State("GameScript"));
	lua_State *L = m_L->L;

#if defined(LUA_EDIT_SUPPORT)
	StartLuaEditRemoteDebugger(6666, L);
#endif

	lua_pushinteger(L, LUA_VERSION_NUM);
	lua_setglobal(L, "LUA_VERSION_NUM");

#if !defined(LUA_JIT) || (LUA_JIT < 200)
	LUART_REGISTER_LUALIB(L, luaopen_base);
#if LUA_VERSION_NUM >= 502
	lua_setglobal(L, "_G");
#endif
	LUART_REGISTER_LUALIB(L, luaopen_math);
#if LUA_VERSION_NUM >= 502
	lua_setglobal(L, LUA_MATHLIBNAME);
#endif
	LUART_REGISTER_LUALIB(L, luaopen_string);
#if LUA_VERSION_NUM >= 502
	lua_setglobal(L, LUA_STRLIBNAME);
#endif
	LUART_REGISTER_LUALIB(L, luaopen_table);
#if LUA_VERSION_NUM >= 502
	lua_setglobal(L, LUA_TABLIBNAME);
#endif
	LUART_REGISTER_LUALIB(L, luaopen_bit);
#if LUA_VERSION_NUM >= 502
	lua_setglobal(L, "bit");
#endif
#if LUA_VERSION_NUM >= 502
	luaopen_coroutine(L);
	lua_setglobal(L, LUA_COLIBNAME);
#endif
#endif
	lua::EnableModuleImport(L, m_impLoader);
	
	lua_createtable(L, kMaxEnts, 0);
	lua_setfield(L, LUA_REGISTRYINDEX, ENTREF_TABLE);
	lua_pushlightuserdata(L, this);
	lua_setfield(L, LUA_REGISTRYINDEX, SELF);

	luaL_Reg worldRegs[] = {
		{ "RequestGenerateSaveGame", lua_World_RequestGenerateSaveGame },
		{ "FindEntityId", lua_World_FindEntityId },
		{ "FindEntityUID", lua_World_FindEntityUID },
		{ "FindEntityClass", lua_World_FindEntityClass },
		{ "FindEntityTargets", lua_World_FindEntityTargets },
		{ "BBoxTouching", lua_World_BBoxTouching },
		{ "LineTrace", lua_World_LineTrace },
		{ "ClipToFloor", lua_World_ClipToFloor },
		{ "CreateFloorMove", lua_World_CreateFloorMove },
		{ "CreateFloorMoveSeq", lua_World_CreateFloorMoveSeq },
		{ "CreateFloorPosition", lua_World_CreateFloorPosition },
		{ "CreateScreenOverlay", lua_World_CreateScreenOverlay },
		{ "PostEvent", lua_World_PostEvent },
		{ "DispatchEvent", lua_World_DispatchEvent },
		{ "FlushEvents", lua_World_FlushEvents },
		{ "GetEvents", lua_World_GetEvents },
		{ "Project", lua_World_Project },
		{ "Unproject", lua_World_Unproject },
		{ "SetUIViewport", lua_World_SetUIViewport },
		{ "SetRootWidget", lua_World_SetRootWidget },
		{ "CreateWidget", lua_World_CreateWidget },
		{ "AddWidgetTickMaterial", lua_World_AddWidgetTickMaterial },
		{ "CreatePostProcessEffect", lua_World_CreatePostProcessEffect },
		{ "FadePostProcessEffect", lua_World_FadePostProcessEffect },
		{ "EnablePostProcessEffect", lua_World_EnablePostProcessEffect },
		{ "SetPostProcessEffectScale", lua_World_SetPostProcessEffectScale },
		{ "PlayCinematic", lua_World_PlayCinematic },
		{ "StopCinematic", lua_World_StopCinematic },
		{ "CinematicTime", lua_World_CinematicTime },
		{ "SetCinematicTime", lua_World_SetCinematicTime },
		{ "SoundFadeMasterVolume", lua_World_SoundFadeMasterVolume },
		{ "SoundFadeChannelVolume", lua_World_SoundFadeChannelVolume },
		{ "SoundChannelVolume", lua_World_SoundChannelVolume },
		{ "SoundPauseChannel", lua_World_SoundPauseChannel },
		{ "SoundChannelIsPaused", lua_World_SoundChannelIsPaused },
		{ "SoundPauseAll", lua_World_SoundPauseAll },
		{ "SoundStopAll", lua_World_SoundStopAll },
		{ "SoundSetDoppler", lua_World_SoundSetDoppler },
		{ "RequestLoad", lua_World_RequestLoad },
		{ "RequestReturn", lua_World_RequestReturn },
		{ "RequestSwitch", lua_World_RequestSwitch },
		{ "RequestUnloadSlot", lua_World_RequestUnloadSlot },
		{ "RequestSwitchLoad", lua_World_RequestSwitchLoad },
		{ "SetGameSpeed", lua_World_SetGameSpeed },
		{ "SetPauseState", lua_World_SetPauseState },
		{ "PlayerPawn", lua_World_PlayerPawn },
		{ "SetPlayerPawn", lua_World_SetPlayerPawn },
		{ "Worldspawn", lua_World_Worldspawn },
		{ "SetWorldspawn", lua_World_SetWorldspawn },
		{ "ViewController", lua_World_ViewController },
		{ "SetViewController", lua_World_SetViewController },
		{ "GameCode", lua_World_GameCode },
		{ "SetGameCode", lua_World_SetGameCode },
		{ "GameTime", lua_World_GameTime },
		{ "SysTime", lua_World_SysTime },
		{ "DeltaTime", lua_World_DeltaTime },
		{ "Viewport", lua_World_Viewport },
		{ "CameraPos", lua_World_CameraPos },
		{ "CameraFarClip", lua_World_CameraFarClip },
		{ "SetCameraFarClip", lua_World_SetCameraFarClip },
		{ "CameraAngles", lua_World_CameraAngles },
		{ "CameraFOV", lua_World_CameraFOV },
		{ "CameraFwd", lua_World_CameraFwd },
		{ "CameraLeft", lua_World_CameraLeft },
		{ "CameraUp", lua_World_CameraUp },
		{ "SetCamera", lua_World_SetCamera},
		{ "SetEnabledGestures", lua_World_SetEnabledGestures },
		{ "FlushInput", lua_World_FlushInput },
		{ "FindFloor", lua_World_FindFloor },
		{ "FloorState", lua_World_FloorState },
		{ "FloorName", lua_World_FloorName },
		{ "SetFloorState", lua_World_SetFloorState },
		{ "WaypointPosition", lua_World_WaypointPosition },
		{ "WaypointFloorPosition", lua_World_WaypointFloorPosition },
		{ "WaypointState", lua_World_WaypointState },
		{ "SetWaypointState", lua_World_SetWaypointState },
		{ "WaypointsForTargetname", lua_World_WaypointsForTargetname },
		{ "WaypointsForUserId", lua_World_WaypointsForUserId },
		{ "NumFloors", lua_World_NumFloors },
		{ "WaypointIds", lua_World_WaypointIds },
		{ "PickWaypoint", lua_World_PickWaypoint },
		{ "DrawCounters", lua_World_DrawCounters },
		{ "QuitGame", lua_World_QuitGame },
		{ "SetDrawUIOnly", lua_World_SetDrawUIOnly },
		{ "DrawUIOnly", lua_World_DrawUIOnly },
		{ "CreateSpriteBatch", lua_World_CreateSpriteBatch },
		{ "CreateDynamicLight", lua_World_CreateDynamicLight },
		{ "MarkTempEntsForGC", lua_World_MarkTempEntsForGC },
		{ 0, 0 }
	};

	luaL_Reg systemCalls[] = {
		{ "Platform", lua_System_Platform },
		{ "SystemLanguage", lua_System_SystemLanguage },
		{ "LaunchURL", lua_System_LaunchURL },
		{ "Fullscreen", lua_System_Fullscreen },
		{ "ScreenSize", lua_System_ScreenSize },
		{ "CreatePrecacheTask", lua_System_CreatePrecacheTask },
		{ "CreateSpawnTask", lua_System_CreateSpawnTask },
		{ "CreateTempSpawnTask", lua_System_CreateTempSpawnTask },
		{ "COut", lua_System_COut },
		{ "ReadMilliseconds", lua_System_ReadMilliseconds },
		{ "CurrentDateAndTime", lua_System_CurrentDateAndTime },
		{ "BuildConfig", lua_System_BuildConfig },
		{ "UTF8To32", lua_System_UTF8To32 },
		{ "UTF32To8", lua_System_UTF32To8 },
		{ "UTF8Compare", lua_System_UTF8Compare },
		{ "UTF8Comparei", lua_System_UTF8Comparei },
		{ "NewHTTPGet", lua_System_NewHTTPGet },
		{ "UIMode", lua_System_UIMode },
		{ "PlayFullscreenMovie", lua_System_PlayFullscreenMovie },
		{ 0, 0 }
	};

	luaL_Reg gameNetworkCalls[] = {
		{ "Create", lua_gnCreate },
		{ "AuthenticateLocalPlayer", lua_gnAuthenticateLocalPlayer },
		{ "LocalPlayerId", lua_gnLocalPlayerId },
		{ "SendScore", lua_gnSendScore },
		{ "SendAchievement", lua_gnSendAchievement },
		{ "ShowLeaderboard", lua_gnShowLeaderboard },
		{ "ShowAchievements", lua_gnShowAchievements },
		{ "LogEvent", lua_gnLogEvent },
		{ "EndTimedEvent", lua_gnEndTimedEvent },
		{ "LogError", lua_gnLogError },
		{ "SessionReportOnAppClose", lua_gnSessionReportOnAppClose },
		{ "SetSessionReportOnAppClose", lua_gnSetSessionReportOnAppClose },
		{ "SessionReportOnAppPause", lua_gnSessionReportOnAppPause },
		{ "SetSessionReportOnAppPause", lua_gnSetSessionReportOnAppPause },
		{ 0, 0 }
	};

	luaL_Reg storeCalls[] = {
		{ "Create", lua_StoreCreate },
		{ "Enabled", lua_StoreEnabled },
		{ "AppGUID", lua_StoreAppGUID },
		{ "RestoreProducts", lua_StoreRestoreProducts },
		{ "RequestProductInfo", lua_StoreRequestProductInfo },
		{ "CreatePaymentRequest", lua_StoreCreatePaymentRequest },
		{ "RequestValidateApplication", lua_StoreRequestValidateApplication },
		{ "RequestValidateProducts", lua_StoreRequestValidateProducts },
		{ 0, 0 }
	};

	luaL_Reg cvarCalls[] = {
		{ "CVar", lua_CVar },
		{ "CVarString", lua_CVarString },
		{ "CVarInt", lua_CVarInt },
		{ "CVarBool", lua_CVarBool },
		{ "CVarFloat", lua_CVarFloat },
		{ "CVarFunc", lua_CVarFunc },
		{ 0, 0 }
	};

	lua::RegisterGlobals(L, "World", worldRegs);
	lua::RegisterGlobals(L, "System", systemCalls);
	lua::RegisterGlobals(L, "GameNetwork", gameNetworkCalls);
	lua::RegisterGlobals(L, "Store", storeCalls);
	lua::RegisterGlobals(L, 0, cvarCalls);

	// setup persistence tables.

	Persistence::KeyValue::Map *keys = App::Get()->engine->sys->globals->keys;
	lua_createtable(L, 0, 3);
	PushKeysTable(L, *keys);
	lua_setfield(L, -2, "keys");
	lua_pushcfunction(L, lua_System_SaveGlobals);
	lua_setfield(L, -2, "Save");
	lua_pushcfunction(L, lua_System_LoadGlobals);
	lua_setfield(L, -2, "Load");
	lua_setglobal(L, "Globals");

	keys = m_world->game->session->keys;
	lua_createtable(L, 0, 3);
	PushKeysTable(L, *keys);
	lua_setfield(L, -2, "keys");
	lua_pushcfunction(L, lua_System_SaveSession);
	lua_setfield(L, -2, "Save");
	lua_pushcfunction(L, lua_System_LoadSession);
	lua_setfield(L, -2, "Load");
	lua_setglobal(L, "Session");

	keys = m_world->game->saveGame->keys;
	lua_createtable(L, 0, 12);
	PushKeysTable(L, *keys);
	lua_setfield(L, -2, "keys");
	lua_pushcfunction(L, lua_System_CreateSaveGame);
	lua_setfield(L, -2, "Create");
	lua_pushcfunction(L, lua_System_LoadSavedGame);
	lua_setfield(L, -2, "LoadSavedGame");
	lua_pushcfunction(L, lua_System_LoadSaveTable);
	lua_setfield(L, -2, "Load");
	lua_pushcfunction(L, lua_System_SaveGame);
	lua_setfield(L, -2, "Save");
	lua_pushcfunction(L, lua_System_NumSavedGameConflicts);
	lua_setfield(L, -2, "NumConflicts");
	lua_pushcfunction(L, lua_System_LoadSavedGameConflict);
	lua_setfield(L, -2, "LoadConflict");
	lua_pushcfunction(L, lua_System_ResolveSavedGameConflict);
	lua_setfield(L, -2, "ResolveConflict");
	lua_pushcfunction(L, lua_System_EnableCloudStorage);
	lua_setfield(L, -2, "EnableCloudStorage");
	lua_pushcfunction(L, lua_System_CloudFileStatus);
	lua_setfield(L, -2, "CloudFileStatus");
	lua_pushcfunction(L, lua_System_StartDownloadingLatestSaveVersion);
	lua_setfield(L, -2, "StartDownloadingLatestVersion");
	lua_pushcfunction(L, lua_System_CloudStorageAvailable);
	lua_setfield(L, -2, "CloudStorageAvailable");
	lua_setglobal(L, "SaveGame");

	if (!lua::ImportModule(L, "Imports"))
		return false;

	return true;
}

Entity::Ref WorldLua::CreateEntity(const Keys &keys) {
	const char *classname = keys.StringForKey("classname", 0);
	if (!classname)
		return Entity::Ref();

	lua_getglobal(L, classname);
	bool hasClass = lua_type(L, -1) == LUA_TTABLE;
	lua_pop(L, 1);

	if (hasClass)
		return Entity::LuaCreate(classname);
	
	return Entity::LuaCreate(classname);
}

bool WorldLua::PushGlobalCall(const char *name) {
	return PushGlobalCall(L, name);
}

bool WorldLua::PushGlobalCall(lua_State *L, const char *name) {
#if LUA_VERSION_NUM >= 502
	lua_pushglobaltable(L);
	bool r = lua::GetFieldExt(L, -1, name);
	if (r) {
		lua_remove(L, -2);
	} else {
		lua_pop(L, 1);
	}
#else
	bool r = lua::GetFieldExt(L, LUA_GLOBALSINDEX, name);
#endif
	return r;
}

bool WorldLua::Call(const char *context, int nargs, int nresults, int errfunc) {
	return Call(L, context, nargs, nresults, errfunc);
}

bool WorldLua::Call(lua_State *L, const char *context, int nargs, int nresults, int errfunc) {
	if (lua_pcall(L, nargs, nresults, errfunc)) {
		COut(C_Error) << "ScriptError(" << context << "): " << lua_tostring(L, -1) << std::endl;
		lua_pop(L, 1);
		return false;
	}

	return true;
}

bool WorldLua::CreateEntity(Entity &ent, int id, int uid, const char *classname) {
	lua_State *L = m_L->L;
	lua_getfield(L, LUA_REGISTRYINDEX, ENTREF_TABLE);	
	lua_pushinteger(L, id);

	if (!PushGlobalCall("World.CreateEntity")) {
		lua_pop(L, 2);
		return false;
	}

	if (!classname)
		classname = "Entity";

	// call Classname:New()
	{
		char path[256];
		string::cpy(path, classname);
		strcat(path, ".New");
		if (!PushGlobalCall(path)) {
			lua_pop(L, 3);
			return false;
		}
		// locate self parameter
		lua_getglobal(L, classname);
		if (!Call("WorldLua::CreateEntity()", 1, 1, 0)) {
			lua_pop(L, 2);
			return false;
		}
	}

	ent.PushCallTable(L);

	lua_pushinteger(L, id);
	lua_pushinteger(L, uid);
	lua_pushlightuserdata(L, &ent);
	
	if (!Call("World.CreateEntity", 4, 1, 0)) {
		lua_pop(L, 2);
		return false;
	}

	lua_settable(L, -3);
	lua_pop(L, 1);

	return true;
}

void WorldLua::PostSpawn() {
	lua_gc(m_L->L, LUA_GCCOLLECT, 0);
#if !defined(LUA_JIT)
	lua_gc(m_L->L, LUA_GCSTOP, 0);
#endif
	lua::State::CompactPools();
}

void WorldLua::DeleteEntId(Entity &ent) {
	lua_State *L = m_L->L;
	lua_getfield(L, LUA_REGISTRYINDEX, ENTREF_TABLE);
	lua_pushinteger(L, ent.m_id);
	lua_pushnil(L);
	lua_settable(L, -3);
	lua_pop(L, -1);
}

void WorldLua::PushEntityFrame(lua_State *L, Entity &ent) {
	lua_getfield(L, LUA_REGISTRYINDEX, ENTREF_TABLE);
	lua_pushinteger(L, ent.m_id);
	lua_gettable(L, -2);
	lua_remove(L, -2);
}

void WorldLua::PushEntityFrame(Entity &ent) {
	PushEntityFrame(m_L->L, ent);
}

bool WorldLua::PushEntityCall(Entity &ent, const char *name) {
	return PushEntityCall(m_L->L, ent, name);
}

bool WorldLua::PushEntityCall(lua_State *L, Entity &ent, const char *name) {
	PushEntityFrame(L, ent);

	lua_getfield(L, -1, name);
	if (lua_type(L, -1) != LUA_TFUNCTION) {
		lua_pop(L, 2);
		return false;
	}

	lua_pushvalue(L, -2); // move ent frame as call parameter
	lua_remove(L, -3); // remove ent frame
	return true;
}

bool WorldLua::CoSpawn(Entity &ent, const Keys &keys) {
	if (!PushGlobalCall("World.CoSpawn"))
		return false;
	PushEntityFrame(ent);
	PushKeysTable(keys);
	return Call("WorldLua::CoSpawn()", 2, 0, 0);
}

bool WorldLua::CoPostSpawn(Entity &ent) {
	if (!PushGlobalCall("World.CoPostSpawn"))
		return false;
	PushEntityFrame(ent);
	return Call("WorldLua::CoPostSpawn()", 1, 0, 0);
}

bool WorldLua::CoThink(Entity &ent) {
	if (!PushGlobalCall("World.CoThink"))
		return false;
	PushEntityFrame(ent);
	return Call("WorldLua::CoThink", 1, 0, 0);
}

bool WorldLua::RunCo(Entity &ent, bool &complete) {
	lua_State *L = m_L->L;

	if (!PushGlobalCall("World.RunCo"))
		return false;
	
	PushEntityFrame(ent);
	
	if (Call("WorldLua::RunCo()", 1, 1, 0)) { 
		// returns: 0 for pending, 1 for complete, 2 for error
		if (lua_type(L, -1) != LUA_TNUMBER) {
			COut(C_Error) << "WorldLua::RunCo() no value returned from World.RunCo in lua script!" << std::endl;
			return false;
		}

		int r = (int)luaL_checkinteger(L, -1);
		lua_pop(L, 1);
		complete = (r==2||r==1); // error or success
		return r != 2; // 2 is error code
	}

	complete = true;
	return false;
}

void WorldLua::PushKeysTable(const Keys &keys) {
	PushKeysTable(m_L->L, keys);
}

void WorldLua::PushKeysTable(lua_State *L, const Keys &keys) {
	lua_createtable(L, 0, (int)keys.pairs.size());
	for (Keys::Pairs::const_iterator it = keys.pairs.begin(); it != keys.pairs.end(); ++it) {
		lua_pushstring(L, it->first.c_str);
		lua_pushstring(L, it->second.c_str);
		lua_settable(L, -3);
	}
}

bool WorldLua::ParseKeysTable(Keys &keys, int index, bool luaError) {
	return ParseKeysTable(m_L->L, keys, index, luaError);
}

bool WorldLua::ParseKeysTable(lua_State *L, Keys &keys, int index, bool luaError) {
	if (luaError)
		luaL_checktype(L, index, LUA_TTABLE);
	else if (lua_type(L, index) != LUA_TTABLE)
		return false;

	keys.pairs.clear();

	lua_checkstack(L, 3);
	lua_pushnil(L);
	while (lua_next(L, (index<0) ? (index-1) : index) != 0) {
		const char *key = lua_tolstring(L, -2, 0);
		
		if (!key) {
			if (luaError) {
				luaL_checktype(L, -1, LUA_TSTRING);
				luaL_checktype(L, -2, LUA_TSTRING);
			}
			lua_pop(L, 2);
			return false;
		}

		if (!lua_isnil(L, -1)) { // deleted key.
			const char *val = lua_tolstring(L, -1, 0);
			keys.pairs[String(key)] = String(val);
		}
		lua_pop(L, 1);
	}

	return true;
}

void WorldLua::PushKeysTable(const Persistence::KeyValue::Map &keys) {
	PushKeysTable(m_L->L, keys);
}

void WorldLua::PushKeysTable(lua_State *L, const Persistence::KeyValue::Map &keys) {
	lua_createtable(L, 0, (int)keys.size());
	for (Persistence::KeyValue::Map::const_iterator it = keys.begin(); it != keys.end(); ++it) {
		lua_pushstring(L, it->first.c_str);
		if (it->second.mVal) {
			PushKeysTable(L, *it->second.mVal);
		} else {
			lua_pushstring(L, it->second.sVal.c_str);
		}
		lua_settable(L, -3);
	}
}

bool WorldLua::ParseKeysTable(Persistence::KeyValue::Map &keys, int index, bool luaError) {
	return ParseKeysTable(m_L->L, keys, index, luaError);
}

bool WorldLua::ParseKeysTable(lua_State *L, Persistence::KeyValue::Map &keys, int index, bool luaError) {
	if (luaError) {
		luaL_checktype(L, index, LUA_TTABLE);
	} else if(lua_type(L, index) != LUA_TTABLE) {
		return false;
	}

	lua_pushnil(L);
	while (lua_next(L, (index < 0) ? (index-1) : index) != 0) {
		if (luaError) {
			luaL_checktype(L, -2, LUA_TSTRING);
		} else if (lua_type(L, -2) != LUA_TSTRING) {
			lua_pop(L, 2);
			return false;
		}

		const char *name = lua_tostring(L, -2);
		
		if (lua_isnil(L, -1)) {
			// deleted key.
			keys.erase(String(name));
			lua_pop(L, 1);
			continue;
		}

		// avoid deep copy constructor.
		Persistence::KeyValue &kv = keys[String(name)];

		if (lua_type(L, -1) == LUA_TTABLE) {
			if (!kv.mVal)
				kv.mVal = new (ZWorld) Persistence::KeyValue::Map();
			kv.sVal.Clear();
			if (!ParseKeysTable(L, *kv.mVal, -1, luaError))
				return false;
		} else {
			if (luaError) {
				luaL_checktype(L, -1, LUA_TSTRING);
			} else if (lua_type(L, -1) != LUA_TSTRING) {
				lua_pop(L, 2);
				return false;
			}
			kv.sVal = lua_tostring(L, -1);
			if (kv.mVal) {
				delete kv.mVal;
				kv.mVal = 0;
			}
		}

		lua_pop(L, 1);
	}

	return true;
}

Entity *WorldLua::EntFramePtr(int index, bool luaError) {
	return EntFramePtr(L, index, luaError);
}

Entity *WorldLua::EntFramePtr(lua_State *L, int index, bool luaError) {
	if (!lua::GetFieldExt(L, index, "sys.ptr")) {
		if (luaError)
			luaL_typerror(L, index, "Entity Frame Ptr");
		return 0;
	}
	
	if (luaError)
		luaL_checktype(L, -1, LUA_TLIGHTUSERDATA);

	void *p = lua_touserdata(L, -1);
	lua_pop(L, 1);
	return (Entity*)p;
}

void WorldLua::Tick(float dt) {
	GarbageCollect();
}

void WorldLua::SaveState() {
	if (PushGlobalCall("World.SaveGameState"))
		Call("World.SaveGameState", 0, 0, 0);
}

void WorldLua::GarbageCollect() {
#if !defined(LUA_JIT)
	// lua gc
	enum { MaxGCTicks = 3 };

	lua_State *L = m_L->L;

	lua_gc(L, LUA_GCRESTART, 0);
	lua_gc(L, LUA_GCSETSTEPMUL, 10);
	lua_lock(L);

	xtime::TimeVal start = xtime::ReadMilliseconds();
	xtime::TimeVal delta;
	int numSteps = 0;
	do {
		++numSteps;
		luaC_step(L);
		delta = xtime::ReadMilliseconds()-start;
	} while (delta < MaxGCTicks);

	lua_unlock(L);
	lua_gc(L, LUA_GCSTOP, 0);

	if (delta > MaxGCTicks+1)
		COut(C_Debug) << "GC cycle overflow (" << delta << "/" << numSteps << ")" << std::endl;
#endif
}

bool WorldLua::PostSpawn(Entity &ent) {
	if (!PushEntityCall(ent, "PostSpawn"))
		return true; // not an error to have no PostSpawn
	return Call("PostSpawn", 1, 0, 0);
}

#if defined(RAD_TARGET_GOLDEN)
#include "../../../../Source/Scripts/CompiledScripts.cpp"
#else
lua::SrcBuffer::Ref WorldLua::ImportLoader::Load(lua_State *L, const char *name) {
	String path(CStr("@r:/Source/Scripts/"));

	path += name;
	path += ".lua";

	file::MMapping::Ref mm = App::Get()->engine->sys->files->MapFile(path.c_str, ZWorld);
	if (!mm)
		return lua::SrcBuffer::Ref();

	return lua::SrcBuffer::Ref(new lua::FileSrcBuffer((CStr(name) + ".lua").c_str, mm));
}
#endif

} // world
