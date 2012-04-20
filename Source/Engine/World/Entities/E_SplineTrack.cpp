// E_SplineTrack.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "E_SplineTrack.h"
#include "../World.h"
#include <algorithm>
#include <stdio.h>

namespace world {

enum { SplineDiv=64 };

E_SplineTrack::E_SplineTrack() : E_CONSTRUCT_BASE
{
}

E_SplineTrack::~E_SplineTrack()
{
}

int E_SplineTrack::Spawn(
	const Keys &keys,
	const xtime::TimeSlice &time,
	int flags
)
{
	E_SPAWN_BASE();
	LoadCurve(keys);

	lua_State *L = world->lua->L;
	PushEntityFrame(L);
	
	if (m_points.empty())
		lua_pushnumber(L, (lua_Number)0.0);
	else
		lua_pushnumber(L, (lua_Number)m_points.back().distance[1]);
	lua_setfield(L, -2, "splineLength");
	lua_pop(L, 1); // frame

	return pkg::SR_Success;
}

bool E_SplineTrack::Eval(
	int &idx, 
	float &distance, 
	Vec3 &pos, 
	Vec3 &fwd,
	Vec3 &rate,
	StringVec &outEvents
) const
{
	if (distance < 0.f)
		return false;

	if (idx >= (int)m_points.size() || m_points.size() < 2)
		return true;

	while (idx < (int)m_points.size() && distance >= m_points[idx].distance[1])
	{
		std::copy(
			m_points[idx].events.begin(),
			m_points[idx].events.end(),
			std::back_inserter(outEvents)
		);
		++idx;
	}

	if (idx >= (int)m_points.size())
	{
		// off end.
		if (m_points.empty())
			return true;
		distance -= m_points[idx-1].distance[1];
		pos = m_points[idx-1].pos;
		fwd = m_points[idx-1].fwd;
		rate = m_points[idx-1].rate;
		return true;
	}

	RAD_ASSERT(idx>0);

	const Point &p = m_points[idx-1];
	const Point &p2 = m_points[idx];
	RAD_ASSERT(distance >= p.distance[1]);
	float lerp = distance-p.distance[1];
	if (lerp > 0.f && p2.distance[0] > 0.f)
	{
		lerp /= p2.distance[0];
		pos = math::Lerp(p.pos, p2.pos, lerp);
		fwd = math::Lerp(p.fwd, p2.fwd, lerp);
		rate = math::Lerp(p.rate, p2.rate, lerp);
	}
	else
	{
		pos = p.pos;
		fwd = p.fwd;
		rate = p.rate;
	}
	
	fwd.Normalize();
	return false;
}

void E_SplineTrack::LoadCurve(const Keys &keys)
{
	BOOST_STATIC_ASSERT(sizeof(Vec3)==sizeof(float)*3);
	char sz[64];

	size_t numSplines = 0;
	for (int i = 0;; ++i)
	{
		string::sprintf(sz, "segment %d", i);
		const char *spline = keys.StringForKey(sz);
		if (!spline)
			break;
		++numSplines;
	}

	Event::Vec events;
	events.reserve(4);

	for (int i = 0;; ++i)
	{
		string::sprintf(sz, "event_keyframe_info %d", i);
		const char *event = keys.StringForKey(sz);
		if (!event)
			break;
		Event e;
		e.time = string::atof(event);

		string::sprintf(sz, "event_keyframe_data %d", i);
		event = keys.StringForKey(sz);
		if (!event)
			break;
		e.tags = SplitEvents(event);

		if (!e.tags.empty())
			events.push_back(e);
	}

	int eventIdx = 0;
	float eventTime = -1.f;

	if (!events.empty())
		eventTime = events[0].time;

	m_points.reserve(numSplines*SplineDiv+events.size());

	float time = 0.f;
	float distance = 0.f;
	bool firstPoint = true;
	Vec3 lastPos;
	Vec3 lastFwd;
	Vec3 ctrls[4];

	for (int i = 0;; ++i)
	{
		string::sprintf(sz, "segment %d", i);
		const char *spline = keys.StringForKey(sz);
		if (!spline)
		{ // collect any trailing events.
			if (!m_points.empty())
			{
				Point &p = m_points.back();

				while (eventIdx < (int)events.size())
				{
					std::copy(
						events[eventIdx].tags.begin(),
						events[eventIdx].tags.end(),
						std::back_inserter(p.events)
					);
					++eventIdx;
				}
			}
			break;
		}
		float splineTime;
		float *f = (float*)ctrls;
		sscanf(
			spline, 
			"( %f %f %f ) ( %f %f %f ) ( %f %f %f ) ( %f %f %f ) %f",
			&f[0], &f[1], &f[2],
			&f[3], &f[4], &f[5],
			&f[6], &f[7], &f[8],
			&f[9], &f[10], &f[11],
			&splineTime
		);

		if (splineTime <= 0.f)
			continue;

		float u = 0.f;
		float uStep = splineTime / (SplineDiv-1);
		
		for (int k = 0; k < SplineDiv; ++k, u += uStep)
		{
			if (i > 0 && k == 0)
				continue; // always skip the first one here.
				
			while (eventTime != -1.f && time+u >= eventTime )
			{ // insert event.
				Point p;

				p.time = events[eventIdx].time;
				p.events = events[eventIdx].tags;
				++eventIdx;
				if (eventIdx < (int)events.size())
					eventTime = events[eventIdx].time;
				else
					eventTime = -1.f;

				Eval(ctrls, firstPoint ? 0 : &lastFwd, (p.time-time) / splineTime, p.pos, p.fwd, p.rate);

				if (firstPoint)
				{
					p.distance[0] = p.distance[1] = 0.f;
					firstPoint = false;
				}
				else
				{
					p.distance[0] = (p.pos - lastPos).Magnitude();
					distance += p.distance[0];
					p.distance[1] = distance;
				}

				lastPos = p.pos;
				lastFwd = p.fwd;

				m_points.push_back(p);
			}

			Point p;
			Eval(ctrls, firstPoint ? 0 : &lastFwd, u / splineTime, p.pos, p.fwd, p.rate);
			p.time = time+u;

			if (firstPoint)
			{
				p.distance[0] = p.distance[1] = 0.f;
				firstPoint = false;
			}
			else
			{
				p.distance[0] = (p.pos - lastPos).Magnitude();
				distance += p.distance[0];
				p.distance[1] = distance;
			}

			lastPos = p.pos;
			lastFwd = p.fwd;
			m_points.push_back(p);
		}

		time += splineTime;
	}
}

void E_SplineTrack::Eval(const Vec3 *ctrls, const Vec3 *last, float u, Vec3 &pos, Vec3 &fwd, Vec3 &rate)
{
	Vec3 a = -ctrls[0] + 3.f*ctrls[1] - 3.f*ctrls[2] + ctrls[3];
	Vec3 b = 3.f*ctrls[0] - 6.f*ctrls[1] + 3.f*ctrls[2];
	Vec3 c = -3.f*ctrls[0] + 3.f*ctrls[1];
	Vec3 d = ctrls[0];

	float u_sq = u*u;
	float u_cb = u*u_sq;

	pos = a*u_cb + b*u_sq + c*u + d;
	fwd = 3.f*a*u_sq + 2.f*b*u + c;
	
	fwd.Normalize();
	
	if (last)
		rate = *last;
	else
		rate = fwd;
}

E_SplineTrack::StringVec E_SplineTrack::SplitEvents(const char *str)
{
	String s;
	StringVec v;

	while (*str)
	{
		if (str[0] == '\n') // end of line
		{
			if (!s.empty())
				v.push_back(s);
			s.clear();
		}
		else
		{
			s += *str;
		}

		++str;
	}

	if (!s.empty())
		v.push_back(s);

	return v;
}

} // world

namespace spawn {

void *info_spline_track::Create()
{
	return new (ZWorld) world::E_SplineTrack();
}

} // spawn

