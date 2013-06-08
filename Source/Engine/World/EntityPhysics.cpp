/*! \file EntityPhysics.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#include RADPCH
#include "Entity.h"
#include <algorithm>

namespace world {

void Entity::UpdateVelocity(float dt) {
	// velocity
	if (m_ps.flags&kPhysicsFlag_Friction) {
		float mag = m_ps.velocity.Magnitude();
		if (mag > 0.01f) {
			float decel;
			if (m_ps.mtype == kMoveType_Floor) {
				decel = m_ps.groundFriction;
			} else {
				decel = m_ps.airFriction;
			}

			decel = mag - dt*decel;
			if (decel < 0.f)
				decel = 0.f;
			decel /= mag;
			m_ps.velocity *= decel;
		} else {
			m_ps.velocity = Vec3::Zero;
		}
	} else {
		m_ps.velocity += m_ps.accel * dt;

		float mag = m_ps.velocity.MagnitudeSquared();
		float max;

		if (m_ps.mtype == kMoveType_Floor) {
			max = m_ps.maxGroundSpeed;
		} else {
			max = m_ps.maxAirSpeed;
		}
			
		if (mag > max*max) { 
			// clamp
			m_ps.velocity.Normalize();
			m_ps.velocity *= max;
		}
	}
}

void Entity::AutoFace(float dt) {
	Mat4 m = Mat4::Rotation(QuatFromAngles(m_ps.worldAngles));
	float mag = m_ps.velocity.Magnitude();
	m_ps.velocity = Vec3(1, 0, 0) * m;
	m_ps.velocity *= mag;

	mag = m_ps.accel.Magnitude();
	m_ps.accel = Vec3(1, 0, 0) * m;
	m_ps.accel *= mag;
}

Vec3 Entity::ApplyVelocity(float dt) {
	return m_ps.pos + m_ps.velocity * dt;
}

void Entity::Move() {
	m_ps.worldAngles = WrapAngles(m_ps.angles.pos + m_ps.originAngles);
	m_ps.worldPos = m_ps.pos + m_ps.origin;
	m_ps.cameraPos = m_ps.worldPos + m_ps.cameraShift;
	m_ps.cameraAngles = m_ps.worldAngles;
	Link();
}

void Entity::SkaMove() {
	if (m_ps.motionSka) {
		const ska::BoneTM *delta = m_ps.motionSka->ska->deltaMotion;
		Vec3 t;
		Mat4 m = Mat4::Rotation(QuatFromAngles(m_ps.targetAngles));
		t = m.Transform3X3(delta->t);
		m_ps.origin += t * m_ps.motionScale;
		m_ps.velocity = Vec3::Zero;
		m_ps.distanceMoved = 0.f;
		Move();
	}
}

void Entity::SeekAngles(float dt) {
	// Need to do some stuff to fixup so the spring system works here
	// Because angles need to lerp shortest path.

	// To do this, treat the angles as if they are in a different coordinate
	// space than the actual spring, and map the spring length to the angle lerp.

	m_ps.angles.pos = DeltaAngles(m_ps.angles.pos, m_ps.targetAngles);

	for (int i = 0; i < 3; ++i) {
		if (m_ps.angles.pos[i] > m_ps.snapTurnAngles[i])
			m_ps.angles.pos[i] = 0.f; // snap to facing
		if (m_ps.angles.pos[i] < -m_ps.snapTurnAngles[i])
			m_ps.angles.pos[i] = 0.f;
	}

	Vec3 x = m_ps.angles.pos;

	if (!m_ps.angles.Update(dt, Vec3::Zero, m_ps.angleSpring)) {
		m_ps.angles.pos = m_ps.targetAngles;
	} else {

		Vec3 angles = WrapAngles(m_ps.targetAngles - m_ps.angles.pos);

		for (int i = 0; i < 3; ++i) {
			if (angles[i] < 0)
				angles[i] += 360.f;
			if (angles[i] > 360.f)
				angles[i] -= 360.f;
		}

//		COut(C_Debug) << "Target: (" << m_ps.targetAngles[2] << "), Angles: (" << angles[2] << "), Delta: (" << x[2] << ", " << m_ps.angles.pos[2] << ")" << std::endl;
		m_ps.angles.pos = angles;
	}
}

void Entity::TickPhysics(
	int frame, 
	float dt, 
	const xtime::TimeSlice &time
) {
	switch (m_ps.mtype) {
	case kMoveType_None:
		m_ps.distanceMoved = -1.f;
		m_ps.worldPos = m_ps.origin + m_ps.pos;
		m_ps.worldAngles = WrapAngles(m_ps.originAngles + m_ps.angles.pos);
		m_ps.cameraPos = m_ps.worldPos + m_ps.cameraShift;
		m_ps.cameraAngles = m_ps.worldAngles;
		break;
	case kMoveType_Fly:
		Tick_MT_Fly(frame, dt, time);
		break;
	case kMoveType_Spline:
		Tick_MT_Spline(frame, dt, time);
		break;
	case kMoveType_Floor:
		Tick_MT_Floor(frame, dt, time);
		break;
	case kMoveType_Ska:
		SkaMove();
		break;
	}
}

} // world
