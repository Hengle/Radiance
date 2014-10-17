/*! \file WorldLuaCVars.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#include RADPCH
#include "../App.h"
#include "../Engine.h"
#include "../Game/Game.h"
#include "../Game/GameCVars.h"
#include "../CVars.h"
#include "World.h"
#include "WorldLuaCommon.h"

namespace world {

int WorldLua::lua_CVar(lua_State *L) {
	LOAD_SELF
	const char *sz = luaL_checkstring(L, 1);
	CVar *cvar = self->m_world->game->cvarZone->Find(sz, CVarZone::kFindScope_IncludingGlobals);
	if (!cvar)
		luaL_error(L, "no variable exists with that name (while trying to bind cvar named %s)", sz);
	if (self->m_world->cvars->IsLuaVar(cvar)) {
		luaL_error(L, "cvar %s is a luacvar, use the lua reference instead!", sz);
		return 0;
	}
	switch (cvar->type) {
	case CVarBool::kType:
		return lua_PkgCVar(L, static_cast<CVarBool*>(cvar));
	case CVarFloat::kType:
		return lua_PkgCVar(L, static_cast<CVarFloat*>(cvar));
	case CVarInt::kType:
		return lua_PkgCVar(L, static_cast<CVarInt*>(cvar));
	case CVarString::kType:
		return lua_PkgCVar(L, static_cast<CVarString*>(cvar));
	case CVarFunc::kType:
		return lua_PkgNativeFunc(L, static_cast<CVarFunc*>(cvar));
	default:
		break;
	}

	luaL_error(L, "cvar '%s' has invalid type %d", sz, cvar->type.get());
	return 0;
}

int WorldLua::lua_PkgCVar(lua_State *L, CVarString *cvar) {
	lua_createtable(L, 0, 3);
	lua_pushlightuserdata(L, cvar);
	lua_setfield(L, -2, "@ptr");
	lua_pushcfunction(L, lua_CVarStringGet);
	lua_setfield(L, -2, "Get");
	lua_pushcfunction(L, lua_CVarStringSet);
	lua_setfield(L, -2, "Set");
	return 1;
}

int WorldLua::lua_PkgCVar(lua_State *L, CVarInt *cvar) {
	lua_createtable(L, 0, 3);
	lua_pushlightuserdata(L, cvar);
	lua_setfield(L, -2, "@ptr");
	lua_pushcfunction(L, lua_CVarIntGet);
	lua_setfield(L, -2, "Get");
	lua_pushcfunction(L, lua_CVarIntSet);
	lua_setfield(L, -2, "Set");
	return 1;
}

int WorldLua::lua_PkgCVar(lua_State *L, CVarBool *cvar) {
	lua_createtable(L, 0, 3);
	lua_pushlightuserdata(L, cvar);
	lua_setfield(L, -2, "@ptr");
	lua_pushcfunction(L, lua_CVarBoolGet);
	lua_setfield(L, -2, "Get");
	lua_pushcfunction(L, lua_CVarBoolSet);
	lua_setfield(L, -2, "Set");
	return 1;
}

int WorldLua::lua_PkgCVar(lua_State *L, CVarFloat *cvar) {
	lua_createtable(L, 0, 3);
	lua_pushlightuserdata(L, cvar);
	lua_setfield(L, -2, "@ptr");
	lua_pushcfunction(L, lua_CVarFloatGet);
	lua_setfield(L, -2, "Get");
	lua_pushcfunction(L, lua_CVarFloatSet);
	lua_setfield(L, -2, "Set");
	return 1;
}

int WorldLua::lua_PkgNativeFunc(lua_State *L, CVarFunc *cvar) {
	lua_createtable(L, 0, 3);
	lua_pushlightuserdata(L, cvar);
	lua_setfield(L, -2, "@ptr");
	lua_pushcfunction(L, lua_CVarFuncNativeCall);
	lua_setfield(L, -2, "Execute");
	return 1;
}

int WorldLua::lua_PkgLuaFunc(lua_State *L, CVarFunc *cvar) {
	lua_createtable(L, 0, 3);
	lua_pushlightuserdata(L, cvar);
	lua_setfield(L, -2, "@ptr");
	lua_pushcfunction(L, lua_CVarFuncLuaCall);
	lua_setfield(L, -2, "Execute");
	return 1;
}

#define GET_CVAR(_type) \
	luaL_checktype(L, 1, LUA_TTABLE); \
	lua_getfield(L, 1, "@ptr"); \
	_type *cvar = (_type*)lua_touserdata(L, -1); \
	if (!cvar) \
		luaL_error(L, "bad cvar object")

#define MAKE_CVAR(_class, _data, _marshal) \
	LOAD_SELF \
	CVarZone &zone = *self->m_world->game->cvarZone.get(); \
	const char *name = luaL_checkstring(L, 1); \
	CVar *checkcvar = zone.Find(name, CVarZone::kFindScope_IncludingGlobals); \
	if (checkcvar) { \
		if (checkcvar->type != _class::kType) \
			luaL_error(L, "cvar '%s' exists but is of type %d (tried to create with type %d)", name, checkcvar->type.get(), _class::kType); \
		return lua_PkgCVar(L, static_cast<_class*>(checkcvar)); \
	} \
	_data value = _marshal(L, 2); \
	_class *cvar = new (ZWorld) _class(zone, name, value, false); \
	CVar::Ref r(cvar); \
	self->m_world->cvars->AddLuaVar(r); \
	return lua_PkgCVar(L, cvar);

int WorldLua::lua_CVarString(lua_State *L) {
	MAKE_CVAR(CVarString, const char*, luaL_checkstring);
}

int WorldLua::lua_CVarStringGet(lua_State *L) {
	GET_CVAR(CVarString);
	lua_pushstring(L, cvar->value.get().c_str);
	return 1;
}

int WorldLua::lua_CVarStringSet(lua_State *L) {
	GET_CVAR(CVarString);
	const char *sz = luaL_checkstring(L, 2);
	cvar->value = String(sz);
	return 0;
}

int WorldLua::lua_CVarInt(lua_State *L) {
	MAKE_CVAR(CVarInt, int, (int)luaL_checkinteger);
}

int WorldLua::lua_CVarIntGet(lua_State *L) {
	GET_CVAR(CVarInt);
	lua_pushinteger(L, (int)cvar->value);
	return 1;
}

int WorldLua::lua_CVarIntSet(lua_State *L) {
	GET_CVAR(CVarInt);
	cvar->value = (int)luaL_checkinteger(L, 2);
	return 0;
}

int WorldLua::lua_CVarBool(lua_State *L) {
	MAKE_CVAR(CVarBool, bool, 0 != lua_toboolean);
}

int WorldLua::lua_CVarBoolGet(lua_State *L) {
	GET_CVAR(CVarBool);
	lua_pushboolean(L, cvar->value ? 1 : 0);
	return 1;
}

int WorldLua::lua_CVarBoolSet(lua_State *L) {
	GET_CVAR(CVarBool);
	cvar->value = lua_toboolean(L, 2) != 0;
	return 0;
}

int WorldLua::lua_CVarFloat(lua_State *L) {
	MAKE_CVAR(CVarFloat, float, (float)luaL_checknumber);
}

int WorldLua::lua_CVarFloatGet(lua_State *L) {
	GET_CVAR(CVarFloat);
	lua_pushnumber(L, cvar->value);
	return 1;
}

int WorldLua::lua_CVarFloatSet(lua_State *L) {
	GET_CVAR(CVarFloat);
	cvar->value = (float)luaL_checknumber(L, 2);
	return 0;
}

int WorldLua::lua_CVarFunc(lua_State *L) {
	LOAD_SELF
	CVarZone &zone = *self->m_world->game->cvarZone.get();

	const char *name = luaL_checkstring(L, 1);
	CVar *checkcvar = zone.Find(name, CVarZone::kFindScope_IncludingGlobals);
	if (checkcvar) {
		if (checkcvar->type != CVarFunc::kType)
			luaL_error(L, "cvar '%s' exists but is of type %d (tried to create with type %d)", name, checkcvar->type.get(), CVarFunc::kType);
		if (self->m_world->cvars->IsLuaVar(checkcvar)) {
			return lua_PkgLuaFunc(L, static_cast<LuaCVarFunc*>(checkcvar));
		}
		luaL_error(L, "cvar '%s' is a native C++ cvar func and cannot be bound this way, use CVar(name) instead", name);
		return 0;
	}

	const char *fnCall = luaL_checkstring(L, 2);

	LuaCVarFunc *cvar = new (ZWorld) LuaCVarFunc(*self->m_world->game.get(), name, fnCall);
	CVar::Ref r(cvar);

	self->m_world->cvars->AddLuaVar(r);
	return lua_PkgLuaFunc(L, cvar);
}

int WorldLua::lua_CVarFuncNativeCall(lua_State *L) {
	GET_CVAR(CVarFunc);
	const char *cmdline = 0;
	if (!lua_isnil(L, 2))
		cmdline = luaL_checkstring(L, 2);
	cvar->Execute(cmdline);
	return 0;
}

int WorldLua::lua_CVarFuncLuaCall(lua_State *L) {
	GET_CVAR(LuaCVarFunc);
	const char *cmdline = 0;
	if (!lua_isnil(L, 2))
		cmdline = luaL_checkstring(L, 2);
	cvar->Execute(L, cmdline, false);
	return 0;
}

WorldLua::LuaCVarFunc::LuaCVarFunc(Game &game, const char *name, const char *fnCall)
: CVarFunc(*game.cvarZone.get(), name), m_game(&game), m_fnCall(fnCall) {
}

void WorldLua::LuaCVarFunc::Execute(const char *cmdline) {
	World::Ref world = m_game->world;
	if (!world) {
		COut(C_Error) << "ERROR(LuaCVarFunc): there is no world in which to execute cvarfunc '" << name.get() << "'" << std::endl;
		return;
	}
	Execute(world->lua->L, cmdline, true);
}

void WorldLua::LuaCVarFunc::Execute(lua_State *L, const char *cmdline, bool pcall) {
	
	if (!WorldLua::PushGlobalCall(L, m_fnCall.c_str)) {
		if (pcall) { // not a protected environment
			COut(C_Error) << "ERROR: there is no visible function matching the signature: '" << m_fnCall << "' (lua cvarfunc '" << name.get() << "'" << std::endl;
			return;
		} else {
			luaL_error(L, "there is no visible function matching the signature: '%s' (lua cvarfunc '%s')", m_fnCall.c_str.get(), name.get());
			return;
		}
	}
	
	int nargs = 0;
	if (cmdline && cmdline[0]) {
		lua_pushstring(L, cmdline);
		++nargs;
	}

	if (pcall) {
		String context(CStr("cvarfunc("));
		context += name.get();
		context += ")";
		WorldLua::Call(L, context.c_str, nargs, 0, 0);
	} else {
		lua_call(L, nargs, 0);
	}
}

} // world
