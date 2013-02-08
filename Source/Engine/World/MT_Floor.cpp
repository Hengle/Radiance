/*! \file MT_Floor.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#include RADPCH
#include "Entity.h"
#include "World.h"

namespace world {

void Entity::TransitionFloorMove() {
	if (!m_ps.desiredMove)
		return;

	if (m_ps.activeMove) {
		m_ps.desiredMove->Merge(m_ps.activeMove, m_ps.moveState);
		m_ps.activeMove = m_ps.desiredMove;
		m_ps.desiredMove.reset();
	} else {
		m_ps.activeMove = m_ps.desiredMove;
		m_ps.desiredMove.reset();
		m_ps.activeMove->InitMove(m_ps.moveState);
	}
}

void Entity::CancelFloorMove() {
	m_ps.activeMove.reset();
}

void Entity::Tick_MT_Floor(
	int frame,
	float dt,
	const xtime::TimeSlice &time
) {
	m_ps.distanceMoved = 0.f;
	TransitionFloorMove();

	if (!m_ps.activeMove) {
		m_ps.velocity = Vec3::Zero;
		m_ps.distanceMoved = 0.f;
		Move();
		return;
	}

	UpdateVelocity(dt);

	float moveLen = m_ps.velocity.Magnitude() * dt;

	if (moveLen < 0.01f) {
		m_ps.distanceMoved = 0.f;
		return;
	}

	float distanceRemaining;
	StringVec events;

	m_ps.distanceMoved = m_ps.activeMove->Move(m_ps.moveState, moveLen, distanceRemaining, events);
	m_ps.origin = m_ps.moveState.pos.pos.get() + m_ps.bbox.Origin();

	if (!events.empty()) {
		for (StringVec::const_iterator it = events.begin(); it != events.end(); ++it) {
			if (!(*it).empty)
				world->PostEvent((*it).c_str);
		}
	}

	if (distanceRemaining < m_ps.autoDecelDistance) {
		m_ps.flags |= kPhysicsFlag_Friction;
	}

	if (distanceRemaining < 0.01f) {
		m_ps.activeMove->ClampToEnd(m_ps.moveState);
		m_ps.activeMove.reset(); // done with this move.
	}

	m_ps.targetAngles = LookAngles(m_ps.moveState.facing);
	m_ps.targetAngles[0] = 0.f;
	m_ps.targetAngles[1] = 0.f;
	SeekAngles(dt);
	Move();
}

} // world
