// WorldLua.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "../App.h"
#include "../Engine.h"
#include "World.h"
#include "WorldDraw.h"
#include "ScreenOverlay.h"
#include "PostProcess.h"
#include "Lua/D_ScreenOverlay.h"
#include "../Game/Game.h"
#include "../Renderer/Renderer.h"
#include "../UI/UIWidget.h"
#include "../UI/UIMatWidget.h"
#include "../UI/UITextLabel.h"
#include "../Assets/MaterialParser.h"	
#include "../Renderer/Material.h"
#include "Lua/T_Precache.h"
#include "Lua/T_Spawn.h"
#include "Lua/T_TempSpawn.h"
#include "Lua/D_Asset.h"
#include "Lua/D_SkModel.h"
#include "Lua/D_Sound.h"
#include "Lua/D_Mesh.h"
#include "../Sound/Sound.h"
#include "../Persistence.h"
#include "../StringTable.h"

extern "C" {
#include <Lua/lualib.h>
#include <Lua/lgc.h>
#include <Lua/lstate.h>
#if LUA_VERSION_NUM >= 502
#define LUALIB_API LUAMOD_API
#endif
LUALIB_API int luaopen_bit(lua_State *L);
}

#if defined(LUA_EDIT_SUPPORT)
#include <LuaEdit/RemoteDebugger.hpp>
#endif

#define SELF "@world"
#define ENTREF_TABLE "@ents"
#define SPRINTF_ENTREGS "%d"

#if defined(RAD_OPT_IOS)
bool __IOS_IPhone();
#endif

namespace world {

namespace {

class FileSrcBuffer : public lua::SrcBuffer {
public:
	FileSrcBuffer(const char *name, const file::MMapping::Ref &mm) : m_name(name), m_mm(mm) { 
		m_name += ".lua";
	}

protected:
	RAD_DECLARE_GET(ptr, const void *) { return m_mm->data; }
	RAD_DECLARE_GET(size, AddrSize) { return m_mm->size; }
	RAD_DECLARE_GET(name, const char *) { return m_name.c_str; }

	file::MMapping::Ref m_mm;
	String m_name;
};

}

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

	luaopen_base(L);
#if LUA_VERSION_NUM >= 502
	lua_setglobal(L, "_G");
#endif
	luaopen_math(L);
#if LUA_VERSION_NUM >= 502
	lua_setglobal(L, LUA_MATHLIBNAME);
#endif
	luaopen_string(L);
#if LUA_VERSION_NUM >= 502
	lua_setglobal(L, LUA_STRLIBNAME);
#endif
	luaopen_table(L);
#if LUA_VERSION_NUM >= 502
	lua_setglobal(L, LUA_TABLIBNAME);
#endif
	luaopen_bit(L);
#if LUA_VERSION_NUM >= 502
	lua_setglobal(L, "bit");
#endif
#if LUA_VERSION_NUM >= 502
	luaopen_coroutine(L);
	lua_setglobal(L, LUA_COLIBNAME);
#endif
	lua::EnableModuleImport(L, m_impLoader);
	
	lua_createtable(L, kMaxEnts, 0);
	lua_setfield(L, LUA_REGISTRYINDEX, ENTREF_TABLE);
	lua_pushlightuserdata(L, this);
	lua_setfield(L, LUA_REGISTRYINDEX, SELF);

	luaL_Reg worldRegs [] = {
		{ "FindEntityId", lua_World_FindEntityId },
		{ "FindEntityClass", lua_World_FindEntityClass },
		{ "FindEntityTargets", lua_World_FindEntityTargets },
		{ "BBoxTouching", lua_World_BBoxTouching },
		{ "LineTrace", lua_World_LineTrace },
		{ "ClipToFloor", lua_World_ClipToFloor },
		{ "CreateFloorMove", lua_World_CreateFloorMove },
		{ "CreateScreenOverlay", lua_World_CreateScreenOverlay },
		{ "PostEvent", lua_World_PostEvent },
		{ "DispatchEvent", lua_World_DispatchEvent },
		{ "Project", lua_World_Project },
		{ "Unproject", lua_World_Unproject },
		{ "SetUIViewport", lua_World_SetUIViewport },
		{ "SetRootWidget", lua_World_SetRootWidget },
		{ "CreateWidget", lua_World_CreateWidget },
		{ "AddWidgetTickMaterial", lua_World_AddWidgetTickMaterial },
		{ "CreatePostProcessEffect", lua_World_CreatePostProcessEffect },
		{ "FadePostProcessEffect", lua_World_FadePostProcessEffect },
		{ "EnablePostProcessEffect", lua_World_EnablePostProcessEffect },
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
		{ "SwapMaterial", lua_World_SwapMaterial },
		{ "PlayerPawn", lua_World_PlayerPawn },
		{ "SetPlayerPawn", lua_World_SetPlayerPawn },
		{ "Worldspawn", lua_World_Worldspawn },
		{ "SetWorldspawn", lua_World_SetWorldspawn },
		{ "ViewController", lua_World_ViewController },
		{ "SetViewController", lua_World_SetViewController },
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
		{ "SetEnabledGestures", lua_World_SetEnabledGestures },
		{ "FlushInput", lua_World_FlushInput },
		{ "DrawCounters", lua_World_DrawCounters },
		{ "EnableWireframe", lua_World_EnableWireframe },
		{ "QuitGame", lua_World_QuitGame },
		{ 0, 0 }
	};

	luaL_Reg systemCalls [] = {
		{ "Platform", lua_System_Platform },
		{ "SystemLanguage", lua_System_SystemLanguage },
		{ "GetLangString", lua_System_GetLangString },
		{ "LaunchURL", lua_System_LaunchURL },
		{ "Fullscreen", lua_System_Fullscreen },
		{ "CreatePrecacheTask", lua_System_CreatePrecacheTask },
		{ "CreateSpawnTask", lua_System_CreateSpawnTask },
		{ "CreateTempSpawnTask", lua_System_CreateTempSpawnTask },
		{ "COut", lua_System_COut },
		{ "CurrentDateAndTime", lua_System_CurrentDateAndTime },
		{ 0, 0 }
	};

	luaL_Reg gameNetworkCalls [] = {
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
		{ 0, 0}
	};

	lua::RegisterGlobals(L, "World", worldRegs);
	lua::RegisterGlobals(L, "System", systemCalls);
	lua::RegisterGlobals(L, "GameNetwork", gameNetworkCalls);
	
	lua_pop(L, 1);

	// setup persistence tables.

	Keys *keys = App::Get()->engine->sys->globals->keys;
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

	if (!hasClass)
		return Entity::Create(classname);

	return Entity::LuaCreate(classname);
}

bool WorldLua::PushGlobalCall(const char *name) {
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

bool WorldLua::CreateEntity(Entity &ent, int id, const char *classname) {
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
	lua_pushlightuserdata(L, &ent);
	
	if (!Call("World.CreateEntity", 3, 1, 0)) {
		lua_pop(L, 2);
		return false;
	}

	lua_settable(L, -3);
	lua_pop(L, 1);

	return true;
}

bool WorldLua::HandleInputEvent(const InputEvent &e, const TouchState *touch, const InputState &is) {
	if (!PushGlobalCall("World.OnInputEvent"))
		return false;
	lua_State *L = m_L->L;

	lua::Marshal<InputEvent>::Push(L, e, touch);	

	bool r = false;
	if (Call("WorldLua::HandleInputEvent", 1, 1, 0)) {
		r = lua_toboolean(L, -1) ? true : false;
		lua_pop(L, 1);
	}

	return r;
}

bool WorldLua::HandleInputGesture(const InputGesture &g, const TouchState &touch, const InputState &is) {
	if (!PushGlobalCall("World.OnInputGesture"))
		return false;
	lua_State *L = m_L->L;

	lua::Marshal<InputGesture>::Push(L, g, touch);

	bool r = false;
	if (Call("World::HandleInputGesture", 1, 1, 0)) {
		r = lua_toboolean(L, -1) ? true : false;
		lua_pop(L, 1);
	}

	return r;
}

void WorldLua::NotifyBackground() {
	if (!PushGlobalCall("World.NotifyBackground"))
		return;
	Call("World::NotifyBackground", 0, 0, 0);
}

void WorldLua::NotifyResume() {
	if (!PushGlobalCall("World.NotifyResume"))
		return;
	Call("World::NotifyResume", 0, 0, 0);
}

void WorldLua::SaveApplicationState() {
	if (!PushGlobalCall("World.SaveApplicationState"))
		return;
	Call("World::SaveApplicationState", 0, 0, 0);
}

void WorldLua::RestoreApplicationState() {
	if (!PushGlobalCall("World.RestoreApplicationState"))
		return;
	Call("World::RestoreApplicationState", 0, 0, 0);
}

void WorldLua::PostSpawn() {
	lua_gc(m_L->L, LUA_GCCOLLECT, 0);
	lua_gc(m_L->L, LUA_GCSTOP, 0);
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

	lua_checkstack(L, 3);
	lua_pushnil(L);
	while (lua_next(L, (index<0) ? (index-1) : index) != 0) {
		const char *key = lua_tolstring(L, -2, 0);
		const char *val = lua_tolstring(L, -1, 0);

		if (!key || !val) {
			if (luaError) {
				luaL_checktype(L, -1, LUA_TSTRING);
				luaL_checktype(L, -2, LUA_TSTRING);
			}
			lua_pop(L, 2);
			return false;
		}

		keys.pairs[String(key)] = String(val);
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

void WorldLua::GarbageCollect() {
	// lua gc
	enum { MaxGCTicks = 3 };

	lua_State *L = m_L->L;

	lua_gc(L, LUA_GCRESTART, 0);
	lua_gc(L, LUA_GCSETSTEPMUL, 50);
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
}

bool WorldLua::PostSpawn(Entity &ent) {
	if (!PushEntityCall(ent, "PostSpawn"))
		return true; // not an error to have no PostSpawn
	return Call("PostSpawn", 1, 0, 0);
}

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

int WorldLua::lua_System_SystemLanguage(lua_State *L) {
	lua_pushinteger(L, (int)App::Get()->langId.get());
	return 1;
}

int WorldLua::lua_System_GetLangString(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

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
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

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
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

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
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

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

int WorldLua::lua_World_FindEntityId(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	int id = (int)luaL_checkinteger(L, 1);
	Entity::Ref ref = self->m_world->FindEntityId(id);

	if (ref) {
		ref->PushEntityFrame(L);
	} else {
		lua_pushnil(L);
	}

	return 1;
}

int WorldLua::lua_World_FindEntityClass(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	const char *classname = luaL_checkstring(L, 1);
	Entity::Vec ents = self->m_world->FindEntityClass(classname);
	
	if (ents.empty()) {
		lua_pushnil(L);
	} else {
		int c = 1;
		lua_createtable(L, (int)ents.size(), 0);
		for (Entity::Vec::const_iterator it = ents.begin(); it != ents.end(); ++it, ++c) {
			lua_pushinteger(L, c);
			(*it)->PushEntityFrame(L);
			lua_settable(L, -3);
		}
	}

	return 1;
}

int WorldLua::lua_World_FindEntityTargets(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	const char *targetname = luaL_checkstring(L, 1);
	Entity::Vec ents = self->m_world->FindEntityTargets(targetname);
	
	if (ents.empty()) {
		lua_pushnil(L);
	} else {
		int c = 1;
		lua_createtable(L, (int)ents.size(), 0);
		for (Entity::Vec::const_iterator it = ents.begin(); it != ents.end(); ++it, ++c) {
			lua_pushinteger(L, c);
			(*it)->PushEntityFrame(L);
			lua_settable(L, -3);
		}
	}

	return 1;
}

int WorldLua::lua_World_BBoxTouching(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	BBox bbox(
		lua::Marshal<Vec3>::Get(L, 1, true),
		lua::Marshal<Vec3>::Get(L, 2, true)
	);

	int stypes = (int)luaL_checknumber(L, 3);
	Entity::Vec ents = self->m_world->BBoxTouching(bbox, stypes);

	if (ents.empty()) {
		lua_pushnil(L);
	} else {
		int c = 1;
		lua_createtable(L, (int)ents.size(), 0);
		for (Entity::Vec::const_iterator it = ents.begin(); it != ents.end(); ++it, ++c) {
			lua_pushinteger(L, c);
			(*it)->PushEntityFrame(L);
			lua_settable(L, -3);
		}
	}

	return 1;
}

int WorldLua::lua_World_LineTrace(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	Trace t = lua::Marshal<Trace>::Get(L, 1, true);
	lua_pushboolean(L, self->m_world->LineTrace(t) ? 1 : 0);
	return 1;
}

int WorldLua::lua_World_ClipToFloor(lua_State *L) {

	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	Vec3 start = lua::Marshal<Vec3>::Get(L, 1, true);
	Vec3 end = lua::Marshal<Vec3>::Get(L, 2, true);

	FloorPosition p;
	bool r = self->m_world->floors->ClipToFloor(start, end, p);
	lua_pushboolean(L, r ? 1 : 0);
	if (r)
		lua::Marshal<FloorPosition>::Push(L, p);
	return r ? 2 : 1;
}

int WorldLua::lua_World_CreateFloorMove(lua_State *L) {

	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	FloorPosition start = lua::Marshal<FloorPosition>::Get(L, 1, true);
	FloorPosition end   = lua::Marshal<FloorPosition>::Get(L, 2, true);

	FloorMove::Ref move = self->m_world->floors->CreateMove(start, end);
	if (move)
		move->Push(L);
	return move ? 1 : 0;
}

int WorldLua::lua_World_CreateScreenOverlay(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	D_Asset::Ref dAsset = lua::SharedPtr::Get<D_Asset>(L, "Asset", 1, true);
	luaL_argcheck(L, (dAsset->asset->type==asset::AT_Material), 1, "Expected Material!");

	ScreenOverlay::Ref overlay = self->m_world->draw->CreateScreenOverlay(dAsset->asset->id);
	if (!overlay)
		return 0;

	D_ScreenOverlay::Ref dOverlay = D_ScreenOverlay::New(overlay);
	dOverlay->Push(L);
	return 1;
}

int WorldLua::lua_World_PostEvent(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	const char *event = luaL_checkstring(L, 1);
	self->m_world->PostEvent(event);
	return 0;
}

int WorldLua::lua_World_DispatchEvent(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	const char *event = luaL_checkstring(L, 1);
	self->m_world->DispatchEvent(event);
	return 0;
}

int WorldLua::lua_World_Project(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	Vec3 out;
	bool r = self->m_world->draw->Project(
		lua::Marshal<Vec3>::Get(L, 1, true),
		out
	);
	lua::Marshal<Vec3>::Push(L, out);
	lua_pushboolean(L, r ? 1 : 0);
	return 2;
}

int WorldLua::lua_World_Unproject(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	Vec3 p = self->m_world->draw->Unproject(lua::Marshal<Vec3>::Get(L, 1, true));
	lua::Marshal<Vec3>::Push(L, p);
	return 1;
}

int WorldLua::lua_World_SetUIViewport(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	self->m_world->uiRoot->SetSourceViewport(
		(int)luaL_checkinteger(L, 1),
		(int)luaL_checkinteger(L, 2),
		(int)luaL_checkinteger(L, 3),
		(int)luaL_checkinteger(L, 4)
	);

	return 0;
}

int WorldLua::lua_World_SetRootWidget(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	int layer = (int)luaL_checkinteger(L, 1);

	if (lua_isnil(L, 2)) {
		self->m_world->uiRoot->SetRootWidget(layer, ui::Widget::Ref());
	} else {
		self->m_world->uiRoot->SetRootWidget(
			layer, 
			ui::Widget::GetRef<ui::Widget>(L, "Widget", 2, true)
		);
	}
	
	return 0;
}

int WorldLua::lua_World_CreateWidget(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	String type(luaL_checkstring(L, 1));
	luaL_checktype(L, 2, LUA_TTABLE);

	ui::Widget::Ref w;

	if (type == "Widget") {
		w.reset(new (ui::ZUI) ui::Widget());
	} else if(type == "MatWidget") {
		w.reset(new (ui::ZUI) ui::MatWidget());
	} else if(type == "TextLabel") {
		w.reset(new (ui::ZUI) ui::TextLabel());
	} else {
		luaL_argerror(L, 1, "Invalid widget type!");
	}

	lua_pushvalue(L, 2);
	w->Spawn(self->L, L);
	lua_pop(L, 1);
	w->PushFrame(L);
	return 1;
}

int WorldLua::lua_World_AddWidgetTickMaterial(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	D_Asset::Ref dAsset = lua::SharedPtr::Get<D_Asset>(L, "D_Asset", 1, true);
	luaL_argcheck(L, (dAsset->asset->type==asset::AT_Material), 1, "Expected Material!");
	self->m_world->uiRoot->AddTickMaterial(dAsset->asset);
	self->m_world->draw->AddMaterial(dAsset->asset->id);
	return 0;
}

int WorldLua::lua_World_CreatePostProcessEffect(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	int id = (int)luaL_checkinteger(L, 1);
	D_Asset::Ref dAsset = lua::SharedPtr::Get<D_Asset>(L, "D_Asset", 2, true);
	luaL_argcheck(L, (dAsset->asset->type==asset::AT_Material), 2, "Expected Material!");

	PostProcessEffect::Ref effect(new (r::ZRender) PostProcessEffect());
	if (effect->BindMaterial(dAsset->asset))
		self->m_world->draw->AddEffect(id, effect);
	return 0;
}

int WorldLua::lua_World_FadePostProcessEffect(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	int id = (int)luaL_checkinteger(L, 1);
	Vec4 color = lua::Marshal<Vec4>::Get(L, 2, true);
	float time = (float)luaL_checknumber(L, 3);

	const PostProcessEffect::Ref &r = self->m_world->draw->PostFX(id);
	if (r)
		r->FadeTo(color, time);
	return 0;
}

int WorldLua::lua_World_EnablePostProcessEffect(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	int id = (int)luaL_checkinteger(L, 1);
	bool enabled = lua_toboolean(L, 2) ? true : false;

	const PostProcessEffect::Ref &r = self->m_world->draw->PostFX(id);
	if (r)
		r->enabled = enabled;
	return 0;
}

WorldLua::CinematicsNotify::CinematicsNotify(World *world, Entity &entity, int callbackId) :
m_world(world),
m_entity(entity.shared_from_this()),
m_callbackId(callbackId),
m_masked(false) {
}

WorldLua::CinematicsNotify::~CinematicsNotify() {
	if (m_world->destroy) // don't clean ents up (done by world).
		return;

	Entity::Ref entity = m_entity.lock();
	if (entity)
		entity->ReleaseLuaCallback(m_callbackId, lua::InvalidIndex);
}

void WorldLua::CinematicsNotify::PushElements(lua_State *L) {
	lua_pushcfunction(L, lua_SetMasked);
	lua_setfield(L, -2, "SetMasked");
}

void WorldLua::CinematicsNotify::OnTag(const char *str) {
	Entity::Ref entity = m_entity.lock();
	if (!entity)
		return;
	if (!entity->LoadLuaCallback(m_callbackId))
		return;

	lua_State *L = entity->world->lua->L;

	lua_getfield(L, -1, "OnTag");
	if (lua_type(L, -1) != LUA_TFUNCTION) {
		lua_pop(L, 2);
		return;
	}

	entity->PushEntityFrame(L);
	lua_pushstring(L, str);
	entity->world->lua->Call(L, "WorldLua::CinematicsNotify::OnTag", 2, 0, 0);
	lua_pop(L, 1); // pop callback table
}

void WorldLua::CinematicsNotify::OnComplete() {
	Entity::Ref entity = m_entity.lock();
	if (!entity)
		return;
	if (!entity->LoadLuaCallback(m_callbackId))
		return;

	lua_State *L = entity->world->lua->L;

	lua_getfield(L, -1, "OnComplete");
	if (lua_type(L, -1) != LUA_TFUNCTION) {
		lua_pop(L, 2);
		return;
	}

	entity->PushEntityFrame(L);
	entity->world->lua->Call(L, "WorldLua::CinematicsNotify::OnComplete", 1, 0, 0);
	lua_pop(L, 1); // pop callback table
}

void WorldLua::CinematicsNotify::OnSkip() {
	Entity::Ref entity = m_entity.lock();
	if (!entity)
		return;
	if (!entity->LoadLuaCallback(m_callbackId))
		return;

	lua_State *L = entity->world->lua->L;

	lua_getfield(L, -1, "OnSkip");
	if (lua_type(L, -1) != LUA_TFUNCTION) {
		lua_pop(L, 2);
		return;
	}

	entity->PushEntityFrame(L);
	entity->world->lua->Call(L, "WorldLua::CinematicsNotify::OnSkip", 1, 0, 0);
	lua_pop(L, 1); // pop callback table
}

int WorldLua::CinematicsNotify::lua_SetMasked(lua_State *L) {
	Ref self = lua::SharedPtr::Get<CinematicsNotify>(L, "WorldLua::CinematicsNotify", 1, true);
	bool wasMasked = self->m_masked;
	self->m_masked = lua_toboolean(L, 2) ? true : false;
	lua_pushboolean(L, wasMasked ? 1 : 0);
	return 1;
}

int WorldLua::lua_World_PlayCinematic(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	WorldCinematics::Notify::Ref notify;

	Entity *entity = EntFramePtr(L, 4, false);
	if (entity && lua_gettop(L) > 4) { // passed in callbacks?
		int callbackId = entity->StoreLuaCallback(L, 5, 4);
		RAD_ASSERT(callbackId != -1);
		notify.reset(new CinematicsNotify(self->m_world, *entity, callbackId));
	}

	bool r = self->m_world->cinematics->PlayCinematic(
		luaL_checkstring(L, 1),
		(int)luaL_checkinteger(L, 2),
		(float)luaL_checknumber(L, 3),
		notify
	);

	lua_pushboolean(L, r ? 1 : 0);
	return 1;
}

int WorldLua::lua_World_StopCinematic(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	self->m_world->cinematics->StopCinematic(
		luaL_checkstring(L, 1)
	);

	return 0;
}

int WorldLua::lua_World_SkipCinematics(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	self->m_world->cinematics->Skip();
	return 0;
}

int WorldLua::lua_World_CinematicTime(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	float r = self->m_world->cinematics->CinematicTime(luaL_checkstring(L, 1));
	lua_pushnumber(L, r);
	return 1;
}

int WorldLua::lua_World_SetCinematicTime(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	bool r = self->m_world->cinematics->SetCinematicTime(
		luaL_checkstring(L, 1),
		(float)luaL_checknumber(L, 2)
	);

	lua_pushboolean(L, r ? 1 : 0);
	return 1;
}

int WorldLua::lua_World_SoundFadeMasterVolume(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	self->m_world->sound->FadeMasterVolume(
		(float)luaL_checknumber(L, 1),
		(float)luaL_checknumber(L, 2)
	);
	return 0;
}

int WorldLua::lua_World_SoundFadeChannelVolume(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	int c = (int)luaL_checkinteger(L, 1);
	if (c < SC_First || c >= SC_Max)
		luaL_error(L, "Invalid SoundChannel %d", c);
	self->m_world->sound->FadeChannelVolume(
		(SoundChannel)c,
		(float)luaL_checknumber(L, 2),
		(float)luaL_checknumber(L, 3)
	);
	return 0;
}

int WorldLua::lua_World_SoundChannelVolume(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	int c = (int)luaL_checkinteger(L, 1);
	if (c < SC_First || c >= SC_Max)
		luaL_error(L, "Invalid SoundChannel %d", c);
	lua_pushnumber(L, (lua_Number)self->m_world->sound->ChannelVolume((SoundChannel)c));
	return 1;
}

int WorldLua::lua_World_SoundPauseChannel(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	int c = (int)luaL_checkinteger(L, 1);
	if (c < SC_First || c >= SC_Max)
		luaL_error(L, "Invalid SoundChannel %d", c);
	self->m_world->sound->PauseChannel((SoundChannel)c, lua_toboolean(L, 2) ? true : false);
	return 0;
}

int WorldLua::lua_World_SoundChannelIsPaused(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	int c = (int)luaL_checkinteger(L, 1);
	if (c < SC_First || c >= SC_Max)
		luaL_error(L, "Invalid SoundChannel %d", c);
	lua_pushboolean(L, self->m_world->sound->ChannelIsPaused((SoundChannel)c) ? 1 : 0);
	return 1;
}

int WorldLua::lua_World_SoundPauseAll(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	self->m_world->sound->PauseAll(lua_toboolean(L, 1) ? true : false);
	return 0;
}

int WorldLua::lua_World_SoundStopAll(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	self->m_world->sound->StopAll();
	return 0;
}

int WorldLua::lua_World_SoundSetDoppler(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	self->m_world->sound->SetDoppler(
		(float)luaL_checknumber(L, 1),
		(float)luaL_checknumber(L, 2)
	);
	return 0;
}

int WorldLua::lua_World_RequestLoad(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	self->m_world->RequestLoad(
		luaL_checkstring(L, 1),
		(UnloadDisposition)luaL_checkinteger(L, 2),
		(lua_isnone(L, 3) || lua_toboolean(L, 3)) ? true : false
	);
	return 0;
}

int WorldLua::lua_World_RequestReturn(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	self->m_world->RequestReturn();
	return 0;
}

int WorldLua::lua_World_RequestSwitch(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	self->m_world->RequestSwitch((int)luaL_checkinteger(L, 1));
	return 0;
}

int WorldLua::lua_World_RequestUnloadSlot(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	self->m_world->RequestUnloadSlot((int)luaL_checkinteger(L, 1));
	return 0;
}

int WorldLua::lua_World_RequestSwitchLoad(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	self->m_world->RequestSwitchLoad(
		(int)luaL_checkinteger(L, 1),
		luaL_checkstring(L, 2),
		(UnloadDisposition)luaL_checkinteger(L, 3),
		(lua_isnone(L, 4) || lua_toboolean(L, 4)) ? true : false
	);
	return 0;
}

int WorldLua::lua_System_SaveSession(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	lua_pop(L, 1);

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
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	lua_pop(L, 1);

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
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	lua_pop(L, 1);

	luaL_checktype(L, 1, LUA_TTABLE);
	self->m_world->game->CreateSaveGame(luaL_checkstring(L, 2));
	PushKeysTable(L, *self->m_world->game->saveGame->keys.get());
	lua_setfield(L, 1, "keys");
	return 0;
}

int WorldLua::lua_System_LoadSavedGame(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	lua_pop(L, 1);

	luaL_checktype(L, 1, LUA_TTABLE);
	self->m_world->game->LoadSavedGame(luaL_checkstring(L, 2));
	PushKeysTable(L, *self->m_world->game->saveGame->keys.get());
	lua_setfield(L, 1, "keys");
	return 0;
}

int WorldLua::lua_System_LoadSaveTable(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	lua_pop(L, 1);

	PushKeysTable(L, *self->m_world->game->saveGame->keys.get());
	lua_setfield(L, 1, "keys");
	return 0;
}

int WorldLua::lua_System_SaveGame(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	lua_pop(L, 1);

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
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	lua_pop(L, 1);

	luaL_checktype(L, 1, LUA_TTABLE);
	lua_pushinteger(L, self->m_world->game->numSavedGameConflicts.get());
	return 1;
}

int WorldLua::lua_System_LoadSavedGameConflict(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	lua_pop(L, 1);

	luaL_checktype(L, 1, LUA_TTABLE);
	self->m_world->game->LoadSavedGameConflict((int)luaL_checkinteger(L, 2));
	PushKeysTable(L, *self->m_world->game->saveGame->keys.get());
	lua_setfield(L, 1, "keys");
	return 0;
}

int WorldLua::lua_System_ResolveSavedGameConflict(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	lua_pop(L, 1);

	luaL_checktype(L, 1, LUA_TTABLE);
	self->m_world->game->ResolveSavedGameConflict((int)luaL_checkinteger(L, 2));
	return 0;
}

int WorldLua::lua_System_EnableCloudStorage(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	lua_pop(L, 1);

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

int WorldLua::lua_World_SetGameSpeed(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	self->m_world->SetGameSpeed(
		(float)luaL_checknumber(L, 1),
		(float)luaL_checknumber(L, 2)
	);
	return 0;
}

int WorldLua::lua_World_SetPauseState(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	self->m_world->pauseState = (int)luaL_checkinteger(L, 1);
	return 0;
}

int WorldLua::lua_World_SwapMaterial(lua_State *L) {
	D_Asset::Ref src = lua::SharedPtr::Get<D_Asset>(L, "D_Asset", 1, true);
	if (src->asset->type != asset::AT_Material)
		return 0;
	D_Asset::Ref dst = lua::SharedPtr::Get<D_Asset>(L, "D_Asset", 2, true);
	if (dst->asset->type != asset::AT_Material)
		return 0;

	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
//	self->m_world->draw->SwapMaterial(src->asset->id, dst->asset->id);
	return 0;
}

int WorldLua::lua_World_PlayerPawn(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	Entity::Ref r = self->m_world->playerPawn;
	if (r) {
		r->PushEntityFrame(L);
	} else {
		lua_pushnil(L);
	}

	return 1;
}

int WorldLua::lua_World_SetPlayerPawn(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	if (lua_isnil(L, 1)) {
		self->m_world->playerPawn = Entity::Ref();
	} else {
		self->m_world->playerPawn = EntFramePtr(L, 1, true)->shared_from_this();
	}

	return 0;
}

int WorldLua::lua_World_Worldspawn(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	Entity::Ref r = self->m_world->worldspawn;
	if (r) {
		r->PushEntityFrame(L);
	} else {
		lua_pushnil(L);
	}

	return 1;
}

int WorldLua::lua_World_SetWorldspawn(lua_State *L)
{
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	if (lua_isnil(L, 1)) {
		self->m_world->worldspawn = Entity::Ref();
	} else {
		self->m_world->worldspawn = EntFramePtr(L, 1, true)->shared_from_this();
	}

	return 0;
}

int WorldLua::lua_World_ViewController(lua_State *L)
{
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	Entity::Ref r = self->m_world->viewController;
	if (r) {
		r->PushEntityFrame(L);
	} else {
		lua_pushnil(L);
	}

	return 1;
}

int WorldLua::lua_World_SetViewController(lua_State *L)
{
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	if (lua_isnil(L, 1)) {
		self->m_world->viewController = Entity::Ref();
	} else {
		self->m_world->viewController = EntFramePtr(L, 1, true)->shared_from_this();
	}

	return 0;
}

int WorldLua::lua_World_GameTime(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	lua_pushnumber(L, self->m_world->gameTime.get()*1000.f);
	return 1;
}

int WorldLua::lua_World_SysTime(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	lua_pushnumber(L, self->m_world->time.get()*1000.f);
	return 1;
}

int WorldLua::lua_World_DeltaTime(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	lua_pushnumber(L, self->m_world->dt.get()*1000.f);
	return 1;
}

int WorldLua::lua_World_Viewport(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	int vpx, vpy, vpw, vph;
	self->m_world->game->Viewport(vpx, vpy, vpw, vph);
	lua::Marshal<Vec2>::Push(L, Vec2((float)vpw, (float)vph));
	return 1;
}

int WorldLua::lua_World_CameraPos(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	lua::Marshal<Vec3>::Push(L, self->m_world->camera->pos);
	return 1;
}

int WorldLua::lua_World_CameraFarClip(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	lua_pushnumber(L, (lua_Number)self->m_world->camera->farClip.get());
	return 1;
}

int WorldLua::lua_World_SetCameraFarClip(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	self->m_world->camera->farClip = (float)luaL_checknumber(L, 1);
	return 0;
}

int WorldLua::lua_World_CameraAngles(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	lua::Marshal<Vec3>::Push(L, self->m_world->camera->angles);
	return 1;
}

int WorldLua::lua_World_CameraFOV(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	lua_pushnumber(L, (lua_Number)self->m_world->camera->fov.get());
	return 1;
}

int WorldLua::lua_World_CameraFwd(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	lua::Marshal<Vec3>::Push(L, self->m_world->camera->fwd);
	return 1;
}

int WorldLua::lua_World_CameraLeft(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	lua::Marshal<Vec3>::Push(L, self->m_world->camera->left);
	return 1;
}

int WorldLua::lua_World_CameraUp(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	lua::Marshal<Vec3>::Push(L, self->m_world->camera->up);
	return 1;
}
	
int WorldLua::lua_World_SetEnabledGestures(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	self->m_world->enabledGestures = (int)luaL_checkinteger(L, 1);
	return 0;
}

int WorldLua::lua_World_FlushInput(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	self->m_world->game->FlushInput(lua_toboolean(L, 1) ? true : false);
	return 0;
}

int WorldLua::lua_World_DrawCounters(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	const WorldDraw::Counters *counters = self->m_world->drawCounters;

	lua_createtable(L, 0, 7);
	lua_pushinteger(L, counters->drawnLeafs);
	lua_setfield(L, -2, "drawnLeafs");
	lua_pushinteger(L, counters->testedLeafs);
	lua_setfield(L, -2, "testedLeafs");
	lua_pushinteger(L, counters->drawnNodes);
	lua_setfield(L, -2, "drawnNodes");
	lua_pushinteger(L, counters->testedNodes);
	lua_setfield(L, -2, "testedNodes");
	lua_pushinteger(L, counters->numModels);
	lua_setfield(L, -2, "numModels");
	lua_pushinteger(L, counters->numTris);
	lua_setfield(L, -2, "numTris");
	lua_pushinteger(L, counters->numMaterials);
	lua_setfield(L, -2, "numMaterials");

	return 1;
}

int WorldLua::lua_World_EnableWireframe(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	self->m_world->draw->wireframe = lua_toboolean(L, 1) ? true : false;
	return 0;
}

int WorldLua::lua_System_CurrentDateAndTime(lua_State *L) {
	xtime::TimeDate ct = xtime::TimeDate::Now(xtime::TimeDate::local_time_tag());

	lua_createtable(L, 0, 8);
	lua_pushinteger(L, ct.year);
	lua_setfield(L, -2, "year");
	lua_pushinteger(L, ct.millis);
	lua_setfield(L, -2, "millis");
	lua_pushinteger(L, ct.month);
	lua_setfield(L, -2, "month");
	lua_pushinteger(L, ct.dayOfMonth);
	lua_setfield(L, -2, "day");
	lua_pushinteger(L, ct.dayOfWeek);
	lua_setfield(L, -2, "dayOfWeek");
	lua_pushinteger(L, ct.hour);
	lua_setfield(L, -2, "hour");
	lua_pushinteger(L, ct.minute);
	lua_setfield(L, -2, "minute");
	lua_pushinteger(L, ct.second);
	lua_setfield(L, -2, "second");

	return 1;
}

int WorldLua::lua_World_QuitGame(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	self->m_world->game->quit = true;
	return 0;
}

int WorldLua::lua_gnCreate(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	lua_pushboolean(L, self->m_world->game->CreateGameNetwork() ? 1 : 0);
	return 1;
}

int WorldLua::lua_gnAuthenticateLocalPlayer(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	gn::GameNetwork *network = self->m_world->game->gameNetwork;
	if (network)
		network->AuthenticateLocalPlayer();

	return 0;
}

int WorldLua::lua_gnLocalPlayerId(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

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
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	gn::GameNetwork *network = self->m_world->game->gameNetwork;
	if (network && network->localPlayer->authenticated) {
		const char *leaderboardId = luaL_checkstring(L, 1);
		int score = (int)luaL_checknumber(L, 2);
		network->SendScore(leaderboardId, score);
	}

	return 0;
}

int WorldLua::lua_gnSendAchievement(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	lua_pop(L, 1);

	gn::GameNetwork *network = self->m_world->game->gameNetwork;
	if (network && network->localPlayer->authenticated) {
		const char *achievementId = luaL_checkstring(L, 1);
		float percent = (lua_gettop(L) > 1) ? (float)luaL_checknumber(L, 2) : 100.f;
		network->SendAchievement(achievementId, percent);
	}

	return 0;
}

int WorldLua::lua_gnShowLeaderboard(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	gn::GameNetwork *network = self->m_world->game->gameNetwork;
	if (network) {
		const char *leaderboardId = luaL_checkstring(L, 1);
		network->ShowLeaderboard(leaderboardId);
	}

	return 0;
}
	
int WorldLua::lua_gnShowAchievements(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	
	gn::GameNetwork *network = self->m_world->game->gameNetwork;
	if (network)
		network->ShowAchievements();
	
	return 0;
}

int WorldLua::lua_gnLogEvent(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	lua_pop(L, 1);

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
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);
	lua_pop(L, 1);

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
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	gn::GameNetwork *network = self->m_world->game->gameNetwork;
	if (network) {
		const char *eventName = luaL_checkstring(L, 1);
		const char *message = luaL_checkstring(L, 2);
		network->LogError(eventName, message);
	}

	return 0;
}

int WorldLua::lua_gnSessionReportOnAppClose(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	gn::GameNetwork *network = self->m_world->game->gameNetwork;
	if (network) {
		lua_pushboolean(L, network->sessionReportOnAppClose ? 1 : 0);
	} else {
		lua_pushboolean(L, 0);
	}

	return 1;
}

int WorldLua::lua_gnSetSessionReportOnAppClose(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	gn::GameNetwork *network = self->m_world->game->gameNetwork;
	if (network)
		network->sessionReportOnAppClose = lua_toboolean(L, 1) ? true : false;

	return 0;
}

int WorldLua::lua_gnSessionReportOnAppPause(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	gn::GameNetwork *network = self->m_world->game->gameNetwork;
	if (network) {
		lua_pushboolean(L, network->sessionReportOnAppPause ? 1 : 0);
	} else {
		lua_pushboolean(L, 0);
	}

	return 1;
}

int WorldLua::lua_gnSetSessionReportOnAppPause(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1);

	gn::GameNetwork *network = self->m_world->game->gameNetwork;
	if (network)
		network->sessionReportOnAppPause = lua_toboolean(L, 1) ? true : false;

	return 0;
}

lua::SrcBuffer::Ref WorldLua::ImportLoader::Load(lua_State *L, const char *name) {
	String path(CStr("Scripts/"));
	path += name;
	path += ".lua";

	file::MMapping::Ref mm = App::Get()->engine->sys->files->MapFile(path.c_str, ZWorld);
	if (!mm)
		return lua::SrcBuffer::Ref();

	return lua::SrcBuffer::Ref(new FileSrcBuffer(name, mm));
}

} // world

namespace lua {

void Marshal<world::Trace>::Push(lua_State *L, const world::Trace &val) {
	lua_createtable(L, 0, 6);
	Marshal<Vec3>::Push(L, val.start);
	lua_setfield(L, -2, "start");
	Marshal<Vec3>::Push(L, val.end);
	lua_setfield(L, -2, "end");
	Marshal<Vec3>::Push(L, val.result);
	lua_setfield(L, -2, "result");
	lua_pushinteger(L, val.contents);
	lua_setfield(L, -2, "contents");
	lua_pushnumber(L, val.frac);
	lua_setfield(L, -2, "frac");
	lua_pushboolean(L, val.startSolid ? 1 : 0);
	lua_setfield(L, -2, "startSolid");
}

world::Trace Marshal<world::Trace>::Get(lua_State *L, int index, bool luaError) {
	world::Trace t;
	t.startSolid = false;
	t.frac = 1.f;
	t.contents = world::bsp_file::kContentsFlag_Solid;

	lua_getfield(L, index, "start");
	t.start = Marshal<Vec3>::Get(L, -1, luaError);
	lua_pop(L, 1);

	lua_getfield(L, index, "end");
	t.end = Marshal<Vec3>::Get(L, -1, luaError);
	lua_pop(L, 1);

	lua_getfield(L, index, "contents");
	if (lua_type(L, -1) == LUA_TNUMBER)
		t.contents = (int)lua_tointeger(L, -1);
	lua_pop(L, 1);

	// we do not marshal frac or startSolid (output params only).
	return t;
}

bool Marshal<world::Trace>::IsA(lua_State *L, int index) {
	return lua_type(L, index) == LUA_TTABLE;
}

} // lua
