// T_Tick.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Entity.h"
#include "../../Lua/LuaRuntime.h"
#include <Runtime/PushPack.h>

namespace world {

class RADENG_CLASS T_Tick : public Entity::Tickable, public lua::SharedPtr
{
public:
	typedef boost::shared_ptr<T_Tick> Ref;

	T_Tick(const Entity::Ref &entity, int tickFlags);
	virtual ~T_Tick();

	virtual int Tick(Entity &e, float dt, const xtime::TimeSlice &time, int flags);

protected:

	virtual void PushElements(lua_State *L); // <- from lua::SharedPtr

private:

	friend class Entity;

	Entity::WRef m_ent;
	int m_id;
	int m_tickFlags;

	static int lua_Remove(lua_State *L);
};

} // world

#include <Runtime/PopPack.h>
