// Input.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "Input.h"

namespace lua {

void Marshal<InputEvent>::Push(lua_State *L, const InputEvent &e, const TouchState *touch)
{
	lua_createtable(L, 0, 4);
	lua_pushlightuserdata(L, e.touch);
	lua_setfield(L, -2, "touch");
	lua_pushinteger(L, e.type);
	lua_setfield(L, -2, "type");
	lua_pushinteger(L, e.time);
	lua_setfield(L, -2, "time");
	if (touch)
	{
		lua_pushinteger(L, touch->Age());
	}
	else
	{
		lua_pushinteger(L, 0);
	}
	lua_setfield(L, -2, "age");
	lua_createtable(L, 3, 0);
	for (int i = 0; i < 3; ++i)
	{
		lua_pushinteger(L, i+1);
		lua_pushinteger(L, e.data[i]);
		lua_settable(L, -3);
	}
	lua_setfield(L, -2, "data");
}

void Marshal<InputGesture>::Push(lua_State *L, const InputGesture &g, const TouchState &touch)
{
	lua_createtable(L, 0, 5);
	lua_pushinteger(L, g.id);
	lua_setfield(L, -2, "id");
	lua_pushinteger(L, g.phase);
	lua_setfield(L, -2, "phase");
	lua_pushinteger(L, g.time);
	lua_setfield(L, -2, "time");
	lua_pushinteger(L, touch.Age());
	lua_setfield(L, -2, "age");
	
	lua_createtable(L, 2, 0);
	for (int i = 0; i < 2; ++i)
	{
		lua_pushinteger(L, i+1);
		lua_pushinteger(L, g.mins[i]);
		lua_settable(L, -3);
	}
	lua_setfield(L, -2, "mins");

	lua_createtable(L, 2, 0);
	for (int i = 0; i < 2; ++i)
	{
		lua_pushinteger(L, i+1);
		lua_pushinteger(L, g.maxs[i]);
		lua_settable(L, -3);
	}
	lua_setfield(L, -2, "maxs");

	lua_createtable(L, 2, 0);
	for (int i = 0; i < 2; ++i)
	{
		lua_pushinteger(L, i+1);
		lua_pushinteger(L, g.origin[i]);
		lua_settable(L, -3);
	}
	lua_setfield(L, -2, "origin");
	
	lua::Marshal<Vec3>::Push(L, g.args);
	lua_setfield(L, -2, "args");
}

} // lua
