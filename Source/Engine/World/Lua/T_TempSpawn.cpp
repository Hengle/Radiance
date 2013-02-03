// T_TempSpawn.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "T_TempSpawn.h"
#include "../World.h"

namespace world {

T_TempSpawn::Ref T_TempSpawn::New(World *world, const Keys &keys) {
	return Ref(new (ZWorld) T_TempSpawn(world, keys));
}

T_TempSpawn::T_TempSpawn(World *world, const Keys &keys) : 
m_world(world), 
m_keys(keys),
m_r(pkg::SR_Pending),
m_post(false) {
}

int T_TempSpawn::Tick(Entity &e, float dt, const xtime::TimeSlice &time, int flags) {
	int r = pkg::SR_Pending;

	while (time.remaining) {
		if (m_post) {
			r = m_world->PostSpawnEntity(m_entity, time);
			if (r <= pkg::SR_Success)
				break;
		} else {
			r = m_world->SpawnTempEntity(m_entity, m_keys, time);
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

int T_TempSpawn::PushResult(lua_State *L) {
	if (m_r != pkg::SR_Success)
		return 0;

	if (m_entity) {
		m_entity->PushEntityFrame(L);
		return 1;
	}
	return 0;
}

} // world
