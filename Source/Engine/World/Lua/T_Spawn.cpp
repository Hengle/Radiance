// T_Spawn.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "T_Spawn.h"
#include "../World.h"

namespace world {

T_Spawn::Ref T_Spawn::New(World *world, const Keys &keys)
{
	return Ref(new (ZWorld) T_Spawn(world, keys));
}

T_Spawn::T_Spawn(World *world, const Keys &keys) : 
m_world(world), 
m_keys(keys),
m_r(pkg::SR_Pending),
m_post(false)
{
}

int T_Spawn::Tick(Entity &e, float dt, const xtime::TimeSlice &time, int flags)
{
	int r = pkg::SR_Pending;

	while (time.remaining)
	{
		if (m_post)
		{
			r = m_world->PostSpawnEntity(m_entity, time);
			if (r <= pkg::SR_Success)
				break;
		}
		else
		{
			r = m_world->SpawnEntity(m_entity, m_keys, time);
			if (r < pkg::SR_Success)
				break;
			m_post = r == pkg::SR_Success;
			if (m_post)
				r = pkg::SR_Pending;
		}
	}

	if (r < pkg::SR_Success)
		m_entity.reset(); // error.

	m_r = r;
	return (r<=pkg::SR_Success) ? TickPop : TickNext;
}

int T_Spawn::PushResult(lua_State *L)
{
	if (m_r != pkg::SR_Success)
		return 0;

	if (m_entity)
	{
		m_entity->PushEntityFrame(L);
		return 1;
	}

	return 0;
}

} // world
