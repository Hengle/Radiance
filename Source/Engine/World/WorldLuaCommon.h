/*! \file WorldLuaCommon.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#pragma once

#define SELF "@world"
#define ENTREF_TABLE "@ents"
#define SPRINTF_ENTREGS "%d"
#define LOAD_SELF \
	lua_getfield(L, LUA_REGISTRYINDEX, SELF); \
	WorldLua *self = (WorldLua*)lua_touserdata(L, -1); \
	lua_pop(L, 1);