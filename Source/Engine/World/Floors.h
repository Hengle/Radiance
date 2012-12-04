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
#include "../Lua/LuaRuntime.h"
#include <Runtime/Container/StackVector.h>
#include <Runtime/PushPack.h>
#include <bitset>

namespace world {

class World;
class Floors;
class FloorPosition;

enum {
	kMaxFloors = 64,
	kMaxFloorTris = 2048
};

typedef std::bitset<kMaxFloors> FloorBits;
typedef std::bitset<kMaxFloorTris> FloorTriBits;

} // world

namespace lua {

template <>
struct Marshal<world::FloorPosition> {
	static void Push(lua_State *L, const world::FloorPosition &val);
	static world::FloorPosition Get(lua_State *L, int index, bool luaError);
	static bool IsA(lua_State *L, int index);
};

} // lua

namespace world {

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS FloorPosition {
public:

	RAD_DECLARE_READONLY_PROPERTY(FloorPosition, pos, const Vec3&);
	RAD_DECLARE_READONLY_PROPERTY(FloorPosition, floor, int);
	RAD_DECLARE_READONLY_PROPERTY(FloorPosition, tri, int);

private:
	friend class Floors;
	friend struct lua::Marshal<FloorPosition>;

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

class RADENG_CLASS FloorMove : public lua::SharedPtr {
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

	class State {
	public:
		FloorPosition pos;
		String anim;
		Vec3 facing;
		int flags;
	private:
		friend class FloorMove;
		int m_stepIdx;
		float m_t;
	};

	FloorMove();

	void InitMove(State &state);
	bool Move(State &state, float velocity);

	RAD_DECLARE_READONLY_PROPERTY(FloorMove, busy, bool);
	RAD_DECLARE_READONLY_PROPERTY(FloorMove, route, const Route*);

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

private:

	struct WalkStep {
		typedef stackify<std::vector<WalkStep>, 64> Vec;
		Vec3 pos;
		int tri;
		int connection;
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
		int waypoint,
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
		FloorBits stack,
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

	bsp_file::BSPFile::Ref m_bsp;
};

} // world

#include <Runtime/PopPack.h>
