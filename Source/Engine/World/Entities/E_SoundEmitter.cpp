// E_SoundEmitter.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "E_SoundEmitter.h"
#include "../World.h"

namespace world {

E_SoundEmitter::E_SoundEmitter() : 
E_CONSTRUCT_BASE, 
m_maxDistance(0.f), 
m_on(false),
m_positional(true)
{
}

void E_SoundEmitter::Tick(
	int frame,
	float dt,
	const xtime::TimeSlice &time
)
{
	if (!m_positional || frame < 1)
		return; // let view position settle.

	float distance = (ps->worldPos - world->listenerPos.get()).Magnitude();
	if (distance <= m_maxDistance && !m_on)
	{
		m_on = true;
		if (PushEntityCall("Enable"))
		{
			lua_pushboolean(world->lua->L, 1);
			world->lua->Call("E_SoundEmitter::Enable", 2, 0, 0);
		}
	}
	else if (distance > m_maxDistance && m_on)
	{
		m_on = false;
		if (PushEntityCall("Enable"))
		{
			lua_pushboolean(world->lua->L, 0);
			world->lua->Call("E_SoundEmitter::Disable", 2, 0, 0);
		}
	}
}

int E_SoundEmitter::Spawn(
	const Keys &keys,
	const xtime::TimeSlice &time,
	int flags
)
{
	E_SPAWN_BASE();
	m_maxDistance = keys.FloatForKey("maxDistance", 100.f);
	m_positional = keys.BoolForKey("positional", true);
	return pkg::SR_Success;
}

} // world

namespace spawn {

void *info_sound_emitter::Create()
{
	return new (ZWorld) world::E_SoundEmitter();
}

} // spawn
