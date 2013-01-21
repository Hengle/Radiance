// MT_Fly.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "Entity.h"

namespace world {

void Entity::Tick_MT_Fly(
	int frame, 
	float dt, 
	const xtime::TimeSlice &time
)
{
	m_ps.flags &= ~kPhysicsFlag_OnGround; // flying

	if (m_ps.flags&kPhysicsFlag_SeekAngles)
		SeekAngles(dt);

	if (m_ps.flags&kPhysicsFlag_AutoFace)
		AutoFace(dt);

	UpdateVelocity(dt);

	Entity::Ref parent = m_ps.parent.lock();
	if (parent)
	{
		TickOther(*parent, frame, dt, time);
		m_ps.origin = parent->ps->worldPos;
		m_ps.originAngles = parent->ps->worldAngles;
	}

	m_ps.pos = ApplyVelocity(dt);
	Move();

	m_ps.cameraPos = m_ps.pos + m_ps.origin;
	m_ps.cameraAngles = m_ps.worldAngles;
}

} // world
