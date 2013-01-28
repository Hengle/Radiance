// MT_Spline.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "Entity.h"
#include "Entities/E_SplineTrack.h"
#include "World.h"
#include <algorithm>

namespace world {

void Entity::Tick_MT_Spline(
	int frame, 
	float dt, 
	const xtime::TimeSlice &time
)
{
	E_SplineTrack::Ref spline = boost::dynamic_pointer_cast<E_SplineTrack>(m_ps.parent.lock());
	if (!spline)
		return;

	if (spline->id != m_psv.splineId || (m_ps.flags&kPhysicsFlag_ResetSpline))
	{
		m_psv.splineId = spline->id;
		m_psv.splineIdx = 0;
		if (m_ps.flags&kPhysicsFlag_ResetSpline)
			m_ps.splineDistance = 0.f;
		m_ps.flags &= ~kPhysicsFlag_ResetSpline;
	}

	m_ps.flags &= ~kPhysicsFlag_OnGround; // flying

	UpdateVelocity(dt);

	// how much we want to move this frame
	m_ps.splineDistance += m_ps.velocity.Magnitude() * dt;

	Vec3 fwd, pos, rate;
	StringVec events;

	bool end = spline->Eval(
		m_psv.splineIdx,
		m_ps.splineDistance,
		pos,
		fwd,
		rate,
		events
	);

	if (m_ps.flags&kPhysicsFlag_SplineEvents)
	{
		for (StringVec::const_iterator it = events.begin(); it != events.end(); ++it)
		{
			world->DispatchEvent((*it).c_str);
		}
	}

	events.clear();

	if (end)
	{
		Entity::Ref next;

		float distance = m_ps.splineDistance;

		if (PushEntityCall("OnEndOfTrack"))
		{
			if (world->lua->Call("Tick_MT_Spline", 1, 1, 0))
			{
				Entity *x = world->lua->EntFramePtr(-1, false);
				if (x)
					next = x->shared_from_this();
				lua_pop(world->lua->L, 1);
				m_ps.splineDistance = distance;
			}
		}

		E_SplineTrack::Ref nextSpline = boost::dynamic_pointer_cast<E_SplineTrack>(next);

		if (nextSpline && nextSpline->id != spline->id)
		{ // changed tracks
			m_ps.parent = nextSpline;
			m_psv.splineId = nextSpline->id;
			m_psv.splineIdx = 0;
			m_ps.flags &= ~kPhysicsFlag_ResetSpline;
			
			nextSpline->Eval(
				m_psv.splineIdx,
				m_ps.splineDistance, // note, distance stores amount off end.
				pos,
				fwd,
				rate,
				events
			);
		}
		else if (!nextSpline)
		{
			if (m_ps.flags&kPhysicsFlag_LoopSpline)
			{
				m_psv.splineIdx = 0;
				// note that m_psv.splineDistance contains the amount off the end...
				spline->Eval(
					m_psv.splineIdx,
					m_ps.splineDistance,
					pos,
					fwd,
					rate,
					events
				);
			}
		}
	}

	m_ps.origin = Vec3::Zero;
	m_ps.pos = pos;
	
	if (m_ps.flags&(kPhysicsFlag_SplineBank|kPhysicsFlag_FlipSplineBank))
	{
		float oldBank = m_ps.angles.pos[0];
		m_ps.originAngles = Vec3::Zero;
		m_ps.angles.pos = LookAngles(fwd);

		// rate contains the curve direction, forward contains the tangent vector.
		// bank = sign * 1-cos(angle)

		rate[2] = 0.f; // drop Z
		fwd[2] = 0.f;
		rate.Normalize();
		fwd.Normalize();

		Vec3 bankVec = Vec3::Zero;
		bankVec[0] = rate.Dot(fwd);
		std::swap(rate[0], rate[1]);
		rate[0] = -rate[0];
		bankVec[1] = rate.Dot(fwd);

		bankVec.Normalize();
		float bank = -math::RadToDeg(math::ArcTan2(bankVec[1], bankVec[0]))*m_ps.splineBankScale;
		bank = math::Clamp(bank, -m_ps.maxSplineBank, m_ps.maxSplineBank);

		if (m_ps.flags&kPhysicsFlag_FlipSplineBank)
			bank = -bank;

		if (m_ps.splineBankLerp > 0.f)
		{
			float lerp = math::Clamp(m_ps.splineBankLerp*dt, 0.f, 0.9999f);
			m_ps.angles.pos[0] = math::Lerp(oldBank, bank, lerp);
		}
		else
		{
			m_ps.angles.pos[0] = bank;
		}
	}
	else
	{
		m_ps.angles.pos = LookAngles(fwd);
	}

	Move();

	if (m_ps.flags&kPhysicsFlag_SplineEvents)
	{
		for (StringVec::const_iterator it = events.begin(); it != events.end(); ++it)
		{
			world->DispatchEvent((*it).c_str);
		}
	}
}
	
} // world
