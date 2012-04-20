// LuaTask.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "LuaTask.h"

namespace world {

void LuaTask::PushElements(lua_State *L)
{
	lua_pushcfunction(L, lua_Pending);
	lua_setfield(L, -2, "Pending");
	lua_pushcfunction(L, lua_Result);
	lua_setfield(L, -2, "Result");
	lua_pushcfunction(L, lua_Cancel);
	lua_setfield(L, -2, "Cancel");
}

} // world
