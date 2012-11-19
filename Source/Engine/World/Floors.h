/*! \file Floors.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#pragma once

#include "../Types.h"
#include "BSPFile.h"
#include "../Physics/BezierSpline.h"
#include <Runtime/Container/StackVector.h>
#include <Runtime/PushPack.h>
#include <bitset>

namespace world {

class World;
class Floors;

enum {
	kMaxFloors = 64,
	kMaxFloorTris = 2048
};

typedef std::bitset<kMaxFloors> FloorBits;
typedef std::bitset<kMaxFloorTris> FloorTriBits;

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS FloorPosition {
public:

	RAD_DECLARE_READONLY_PROPERTY(FloorPosition, pos, const Vec3&);
	RAD_DECLARE_READONLY_PROPERTY(FloorPosition, floor, int);
	RAD_DECLARE_READONLY_PROPERTY(FloorPosition, tri, int);

private:
	friend class Floors;

	RAD_DECLARE_GET(pos, const Vec3 &) {
		return m_pos;
	}

	RAD_DECLARE_GET(floor, int) {
		return m_floor;
	}

	RAD_DECLARE_GET(tri, int) {
		return m_tri;
	}

	Vec3 m_pos;
	int m_floor;
	int m_tri;
};

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS FloorMove {
public:
	typedef boost::shared_ptr<FloorMove> Ref;
	typedef physics::CachedCubicBZSpline<8> Spline;

	struct Step {
		typedef stackify<std::vector<Step>, 8> Vec;
		Spline path;
		int waypoints[2];
		int floors[2];
	};

	struct Route {
		Step::Vec steps;
	};

	FloorMove();

	void InitMove(
		FloorPosition &pos,
		String &animState
	);

	bool Move(
		FloorPosition &pos,
		float velocity,
		String &animState
	);

	RAD_DECLARE_READONLY_PROPERTY(FloorMove, busy, bool);
	RAD_DECLARE_READONLY_PROPERTY(FloorMove, route, const Route*);

private:

	friend class Floors;

	RAD_DECLARE_GET(busy, bool);
	
	RAD_DECLARE_GET(route, const Route*) {
		return &m_route;
	}

	FloorPosition m_pos;
	Route m_route;
};

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS Floors {
public:

	Floors();
	~Floors();

	bool Load(const bsp_file::BSPFile::Ref &bsp);

	bool ClipToFloor(
		const Vec3 &start,
		const Vec3 &end,
		FloorPosition &pos
	);

	FloorMove::Ref CreateMove(
		const FloorPosition &start,
		const FloorPosition &end
	);

private:

	struct WalkStep {
		typedef stackify<std::vector<WalkStep>, 64> Vec;
		Vec3 pos;
		int connection;
		int waypoints[2];
		int floors[2];
		bool required;
	};

	//! Finds a route from start->end when start and end are on the same floor.
	void WalkFloor(
		const FloorPosition &start,
		const FloorPosition &end,
		WalkStep::Vec &route
	);

	void WalkConnection(
		int waypoint,
		int connection,
		WalkStep::Vec &route
	);

	bool FindDirectRoute(const FloorPosition &start, const FloorPosition &end, WalkStep::Vec &route);
	void OptimizeRoute(const FloorPosition &start, WalkStep::Vec &route);
	
	void GenerateFloorMove(const WalkStep::Vec &walkRoute, FloorMove::Route &moveRoute);

	//! A step in a planned move
	struct MoveStep {
		typedef stackify<std::vector<MoveStep>, 8> Vec;
		int waypoint;
		int connection;
	};

	struct MovePlan {
		FloorPosition start;
		FloorPosition end;
		MoveStep::Vec steps;
	};

	//! Find shortest path from start->end
	bool PlanMove(
		const FloorPosition &start,
		const FloorPosition &end,
		float distance,
		MovePlan &plan,
		MovePlan &planSoFar,
		FloorBits stack,
		float &bestDistance
	);

	//! Clips a ray to the specified floor.
	/*! Returns true and the position of the closest intersection with the floor, otherwise false. */
	bool ClipToFloor(
		U32 floorNum,
		const Vec3 &start,
		const Vec3 &end,
		FloorPosition &pos,
		float &bestDistSq
	);

	bsp_file::BSPFile::Ref m_bsp;
};

} // world

#include <Runtime/PopPack.h>
