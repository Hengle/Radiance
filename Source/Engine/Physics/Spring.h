// Spring.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Types.h"
#include "../Lua/LuaRuntime.h"
#include <Runtime/PushPack.h>

namespace physics {

struct Spring {
	Spring();

	float length;
	float elasticity; // larger number means stiffer spring

	// if vertex is +/- tolerance from "length" then
	// no force is applied by spring
	float tolerance;
};

struct SpringVertex {
	SpringVertex();

	Vec3 pos;
	Vec3 vel;
	Vec3 force; // force acting on vertex from spring
	float mass;
	float length; // distance to target

	// drag[0] is applied when there is force applied by the spring
	// drag[1] is applied when there is no force applied
	float drag[2];
	float friction;

	// when to apply spring force
	bool inner;
	bool outer;
	bool atRest;

	bool Update(float dt, const Vec3 &root, const Spring &spring);
};

} // physics

namespace lua {

template <>
struct Marshal<physics::Spring> {
	static void Push(lua_State *L, const physics::Spring &val);
	static physics::Spring Get(lua_State *L, int index, bool luaError);
	static bool IsA(lua_State *L, int index) {
		return lua_type(L, index) == LUA_TTABLE;
	}
};

template <>
struct Marshal<physics::SpringVertex> {
	static void Push(lua_State *L, const physics::SpringVertex &val);
	static physics::SpringVertex Get(lua_State *L, int index, bool luaError);
	static bool IsA(lua_State *L, int index) {
		return lua_type(L, index) == LUA_TTABLE;
	}
};

} // lua

#include <Runtime/PopPack.h>