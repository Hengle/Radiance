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

	m_ps.moveState.moveAnim = false;
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
	int oldFlags = m_ps.moveState.flags;
	TransitionFloorMove();

	if (!m_ps.activeMove) {
		m_ps.velocity = Vec3::Zero;
		m_ps.distanceMoved = 0.f;
		SeekAngles(dt);
		Move();
		return;
	}

	if (m_ps.moveState.moveAnim) {
		SeekAngles(dt);
		SkaMove((m_ps.moveState.flags & FloorMove::State::kStateFlag_AutoFace) ? true : false);
		return;
	}

	UpdateVelocity(dt);

	float moveLen = m_ps.velocity.Magnitude() * dt;

	if (moveLen < 0.01f) {
		m_ps.distanceMoved = 0.f;
		SeekAngles(dt);
		Move();
		return;
	}

	float distanceRemaining;
	StringVec events;
	String moveAnim;

	m_ps.distanceMoved = m_ps.activeMove->Move(m_ps.moveState, moveLen, distanceRemaining, events, moveAnim);
	m_ps.origin = m_ps.moveState.pos.pos.get() - Vec3(0, 0, m_ps.bbox.Mins()[2]); // put bbox on floor.

	if (!events.empty()) {
		for (StringVec::const_iterator it = events.begin(); it != events.end(); ++it) {
			if (!(*it).empty)
				world->PostEvent((*it).c_str);
		}
	}

	if (distanceRemaining < m_ps.autoDecelDistance) {
		m_ps.flags |= kPhysicsFlag_Friction;
	}

	if (distanceRemaining < 1.f) {
		m_ps.activeMove->ClampToEnd(m_ps.moveState);
		m_ps.activeMove.reset(); // done with this move.

		if (PushEntityCall("OnFloorMoveComplete")) {
			world->lua->Call("Entity::OnFloorMoveComplete", 1, 0, 0);
		}
	}

	m_ps.targetAngles = LookAngles(m_ps.moveState.facing);
	m_ps.targetAngles[0] = 0.f;

	if (!(m_ps.moveState.flags&FloorMove::State::kStateFlag_AutoPitch))
		m_ps.targetAngles[1] = 0.f;
	
	if (!(oldFlags&FloorMove::State::kStateFlag_AutoFace) && (m_ps.moveState.flags&FloorMove::State::kStateFlag_AutoFace)) {
		// sync orientation
		m_ps.angles.pos = m_ps.targetAngles;
	} else if (!(m_ps.moveState.flags&FloorMove::State::kStateFlag_AutoFace)) {
		// hint to let animation control this.
		m_ps.targetAngles = Vec3::Zero;
		m_ps.angles.pos = Vec3::Zero;
	}

	SeekAngles(dt);
	Move();

	if (m_ps.moveState.moveAnim) {
		bool init = false;
		if (m_ps.activeMove) {
			if (PushEntityCall("CustomAnimMove")) {
				lua_pushstring(world->lua->L, moveAnim.c_str);
				if (world->lua->Call("Entity::CustomAnimMove", 2, 0, 0)) {
					init = true;
					m_ps.velocity = Vec3::Zero;
				}
			}
		}
		if (!init)
			m_ps.moveState.moveAnim = false;
	}
}

void Entity::CustomMoveComplete() {
	m_ps.moveState.moveAnim = false;
	if (m_ps.activeMove) {
		StringVec events;
		if (m_ps.activeMove->NextStep(m_ps.moveState, events)) {
			// move is complete
			m_ps.activeMove->ClampToEnd(m_ps.moveState);
			m_ps.activeMove.reset(); // done with this move.

			if (PushEntityCall("OnFloorMoveComplete")) {
				world->lua->Call("Entity::OnFloorMoveComplete", 1, 0, 0);
			}
		}

		m_ps.origin = m_ps.moveState.pos.pos.get() - Vec3(0, 0, m_ps.bbox.Mins()[2]); // put bbox on floor.

		if (!events.empty()) {
			for (StringVec::const_iterator it = events.begin(); it != events.end(); ++it) {
				if (!(*it).empty)
					world->PostEvent((*it).c_str);
			}
		}
	}
}

} // world
