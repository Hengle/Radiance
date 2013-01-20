/*! \file Floors.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#pragma once

#include "FloorsDef.h"
#include "WorldDef.h"
#include "BSPFile.h"
#include "../Physics/BezierSpline.h"
#include "../Lua/LuaRuntime.h"
#include <Runtime/Container/StackVector.h>
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/PushPack.h>

namespace world {

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS FloorPosition {
public:

	RAD_DECLARE_READONLY_PROPERTY(FloorPosition, pos, const Vec3&);
	RAD_DECLARE_READONLY_PROPERTY(FloorPosition, floor, int);
	RAD_DECLARE_READONLY_PROPERTY(FloorPosition, tri, int);
	RAD_DECLARE_READONLY_PROPERTY(FloorPosition, waypoint, int);
	RAD_DECLARE_READONLY_PROPERTY(FloorPosition, nextWaypoint, int);

private:

	friend class Floors;
	friend class FloorMove;
	friend struct lua::Marshal<FloorPosition>;

	void MakeNull() {
		m_pos = Vec3::Zero;
		m_floor = -1;
		m_tri = -1;
		m_waypoint = -1;
		m_nextWaypoint = -1;
	}

	RAD_DECLARE_GET(pos, const Vec3 &) {
		return m_pos;
	}

	RAD_DECLARE_GET(floor, int) {
		return m_floor;
	}

	RAD_DECLARE_GET(tri, int) {
		return m_tri;
	}

	RAD_DECLARE_GET(waypoint, int) {
		return m_waypoint;
	}

	RAD_DECLARE_GET(nextWaypoint, int) {
		return m_nextWaypoint;
	}

	Vec3 m_pos;
	int m_floor;
	int m_tri;
	int m_waypoint;
	int m_nextWaypoint;
};

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS FloorMove : public lua::SharedPtr {
public:
	typedef boost::shared_ptr<FloorMove> Ref;
	typedef physics::CachedCubicBZSpline<8> Spline;
	typedef zone_vector<String, ZWorldT>::type StringVec;

	struct Step {
		typedef stackify<std::vector<Step>, 8> Vec;
		Spline path;
		int waypoints[2];
		int connection;
		int flags;
		int floors[2];
		String events[2];
	};

	struct Route {
		Step::Vec steps;
	};

	class State {
	public:
		enum {
			RAD_FLAG(kStateFlag_AutoFace),
			RAD_FLAG(kStateFlag_Interruptable)
		};

		FloorPosition pos;
		Vec3 facing;
		int flags;
	private:
		friend class FloorMove;
		friend class Floors;
		Spline::SmoothMotion m_m;
		int m_stepIdx;
		bool m_first;
	};

	FloorMove();

	void InitMove(State &state);
	float Move(
		State &state, 
		float distance, 
		float &distanceRemainingAfterMove,
		StringVec &events
	);
	
	RAD_DECLARE_READONLY_PROPERTY(FloorMove, route, const Route*);

	void Merge(const Ref &old, State &state);

private:

	friend class Floors;

	RAD_DECLARE_GET(busy, bool);
	
	RAD_DECLARE_GET(route, const Route*) {
		return &m_route;
	}

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
	) const;

	FloorMove::Ref CreateMove(
		const FloorPosition &start,
		const FloorPosition &end
	) const;

	enum {
		RAD_FLAG(kFloorState_Enabled)
	};

	int FindFloor(const char *name) const;
	int FloorState(int floor) const;
	void SetFloorState(int floor, int state);

	enum {
		RAD_FLAG(kWaypointState_Enabled)
	};

	bool WaypointPosition(int waypoint, FloorPosition &pos) const;
	int WaypointState(int waypoint) const;
	void SetWaypointState(int waypoint, int state);

	IntVec WaypointsForTargetname(const char *targetname) const;
	IntVec WaypointsForUserId(const char *userId) const;

	int PickWaypoint(
		World &world,
		float x,
		float y,
		float d
	);

private:

	struct WalkStep {
		typedef stackify<std::vector<WalkStep>, 64> Vec;
		Vec3 pos;
		int tri;
		int connection;
		int flags;
		int waypoints[2];
		int floors[2];
		bool slopeChange;
	};

	//! Finds a route from start->end when start and end are on the same floor.
	void WalkFloor(
		const FloorPosition &start,
		const FloorPosition &end,
		WalkStep::Vec &route
	) const;

	void WalkConnection(
		int targetWaypoint,
		int connection,
		WalkStep::Vec &route
	) const;

	bool FindDirectRoute(const FloorPosition &start, const FloorPosition &end, WalkStep::Vec &route) const;
	void OptimizeRoute(WalkStep::Vec &route) const;
	void OptimizeRoute2(WalkStep::Vec &route) const;
	Vec3 FindEdgePoint(const Vec3 &pos, const bsp_file::BSPFloorEdge *edge) const;
	
	void GenerateFloorMove(const WalkStep::Vec &walkRoute, FloorMove::Route &moveRoute) const;

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
		FloorBits &floors,
		WaypointBits &waypoints,
		float &bestDistance
	) const;

	bool PlanFloorMove(
		const FloorPosition &start,
		const FloorPosition &end,
		float distance,
		MovePlan &plan,
		MovePlan &planSoFar,
		FloorBits &floors,
		WaypointBits &waypoints,
		float &bestDistance
	) const;

	bool PlanWaypointMove(
		const FloorPosition &start,
		const FloorPosition &end,
		float distance,
		MovePlan &plan,
		MovePlan &planSoFar,
		FloorBits &floors,
		WaypointBits &waypoints,
		float &bestDistance
	) const;

	//! Clips a ray to the specified floor.
	/*! Returns true and the position of the closest intersection with the floor, otherwise false. */
	bool ClipToFloor(
		U32 floorNum,
		const Vec3 &start,
		const Vec3 &end,
		FloorPosition &pos,
		float &bestDistSq
	) const;

	struct Waypoint {
		typedef zone_multimap<String, int, ZWorldT>::type MMap;
		typedef zone_vector<Waypoint, ZWorldT>::type Vec;

		String targetName;
		String userId;
		int waypointId;
		int flags;
	};

	Waypoint::Vec m_waypoints;
	Waypoint::MMap m_waypointTargets;
	Waypoint::MMap m_waypointUserIds;
	IntVec m_floorState;
	bsp_file::BSPFile::Ref m_bsp;
};

} // world

namespace lua {

template <>
struct Marshal<world::FloorPosition> {
	static void Push(lua_State *L, const world::FloorPosition &val);
	static world::FloorPosition Get(lua_State *L, int index, bool luaError);
	static bool IsA(lua_State *L, int index);
};

} // lua

#include <Runtime/PopPack.h>
