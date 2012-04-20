// Spring.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "Spring.h"

namespace physics {

Spring::Spring() :
length(1.f),
elasticity(1.f),
tolerance(0.5f)
{
}

SpringVertex::SpringVertex() :
pos(Vec3::Zero),
vel(Vec3::Zero),
force(Vec3::Zero),
mass(100.f),
friction(0.1f),
length(0.f),
inner(false),
outer(false),
atRest(true)
{
	drag[0] = drag[1] = 0.1f;
}

bool SpringVertex::Update(float dt, const Vec3 &root, const Spring &spring)
{
	force = pos - root;
	
	atRest = false;

	length = force.Magnitude();
	float mag = length;

	if ((!inner && mag <= spring.length) ||
		(!outer && mag >= spring.length) ||
		(mag >= spring.length-spring.tolerance && mag <= spring.length+spring.tolerance))
	{
		atRest = true;
		force = Vec3::Zero;
	}

	if (!atRest)
	{
		force.Normalize();
		mag = -spring.elasticity * (mag - spring.length);
		force = (force*mag) - (friction*vel);
		vel += (force/mass)*dt;
	}

	pos += vel*dt;
	mag = dt * (atRest ? drag[1] : drag[0]);
	mag = math::Clamp(mag, 0.f, 1.f);
	vel *= (1.f - mag);

	return !atRest;
}

} // physics

namespace lua {

void Marshal<physics::Spring>::Push(lua_State *L, const physics::Spring &s)
{
	lua_createtable(L, 0, 3);
	lua_pushnumber(L, s.length);
	lua_setfield(L, -2, "length");
	lua_pushnumber(L, s.elasticity);
	lua_setfield(L, -2, "elasticity");
	lua_pushnumber(L, s.tolerance);
	lua_setfield(L, -2, "tolerance");
}

physics::Spring Marshal<physics::Spring>::Get(lua_State *L, int index, bool luaError)
{
	physics::Spring s;

	lua_getfield(L, index, "length");
	if (luaError && lua_type(L, -1) != LUA_TNUMBER)
		luaL_typerror(L, index, "physics::Spring");
	s.length = (float)lua_tonumber(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, index, "elasticity");
	if (luaError && lua_type(L, -1) != LUA_TNUMBER)
		luaL_typerror(L, index, "physics::Spring");
	s.elasticity = (float)lua_tonumber(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, index, "tolerance");
	if (luaError && lua_type(L, -1) != LUA_TNUMBER)
		luaL_typerror(L, index, "physics::Spring");
	s.tolerance = (float)lua_tonumber(L, -1);
	lua_pop(L, 1);

	return s;
}

void Marshal<physics::SpringVertex>::Push(lua_State *L, const physics::SpringVertex &v)
{
	lua_createtable(L, 0, 8);
	Marshal<Vec3>::Push(L, v.pos);
	lua_setfield(L, -2, "pos");
	Marshal<Vec3>::Push(L, v.vel);
	lua_setfield(L, -2, "vel");
	Marshal<Vec3>::Push(L, v.force);
	lua_setfield(L, -2, "force");
	lua_pushnumber(L, v.mass);
	lua_setfield(L, -2, "mass");
	lua_pushnumber(L, v.length);
	lua_setfield(L, -2, "length");
	lua_createtable(L, 2, 0);
	lua_pushinteger(L, 1);
	lua_pushnumber(L, v.drag[0]);
	lua_settable(L, -3);
	lua_pushinteger(L, 2);
	lua_pushnumber(L, v.drag[1]);
	lua_settable(L, -3);
	lua_setfield(L, -2, "drag");
	lua_pushnumber(L, v.friction);
	lua_setfield(L, -2, "friction");
	lua_pushboolean(L, v.inner ? 1 : 0);
	lua_setfield(L, -2, "inner");
	lua_pushboolean(L, v.outer ? 1 : 0);
	lua_setfield(L, -2, "outer");
	lua_pushboolean(L, v.atRest ? 1 : 0);
	lua_setfield(L, -2, "atRest");
}

physics::SpringVertex Marshal<physics::SpringVertex>::Get(lua_State *L, int index, bool luaError)
{
	physics::SpringVertex v;

	lua_getfield(L, index, "pos");
	v.pos = Marshal<Vec3>::Get(L, -1, luaError);
	lua_pop(L, 1);

	lua_getfield(L, index, "vel");
	v.vel = Marshal<Vec3>::Get(L, -1, luaError);
	lua_pop(L, 1);

	lua_getfield(L, index, "force");
	v.force = Marshal<Vec3>::Get(L, -1, luaError);
	lua_pop(L, 1);

	lua_getfield(L, index, "mass");
	if (luaError && lua_type(L, -1) != LUA_TNUMBER)
		luaL_typerror(L, index, "physics::SpringVertex");
	v.mass = (float)lua_tonumber(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, index, "drag");
	if (luaError && lua_type(L, -1) != LUA_TTABLE)
		luaL_typerror(L, index, "physics::SpringVertex");
	lua_pushinteger(L, 1);
	lua_gettable(L, -2);
	if (luaError && lua_type(L, -1) != LUA_TNUMBER)
		luaL_typerror(L, index, "physics::SpringVertex");
	v.drag[0] = (float)lua_tonumber(L, -1);
	lua_pop(L, 1);
	lua_pushinteger(L, 2);
	lua_gettable(L, -2);
	if (luaError && lua_type(L, -1) != LUA_TNUMBER)
		luaL_typerror(L, index, "physics::SpringVertex");
	v.drag[1] = (float)lua_tonumber(L, -1);
	lua_pop(L, 2);

	lua_getfield(L, index, "friction");
	if (luaError && lua_type(L, -1) != LUA_TNUMBER)
		luaL_typerror(L, index, "physics::SpringVertex");
	v.friction = (float)lua_tonumber(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, index, "inner");
	if (luaError && lua_type(L, -1) != LUA_TBOOLEAN)
		luaL_typerror(L, index, "physics::SpringVertex");
	v.inner = lua_toboolean(L, -1) ? true : false;
	lua_pop(L, 1);

	lua_getfield(L, index, "outer");
	if (luaError && lua_type(L, -1) != LUA_TBOOLEAN)
		luaL_typerror(L, index, "physics::SpringVertex");
	v.outer = lua_toboolean(L, -1) ? true : false;
	lua_pop(L, 1);

	return v;
}

} // lua
