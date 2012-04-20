// T_Tick.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "T_Tick.h"
#include "../World.h"

namespace world {

T_Tick::T_Tick(const Entity::Ref &entity, int tickFlags) : 
Entity::Tickable(TickPriorityNormal), 
m_id(-1),
m_tickFlags(tickFlags), 
m_ent(entity)
{
}

T_Tick::~T_Tick()
{
}

int T_Tick::Tick(Entity &e, float dt, const xtime::TimeSlice &time, int flags)
{
	int result = TickNext;

	if (flags&m_tickFlags)
	{ // invoke callback into script
		if (m_id != -1 && e.LoadLuaCallback(m_id))
		{
			lua_State *L = e.world->lua->L;

			if (lua_pcall(L, 0, 1, 0))
			{
				COut(C_Error) << "(ScriptError):T_Tick:" << lua_tostring(L, -1) << std::endl;
			}
			else if (lua_isnumber(L, -1))
			{
				result = (int)lua_tointeger(L, -1);
			}

			lua_pop(L, 1);
		}
	}

	if ((result == TickPop) && (m_id != -1))
	{
		e.ReleaseLuaCallback(m_id);
		m_id = -1;
	}

	return result;
}

void T_Tick::PushElements(lua_State *L)
{
	lua_pushcfunction(L, lua_Remove);
	lua_setfield(L, -2, "Remove");
}

int T_Tick::lua_Remove(lua_State *L)
{
	T_Tick::Ref ref = lua::SharedPtr::Get<T_Tick>(L, "T_Tick", 1, true);
	ref->Dequeue();
	Entity::Ref entity = ref->m_ent.lock();
	if (entity)
		entity->ReleaseLuaCallback(ref->m_id);
	ref->m_id = -1;
	return 0;
}

} // world
