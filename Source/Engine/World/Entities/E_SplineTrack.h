// E_SplineTrack.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "../Entity.h"
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/PushPack.h>

namespace world {

class RADENG_CLASS E_SplineTrack : public Entity
{
	E_DECL_BASE(Entity);
public:

	typedef boost::shared_ptr<E_SplineTrack> Ref;

	E_SplineTrack();
	virtual ~E_SplineTrack();

	virtual int Spawn(
		const Keys &keys,
		const xtime::TimeSlice &time,
		int flags
	);

	// Evaluate the spline track starting at index point "idx", and go
	// for "distance" distance.
	// Returns the pos and facing and events.
	// If true is returned then the end of the spline was reached and distance
	// is the distance off the end.
	bool Eval(
		int &idx, 
		float &distance, 
		Vec3 &pos, 
		Vec3 &fwd,
		Vec3 &rate,
		StringVec &outEvents
	) const;

private:

	struct Point
	{
		typedef zone_vector<Point, ZWorldT>::type Vec;
		Vec3 pos;
		Vec3 fwd;
		Vec3 rate;
		StringVec events;
		float time;
		float distance[2];
	};

	struct Event
	{
		typedef zone_vector<Event, ZWorldT>::type Vec;
		float time;
		StringVec tags;
	};

	void LoadCurve(const Keys &keys);
	void Eval(const Vec3 *ctrls, const Vec3 *last, float u, Vec3 &pos, Vec3 &fwd, Vec3 &rate);
	StringVec SplitEvents(const char *str);

	Point::Vec m_points;
};

} // world

E_DECL_SPAWN(RADENG_API, info_spline_track)

#include <Runtime/PopPack.h>

