// EntityPhysics.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "Entity.h"
#include <algorithm>

namespace world {

void Entity::UpdateVelocity(float dt)
{
	// velocity
	if (m_ps.flags&PF_Friction)
	{
		float mag = m_ps.velocity.Magnitude();
		if (mag > 0.f)
		{
			float decel = mag < m_ps.airFriction ? mag : m_ps.airFriction;
			decel = mag - dt*decel;
			if (decel < 0.f)
				decel = 0.f;
			decel /= mag;
			m_ps.velocity *= decel;
		}
	}
	else
	{
		m_ps.velocity += m_ps.accel * dt;
		float mag = m_ps.velocity.MagnitudeSquared();
		if (mag > m_ps.maxAirSpeed*m_ps.maxAirSpeed)
		{ // clamp
			m_ps.velocity.Normalize();
			m_ps.velocity *= m_ps.maxAirSpeed;
		}
	}
}

void Entity::AutoFace(float dt)
{
	Mat4 m = Mat4::Rotation(QuatFromAngles(m_ps.worldAngles));
	float mag = m_ps.velocity.Magnitude();
	m_ps.velocity = Vec3(1, 0, 0) * m;
	m_ps.velocity *= mag;

	mag = m_ps.accel.Magnitude();
	m_ps.accel = Vec3(1, 0, 0) * m;
	m_ps.accel *= mag;
}

Vec3 Entity::ApplyVelocity(float dt)
{
	return m_ps.pos + m_ps.velocity * dt;
}

void Entity::Move(bool touch, bool clip)
{
	Vec3 newPos;

	switch (m_psv.motionType)
	{
	case ska::Ska::MT_Relative:
		m_ps.angles.pos = WrapAngles(AnglesFromQuat(m_psv.motion.r) + m_ps.angles.pos);
		m_ps.worldAngles = WrapAngles(m_ps.angles.pos + m_ps.originAngles);
		m_ps.pos += RotateVector(m_psv.motion.t, m_ps.worldAngles);
		newPos = m_ps.pos + m_ps.origin;
		break;
	case ska::Ska::MT_Absolute:
		m_ps.worldAngles = WrapAngles(m_ps.angles.pos + m_ps.originAngles + AnglesFromQuat(m_psv.motion.r));
		newPos = m_ps.pos + RotateVector(m_psv.motion.t, m_ps.worldAngles) + m_ps.origin;
		break;
	default:
		newPos = m_ps.pos + m_ps.origin;
		m_ps.worldAngles = WrapAngles(m_ps.angles.pos + m_ps.originAngles);
		break;
	}

	// TODO: touch / collision
	m_ps.worldPos = newPos;
}

void Entity::SeekAngles(float dt)
{
	// Need to do some stuff to fixup so the spring system works here
	// Because angles need to lerp shortest path.

	// To do this, treat the angles as if they are in a different coordinate
	// space than the actual spring, and map the spring length to the angle lerp.

	m_ps.angles.inner = false;
	m_ps.angles.outer = true;
	m_ps.angleSpring.length = 0.07f;
	m_ps.angleSpring.tolerance = 0.001f;

	Vec3 x = m_ps.angles.pos;
	m_ps.angles.pos = DeltaAngles(m_ps.angles.pos, m_ps.targetAngles);

	if (!m_ps.angles.Update(dt, Vec3::Zero, m_ps.angleSpring))
	{
		m_ps.angles.pos = m_ps.targetAngles;
	}
	else
	{
		m_ps.angles.pos = WrapAngles(m_ps.targetAngles - m_ps.angles.pos);
		for (int i = 0; i < 3; ++i)
		{
			if (m_ps.angles.pos[i] < 0)
				m_ps.angles.pos[i] += 360.f;
			if (m_ps.angles.pos[i] > 360.f)
				m_ps.angles.pos[i] -= 360.f;
		}
	}
}

void Entity::TickPhysics(
	int frame, 
	float dt, 
	const xtime::TimeSlice &time
)
{
	switch (m_ps.mtype)
	{
	case MT_None:
		m_ps.worldPos = m_ps.origin + m_ps.pos;
		m_ps.worldAngles = WrapAngles(m_ps.originAngles + m_ps.angles.pos);
		m_ps.cameraPos = m_ps.worldPos;
		m_ps.cameraAngles = m_ps.worldAngles;
		break;
	case MT_Fly:
		Tick_MT_Fly(frame, dt, time);
		break;
	case MT_Spline:
		Tick_MT_Spline(frame, dt, time);
		break;
	}
}

} // world
