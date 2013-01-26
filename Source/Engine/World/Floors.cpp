/*! \file Floors.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#include RADPCH
#include "Floors.h"
#include "World.h"
#include <limits>

namespace world {

namespace {
inline int BSPConnectionFlagsToStateFlags(int flags) {
	int z = 0;

	if (flags&bsp_file::kWaypointConnectionFlag_AutoFace)
		z |= FloorMove::State::kStateFlag_AutoFace;
	if (flags&bsp_file::kWaypointConnectionFlag_Interruptable)
		z |= FloorMove::State::kStateFlag_Interruptable;

	return z;
}
}

///////////////////////////////////////////////////////////////////////////////

FloorMove::FloorMove() {
}

void FloorMove::InitMove(State &state) {
	state.m_stepIdx = 0;
	state.flags = 0;
	state.pos.MakeNull();
	state.facing = Vec3::Zero;
	state.m_m.Init();
	state.m_first = true;

	if (!m_route.steps->empty()) {
		const Step &step = m_route.steps[0];
		step.path.Eval(0.f, state.pos.m_pos, &state.facing);
		state.pos.m_waypoint = step.waypoints[0];
		state.pos.m_nextWaypoint = step.waypoints[1];
		if (state.pos.m_waypoint == -1) {
			state.pos.m_floor = step.floors[0];
		}
	}
}

float FloorMove::Move(
	State &state, 
	float distance, 
	float &distanceRemainingAfterMove,
	StringVec &events
) {
	distanceRemainingAfterMove = 0.f;
	if (m_route.steps->empty())
		return 0.f;
	float moved = 0.f;

	while (moved < distance) {
		if (state.m_stepIdx >= (int)m_route.steps->size()) {
			const Step &step = m_route.steps[state.m_stepIdx-1];
			state.pos.m_waypoint = step.waypoints[1];
			state.pos.m_nextWaypoint = -1;
			if (state.pos.m_waypoint == -1) {
				state.pos.m_floor = step.floors[0];
			}
			break;
		}
		const Step &step = m_route.steps[state.m_stepIdx];

		if (state.m_first) {
			state.m_first = false;
			if (!step.events[0].empty)
				events.push_back(step.events[0]);
			state.pos.m_waypoint = step.waypoints[0];
			state.pos.m_nextWaypoint = step.waypoints[0];
			if (state.pos.m_waypoint == -1) {
				state.pos.m_floor = step.floors[0];
			}
		}
		
		float dd = distance - moved;

		float dx = state.m_m.Eval(step.path, dd, state.pos.m_pos, &state.facing, &distanceRemainingAfterMove);
		moved += dx;

		if (dx < dd) {
			events.push_back(step.events[1]);
			++state.m_stepIdx;
			state.m_m.Init();
			state.m_first = true;
		}
	}

	for (int i = state.m_stepIdx+1; i < (int)m_route.steps->size(); ++i) {
		distanceRemainingAfterMove += m_route.steps[i].path.length;
	}

	return moved;
}

void FloorMove::Merge(const Ref &old, State &state) {
}

///////////////////////////////////////////////////////////////////////////////

Floors::Floors() : m_floodNum(-1) {
}

Floors::~Floors() {
}

bool Floors::Load(const bsp_file::BSPFile::Ref &bsp) {
	m_bsp = bsp;

	m_waypoints.reserve(m_bsp->numWaypoints);
	for (U32 i = 0; i < m_bsp->numWaypoints; ++i) {
		const bsp_file::BSPWaypoint *waypoint = m_bsp->Waypoints() + i;

		Waypoint w;
		w.waypointId = (int)i;
		w.flags = (int)waypoint->flags;
		w.floodNum = -1;
		w.floodDistance = 0.f;

		if (waypoint->targetName >= 0) {
			w.targetName = m_bsp->String(waypoint->targetName);
			m_waypointTargets.insert(Waypoint::MMap::value_type(w.targetName, w.waypointId));
		}

		if (waypoint->userId >= 0) {
			w.userId = m_bsp->String(waypoint->userId);
			m_waypointUserIds.insert(Waypoint::MMap::value_type(w.userId, w.waypointId));
		}

		m_waypoints.push_back(w);
	}

	m_floorState.resize(m_bsp->numFloors, 0);

	return true;
}

bool Floors::ClipToFloor(
	const Vec3 &start,
	const Vec3 &end,
	FloorPosition &pos
) const {
	float bestDistSq = std::numeric_limits<float>::max();
	bool r = false;

	for (U32 i = 0; i < m_bsp->numFloors; ++i) {
		if (!(m_floorState[i] & kFloorState_Enabled))
			continue;
		if (ClipToFloor(i, start, end, pos, bestDistSq)) {
			r = true;
		}
	}

	return r;
}

FloorMove::Ref Floors::CreateMove(
	const FloorPosition &start,
	const FloorPosition &end
) const {
	MovePlan plan;
	MovePlan planStack;
	FloorBits floors;
	WaypointBits waypoints;
	float bestDistance = std::numeric_limits<float>::max();

	++m_floodNum;
	if (!PlanMove(start, end, 0.f, plan, planStack, floors, waypoints, bestDistance))
		return FloorMove::Ref(); // there is no path between these points.

	// We have a rough movement plan from start->end, build the walk commands
	WalkStep::Vec route;
	WalkStep::Vec workRoute;

	FloorPosition current(start);

	for (MoveStep::Vec::const_iterator it = plan.steps->begin(); it != plan.steps->end(); ++it) {
		const MoveStep &step = *it;

		const bsp_file::BSPWaypoint *waypoint = m_bsp->Waypoints() + step.waypoint;
		
		FloorPosition waypointPos;
		waypointPos.m_floor = (int)waypoint->floorNum;
		waypointPos.m_tri = (int)waypoint->triNum;
		waypointPos.m_pos = Vec3(waypoint->pos[0], waypoint->pos[1], waypoint->pos[2]);
		waypointPos.m_waypoint = step.waypoint;
		waypointPos.m_nextWaypoint = -1;
		
		// walk from floor -> waypoint (waypoint exiting floor)
		if (current.m_floor >= 0) {
			RAD_ASSERT(current.m_floor == waypointPos.m_floor);
			workRoute->clear();
			WalkFloor(current, waypointPos, workRoute);
			std::copy(workRoute->begin(), workRoute->end(), std::back_inserter(*route));
		}

		// walk from waypoint->waypoint (through connection)
		WalkConnection(step.waypoint, step.connection, route);

		const bsp_file::BSPWaypointConnection *connection = m_bsp->WaypointConnections() + step.connection;
		int otherIdx = connection->waypoints[0] == step.waypoint;
		const bsp_file::BSPWaypoint *otherWaypoint = m_bsp->Waypoints() + connection->waypoints[otherIdx];

		current.m_floor = (int)otherWaypoint->floorNum;
		current.m_tri = (int)otherWaypoint->triNum;
		current.m_pos = Vec3(otherWaypoint->pos[0], otherWaypoint->pos[1], otherWaypoint->pos[2]);
	}

	// walk.
	if (current.m_floor >= 0) {
		RAD_ASSERT(current.m_floor == end.m_floor);
		workRoute->clear();
		WalkFloor(current, end, workRoute);
		std::copy(workRoute->begin(), workRoute->end(), std::back_inserter(*route));
	} else { // waypoint move didn't generate a plan because we are at our destination waypoint already.
		if (route->empty())
			return FloorMove::Ref();
	}


	FloorMove::Ref move(new (ZWorld) FloorMove());
	GenerateFloorMove(route, move->m_route);
	return move;
}

bool Floors::WaypointPosition(int waypoint, FloorPosition &pos) const {
	if (waypoint < 0 || (waypoint >= (int)m_waypoints.size()))
		return false;
	const bsp_file::BSPWaypoint *bspWaypoint = m_bsp->Waypoints() + waypoint;

	pos.m_waypoint = waypoint;
	pos.m_floor = (int)bspWaypoint->floorNum;
	pos.m_tri = (int)bspWaypoint->triNum;
	pos.m_nextWaypoint = -1;
	pos.m_pos = Vec3(bspWaypoint->pos[0], bspWaypoint->pos[1], bspWaypoint->pos[2]);
	return true;
}

int Floors::WaypointState(int waypoint) const {
	if (waypoint < 0 || (waypoint >= (int)m_waypoints.size()))
		return false;
	return m_waypoints[waypoint].flags;
}

void Floors::SetWaypointState(int waypoint, int state) {
	if (waypoint < 0 || (waypoint >= (int)m_waypoints.size()))
		return;
	m_waypoints[waypoint].flags = state;
}

IntVec Floors::WaypointsForTargetname(const char *targetname) const {
	String s(CStr(targetname));
	std::pair<Waypoint::MMap::const_iterator,
		      Waypoint::MMap::const_iterator> pair = m_waypointTargets.equal_range(s);

	IntVec vec;
	while (pair.first != pair.second) {
		vec.push_back(pair.first->second);
		++pair.first;
	}

	return vec;
}

IntVec Floors::WaypointsForUserId(const char *userId) const {
	String s(CStr(userId));
	std::pair<Waypoint::MMap::const_iterator,
		      Waypoint::MMap::const_iterator> pair = m_waypointUserIds.equal_range(s);

	IntVec vec;
	while (pair.first != pair.second) {
		vec.push_back(pair.first->second);
		++pair.first;
	}

	return vec;
}

int Floors::PickWaypoint(
	World &world,
	float x,
	float y,
	float d
) {
	d = d*d; // squared distances.

	int best = -1;
	float bestDist = std::numeric_limits<float>::max();

	for (U32 i = 0; i < m_bsp->numWaypoints; ++i) {
		if (!(m_waypoints[i].flags&kWaypointState_Enabled))
			continue;
		const bsp_file::BSPWaypoint *waypoint = m_bsp->Waypoints() + i;
		const Vec3 kPos(waypoint->pos[0], waypoint->pos[1], waypoint->pos[2]);

		Vec3 out;
		if (!world.draw->Project(kPos, out))
			continue;

		float dx = out[0]-x;
		float dy = out[1]-y;
		float dd = dx*dx + dy*dy;

		if (dd <= d) {
			if (dd < bestDist) {
				bestDist = dd;
				best = (int)i;
			}
		}
	}

	return best;
}

int Floors::FindFloor(const char *name) const {
	const String kName(CStr(name));

	for (U32 i = 0; i < m_bsp->numFloors; ++i) {
		const bsp_file::BSPFloor *floor = m_bsp->Floors() + i;
		const char *floorName = m_bsp->String(floor->name);
		if (kName == floorName)
			return (int)i;
	}

	return -1;
}

int Floors::FloorState(int floor) const {
	if (floor < 0 || (floor >= (int)m_floorState.size()))
		return 0;
	return m_floorState[floor];
}

void Floors::SetFloorState(int floor, int state) {
	if (floor >= 0 && (floor < (int)m_floorState.size()))
		m_floorState[floor] = state;
}

void Floors::WalkFloor(
	const FloorPosition &start,
	const FloorPosition &end,
	WalkStep::Vec &route
) const {
	RAD_ASSERT(start.m_floor == end.m_floor);

	if (FindDirectRoute(start, end, route))
		return; // unobstructed path.

	FloorTriBits visited;
	WalkStep::Vec routeSoFar;
	float bestDistance = std::numeric_limits<float>::max();

	// done without recursion so we can handle large floors

	struct SortedEdge {
		float distance;
		int idx;
		bool visited;

		bool operator < (const SortedEdge &e) const {
			return distance < e.distance;
		}
	};

	struct Stack {
		typedef stackify<std::vector<Stack>, 256> Vec;
		FloorPosition pos;
		int edgeNum;
		int numVisited;
		float distance;
		boost::array<SortedEdge, 3> edges;
	};

	Stack::Vec stack;
	const bsp_file::BSPFloor *floor = m_bsp->Floors() + start.m_floor;

	Stack cur;
	cur.pos = start;
	cur.edgeNum = 0;
	cur.distance = 0.f;
	cur.numVisited = 0;

	WalkStep step;
	step.pos = cur.pos.m_pos;
	step.tri = cur.pos.m_tri;
	step.waypoints[0] = step.waypoints[1] = -1;
	step.floors[0] = step.floors[1] = cur.pos.m_floor;
	step.connection = -1;
	step.slopeChange = false;
	step.flags = bsp_file::kWaypointConnectionFlag_AutoFace;
	routeSoFar->push_back(step);
		
	const bsp_file::BSPFloorTri *tri = m_bsp->FloorTris() + start.m_tri;

	// sort edges
	
	for (int i = 0; i < 3; ++i) {
		const bsp_file::BSPFloorEdge *edge = m_bsp->FloorEdges() + tri->edges[i];
		const Vec3 kMid = FindEdgePoint(cur.pos.m_pos, edge);
		cur.edges[i].distance = (kMid-end.m_pos).MagnitudeSquared();
		cur.edges[i].idx = i;
		cur.edges[i].visited = false;
	}

	std::sort(cur.edges.begin(), cur.edges.end());

	visited.set(cur.pos.m_tri - (int)floor->firstTri);

	for (;;) {

		if ((cur.pos.m_tri == end.m_tri) || (cur.numVisited == 3)) {
			if (cur.pos.m_tri == end.m_tri) {
				// found a path.
				cur.distance += (end.m_pos - cur.pos.m_pos).MagnitudeSquared();
				if (cur.distance < bestDistance) {
					bestDistance = cur.distance;
					route = routeSoFar;

					step.pos = end.m_pos;
					step.tri = end.m_tri;
					step.slopeChange = false;
					route->push_back(step);
				}
			}
			if (stack->empty())
				break; // done
			routeSoFar->pop_back();
			visited.reset(cur.pos.m_tri - (int)floor->firstTri);
			cur = stack->back();
			stack->pop_back();
			tri = m_bsp->FloorTris() + cur.pos.m_tri;
			continue;
		}

		while (cur.numVisited < 3) {
			int curEdge = cur.edgeNum++;

			if (!cur.edges[curEdge].visited) {
				++cur.numVisited;
				cur.edges[curEdge].visited = true;

				int edgeNum = tri->edges[cur.edges[curEdge].idx];
				
				const bsp_file::BSPFloorEdge *edge = m_bsp->FloorEdges() + edgeNum;
				int otherSide = edge->tris[0] == cur.pos.m_tri;
				int otherTriNum = (int)edge->tris[otherSide];

				if ((otherTriNum != -1) && !visited.test(otherTriNum - (int)floor->firstTri)) {
					Vec3 mid = FindEdgePoint(cur.pos.m_pos, edge);
					
					float d = (mid - cur.pos.m_pos).MagnitudeSquared();
					float cost = (mid - end.m_pos).MagnitudeSquared();

					if ((cur.distance+d+cost) < bestDistance) { // valid

						const bsp_file::BSPFloorTri *otherTri = m_bsp->FloorTris() + otherTriNum;

						// cross to next triangle.
						visited.set(otherTriNum - (int)floor->firstTri);

						stack->push_back(cur);

						cur.distance += d;
						cur.pos.m_pos = mid;
						cur.pos.m_tri = otherTriNum;
						cur.edgeNum = 0;
						cur.numVisited = 1;

						step.pos = cur.pos.m_pos;
						step.tri = cur.pos.m_tri;
						step.slopeChange = tri->planenum != otherTri->planenum; // preserve slopes
						routeSoFar->push_back(step);

						tri = m_bsp->FloorTris() + cur.pos.m_tri;
						
						for (int i = 0; i < 3; ++i) {
							cur.edges[i].idx = i;
							if (tri->edges[i] == edgeNum) {
								cur.edges[i].visited = true;
								cur.edges[i].distance = std::numeric_limits<float>::max();
							} else {
								edge = m_bsp->FloorEdges() + tri->edges[i];
								mid = FindEdgePoint(cur.pos.m_pos, edge);
								cur.edges[i].visited = false;
								cur.edges[i].distance = (mid - end.m_pos).MagnitudeSquared();
							}
						}

						std::sort(cur.edges.begin(), cur.edges.end());

						break;
					}
				}
			}
		}
	}

	OptimizeRoute(route);
}

Vec3 Floors::FindEdgePoint(const Vec3 &pos, const bsp_file::BSPFloorEdge *edge) const {
	
#if 0
	const Vec3 kVec(edge->vec[0], edge->vec[1], edge->vec[2]);
	const bsp_file::BSPVertex *v0 = m_bsp->Vertices() + edge->verts[0];

	return Vec3(v0->v[0], v0->v[1], v0->v[2]) + (kVec * (edge->dist[1]-edge->dist[0]) * 0.5f);
#else
	const Vec3 kVec(edge->vec[0], edge->vec[1], edge->vec[2]);
	const bsp_file::BSPVertex *v0 = m_bsp->Vertices() + edge->verts[0];

	float dist = kVec.Dot(pos);

	const float kPad = 8.f;

	if (dist < edge->dist[0] + kPad)
		dist = edge->dist[0] + kPad;
	if (dist > edge->dist[1] - kPad)
		dist = edge->dist[1] - kPad;

	if ((dist < edge->dist[0]) ||
		(dist > edge->dist[1])) { // edge is < kPad units
			dist = (edge->dist[0] + edge->dist[1]) * 0.5f;
	}

	return Vec3(v0->v[0], v0->v[1], v0->v[2]) + (kVec * (dist-edge->dist[0]));
#endif
}

void Floors::WalkConnection(
	int targetWaypointNum,
	int connectionNum,
	WalkStep::Vec &route
) const {
	const bsp_file::BSPWaypointConnection *connection = m_bsp->WaypointConnections() + connectionNum;
	const bsp_file::BSPWaypoint *target = m_bsp->Waypoints() + targetWaypointNum;

	int dir = connection->waypoints[0] == targetWaypointNum;
	int startWaypointNum = (int)connection->waypoints[dir];
	const bsp_file::BSPWaypoint *start = m_bsp->Waypoints() + startWaypointNum;

	WalkStep step;
	step.pos = Vec3(start->pos[0], start->pos[1], start->pos[2]);
	step.connection = connectionNum;
	step.flags = connection->flags;
	step.waypoints[0] = startWaypointNum;
	step.waypoints[1] = targetWaypointNum;
	step.floors[0] = (int)start->floorNum;
	step.floors[1] = (int)target->floorNum;
	step.tri = -1;
	step.slopeChange = false;

	route->push_back(step);
}

bool Floors::FindDirectRoute(const FloorPosition &start, const FloorPosition &end, WalkStep::Vec &route) const {

	RAD_ASSERT(start.m_floor == end.m_floor);

	// follow the ray to the destination.
	WalkStep step;
	step.pos = start.m_pos;
	step.tri = start.m_tri;
	step.connection = -1;
	step.floors[0] = step.floors[1] = start.m_floor;
	step.waypoints[0] = step.waypoints[1] = -1;
	step.slopeChange = false;
	route->push_back(step);
	int skipEdge = -1;

	FloorPosition cur(start);

	while (cur.m_tri != end.m_tri) {
		float bestDist = std::numeric_limits<float>::max();
		int bestEdge = -1;
		Vec3 bestPos;

		const bsp_file::BSPFloorTri * tri = m_bsp->FloorTris() + cur.m_tri;

		for (int i = 0; i < 3; ++i) {
			if (tri->edges[i] == skipEdge)
				continue;

			const bsp_file::BSPFloorEdge *edge = m_bsp->FloorEdges() + tri->edges[i];
			const bsp_file::BSPPlane *plane = m_bsp->Planes() + edge->planenum;
			const Plane kPlane(plane->p[0], plane->p[1], plane->p[2], plane->p[3]);

			Vec3 x;
			if (kPlane.IntersectLineSegment(x, cur.m_pos, end.m_pos, 1.f)) {
				float d = (x - cur.m_pos).MagnitudeSquared();
				if (d < bestDist) {
					bestDist = d;
					bestEdge = i;
					bestPos = x;
				}
			}
		}

		if (bestEdge == -1) {
			return false;
			//FindDirectRoute(start, end, route);
		}

		RAD_ASSERT(bestEdge != -1);
		const bsp_file::BSPFloorEdge *edge = m_bsp->FloorEdges() + tri->edges[bestEdge];
		int otherTriNum = (edge->tris[0] == cur.m_tri) ? edge->tris[1] : edge->tris[0];

		if (otherTriNum == -1)
			return false; // hit solid space no direct path.

		const bsp_file::BSPFloorTri *otherTri = m_bsp->FloorTris() + otherTriNum;

		// have to add this if there is a slope change
		if (tri->planenum != otherTri->planenum) {
			step.pos = bestPos;
			step.tri = otherTriNum;
			step.slopeChange = true;
			route->push_back(step);
		}

		skipEdge = tri->edges[bestEdge]; // don't test this edge.
		cur.m_tri = otherTriNum;
		cur.m_pos = bestPos;
	}

	step.pos = end.m_pos;
	step.slopeChange = false;
	route->push_back(step);

	return true;
}

void Floors::OptimizeRoute(WalkStep::Vec &route) const {
	if (route->empty())
		return;

	WalkStep::Vec original(route);
	route->clear();
	
	const size_t kSize = original->size();

	for (size_t i = 0; i < kSize - 1; ++i) {
		const WalkStep &curStep = original[i];

		FloorPosition curPos;
		curPos.m_floor = curStep.floors[0];
		curPos.m_tri = curStep.tri;
		curPos.m_pos = curStep.pos;
		
		for (size_t k = kSize-1; k > i; --k) {
			const WalkStep &testStep = original[k];

			if (k == (i+1)) {
				// always a direct line between adjacent waypoints
				route->push_back(curStep);
				break;
			} else {
				
				FloorPosition testPos;
				testPos.m_floor = testStep.floors[0];
				testPos.m_tri = testStep.tri;
				
				Vec3 vec(testStep.pos - curPos.m_pos);
				vec.Normalize();

				testPos.m_pos = curPos.m_pos + vec*16384.f; // ensure it crosses edges.

				WalkStep::Vec testRoute;
				if (FindDirectRoute(curPos, testPos, testRoute)) {
					// direct connection between cur -> test through testRoute
					if (testRoute->size() > 2) {
						std::copy(testRoute->begin(), testRoute->end() - 1, std::back_inserter(*route));
					} else {
						route->push_back(curStep);
					}

					// advance to test pos.
					i = k - 1; // ++ in loop
					break;
				}
			}
		}
	}

	// always emit end-point
	route->push_back(original->back());
}

void Floors::OptimizeRoute2(WalkStep::Vec &route) const {
	if (route->empty())
		return;
	
	bool optimized = false;

	do {
		const size_t kSize = route->size();

		if (kSize > 2) {
			WalkStep::Vec original(route);
			route->clear();

			size_t i;
			for (i = 0; i < kSize - 2; i += 2) {
				// test direct connection between i -> (i+2) (optimize out i+1)
				const WalkStep &curStep = original[i];
				const WalkStep &skipStep = original[i+1];
				const WalkStep &testStep = original[i+2];

				FloorPosition curPos;
				curPos.m_floor = curStep.floors[0];
				curPos.m_tri = curStep.tri;
				curPos.m_pos = curStep.pos;

				FloorPosition testPos;
				testPos.m_floor = testStep.floors[0];
				testPos.m_tri = testStep.tri;

				Vec3 vec(testStep.pos - curPos.m_pos);
				vec.Normalize();

				testPos.m_pos = curPos.m_pos + vec*16384.f; // ensure it crosses edges.

				WalkStep::Vec testRoute;

				if (FindDirectRoute(curPos, testPos, testRoute)) {
					std::copy(testRoute->begin(), testRoute->end() - 1, std::back_inserter(*route));
				} else {
					route->push_back(curStep);
					route->push_back(skipStep);
				}
			}

			for (;i < kSize; ++i)
				route->push_back(original[i]);

			optimized = route->size() < original->size();
		} else {
			break;
		}

	} while (optimized);
}

void Floors::GenerateFloorMove(const WalkStep::Vec &walkRoute, FloorMove::Route &moveRoute) const {
	if (walkRoute->empty())
		return;

	const float kSmoothness = 32.f;

	const size_t kSize = walkRoute->size();

	for (size_t i = 0; i < kSize; ++i) {
		const WalkStep &cur = walkRoute[i];
		
		FloorMove::Step step;

		if (cur.waypoints[0] != -1) { // waypoint move
			step.waypoints[0] = cur.waypoints[0];
			step.waypoints[1] = cur.waypoints[1];
			step.floors[0] = cur.floors[0];
			step.floors[1] = cur.floors[1];
			step.connection = cur.connection;
			step.flags = BSPConnectionFlagsToStateFlags(cur.flags);

			const bsp_file::BSPWaypoint *waypoint = m_bsp->Waypoints() + cur.waypoints[1];
			const bsp_file::BSPWaypointConnection *connection = m_bsp->WaypointConnections() + cur.connection;
			int dir = connection->waypoints[1] == cur.waypoints[0];

			int cmdSet = dir;
			if (dir == 1) {
				if (connection->flags&bsp_file::kWaypointConnectionFlag_BtoAUseAtoBScript)
					cmdSet = 0;
			}

			if (connection->cmds[cmdSet*2+0] != -1)
				step.events[0] = String(m_bsp->String(connection->cmds[cmdSet*2+0]));
			if (connection->cmds[cmdSet*2+1] != -1)
				step.events[1] = String(m_bsp->String(connection->cmds[cmdSet*2+1]));

			physics::CubicBZSpline spline(
				cur.pos,
				Vec3(connection->ctrls[dir][0], connection->ctrls[dir][1], connection->ctrls[dir][2]),
				Vec3(connection->ctrls[!dir][0], connection->ctrls[!dir][1], connection->ctrls[!dir][2]),
				Vec3(waypoint->pos[0], waypoint->pos[1], waypoint->pos[2])
			);

			step.path.Load(spline);
			moveRoute.steps->push_back(step);
			continue;
		}

		if (i >= (kSize-1)) {
			RAD_ASSERT(!moveRoute.steps->empty());
			break; // no more moves
		}

		const WalkStep &next = walkRoute[i+1];

		Vec3 ctrls[2];

		Vec3 vNext(next.pos - cur.pos);
		float nextLen = vNext.Normalize();
		
		// make move continous in X, Y. Preserve motion exactly in Z (don't want to float above/below
		// floor).
		if (i > 0) {
			const WalkStep &prev = walkRoute[i-1];
			Vec3 prevPos;

			if (prev.waypoints[0] != -1) {
				const bsp_file::BSPWaypoint *waypoint = m_bsp->Waypoints() + prev.waypoints[1];
				prevPos.Initialize(waypoint->pos[0], waypoint->pos[1], waypoint->pos[2]);
			} else {
				prevPos = prev.pos;
			}

			Vec3 vPrev(cur.pos - prevPos);
			float prevLen = vPrev.Normalize();

			ctrls[0] = vPrev + vNext;
			// isolate Z
			ctrls[0][2] = 0.f;
			ctrls[0].Normalize();
			ctrls[0][2] = vNext[2]; // no longer normalized
				
			if (prev.waypoints[0] != -1) { // connect with previous waypoint move
				const bsp_file::BSPWaypoint *waypoint = m_bsp->Waypoints() + prev.waypoints[1];
				const bsp_file::BSPWaypointConnection *connection = m_bsp->WaypointConnections() + prev.connection;
				int other = connection->waypoints[0] == prev.waypoints[0];

				Vec3 bzCtrls[4];
				bzCtrls[0] = Vec3(waypoint->pos[0], waypoint->pos[1], waypoint->pos[2]);
				
				// invert ctrl point.
				Vec3 z(connection->ctrls[other][0], connection->ctrls[other][1], connection->ctrls[other][2]);
				z = z - bzCtrls[0];
				z[2] = 0.f; // isolate Z
				z.Normalize();
				z[2] = vPrev[2];

				float len = std::min(prevLen / 4.f, kSmoothness);

				bzCtrls[1] = bzCtrls[0] - (z*len);
			
				bzCtrls[2][0] = -ctrls[0][0];
				bzCtrls[2][1] = -ctrls[0][1];
				bzCtrls[2][3] = -vPrev[2];

				bzCtrls[2] = cur.pos + (bzCtrls[2] * len);
				bzCtrls[3] = cur.pos;

				physics::CubicBZSpline spline(bzCtrls);
				step.waypoints[0] = step.waypoints[1] = -1;
				step.connection = -1;
				step.flags = BSPConnectionFlagsToStateFlags(cur.flags);
				step.floors[0] = step.floors[1] = cur.floors[0];
				step.path.Load(spline);
				moveRoute.steps->push_back(step);
			}		
		} else {
			ctrls[0] = vNext;
		}

		float len = std::min(nextLen / 4.f, kSmoothness);
		ctrls[0] = cur.pos + (ctrls[0] * len);

		if (next.waypoints[0] != -1) { // connect to waypoint
			const bsp_file::BSPWaypoint *waypoint = m_bsp->Waypoints() + next.waypoints[0];
			const bsp_file::BSPWaypointConnection *connection = m_bsp->WaypointConnections() + next.connection;
			int self = connection->waypoints[1] == next.waypoints[0];

			// invert ctrl point.
			Vec3 z(connection->ctrls[self][0], connection->ctrls[self][1], connection->ctrls[self][2]);
			z = z - next.pos;
			z[2] = 0.f; // isolate Z
			z.Normalize();
			z[2] = vNext[2]; // no longer normalized

			ctrls[1] = -z;

		} else if (i < (kSize-2)) {
			const WalkStep &nextNext = walkRoute[i+2];
			Vec3 vNextNext(nextNext.pos - next.pos);

			float nnLen = vNextNext.Normalize();

			ctrls[1] = vNextNext + vNext;
			// isolate Z
			ctrls[1][2] = 0.f;
			ctrls[1].Normalize();
			ctrls[1][2] = vNext[2]; // no longer normalized
				
			ctrls[1] = -ctrls[1];
		} else {
			ctrls[1] = -vNext;
		}

		ctrls[1] = next.pos + (ctrls[1]*len);

		physics::CubicBZSpline spline(cur.pos, ctrls[0], ctrls[1], next.pos);
		step.floors[0] = step.floors[1] = cur.floors[0];
		step.waypoints[0] = step.waypoints[1] = -1;
		step.connection = -1;
		step.flags = BSPConnectionFlagsToStateFlags(cur.flags);
		step.path.Load(spline);

		moveRoute.steps->push_back(step);
	}
}

inline bool Floors::PlanMove(
	const FloorPosition &start,
	const FloorPosition &end,
	float distance,
	MovePlan &plan,
	MovePlan &planSoFar,
	FloorBits &floors,
	WaypointBits &waypoints,
	float &bestDistance
) const {
	// done on stack to avoid stackoverflow
	++m_floodNum;

	planSoFar.start = start;
	planSoFar.end   = end;
	planSoFar.steps->clear();
	
	bestDistance = std::numeric_limits<float>::max();
	
	struct Connection {
		typedef stackify<std::vector<Connection>, 16> Vec;

		FloorPosition pos;

		const bsp_file::BSPWaypoint *nextWaypoint;
		const bsp_file::BSPFloor *nextFloor;
		const bsp_file::BSPWaypointConnection *connection;

		int nextWaypointIdx;
		int connectionIdx;
		int nextFloorIdx;

		float cost; // <-- distace from position to the waypoint
		float distance; // <-- distance from waypoint to the target

		bool operator < (const Connection &c) {
			return distance < c.distance;
		}
	};

	struct Stack {
		typedef stackify<std::vector<Stack>, 512> Vec;
		Connection::Vec connections;
		FloorPosition pos;
		float distance;
		int idx;
	};
	
	Stack::Vec stack;
	Stack cur;
	bool foundPath = false;


	// setup our starting state

	if (start.m_waypoint != -1) {
		// standing on a waypoint:

		const bsp_file::BSPWaypoint *waypoint = m_bsp->Waypoints() + start.m_waypoint;

		for (U32 i = 0; i < waypoint->numConnections; ++i) {
			Connection c;
			c.connectionIdx = (int)*(m_bsp->WaypointIndices() + waypoint->firstConnection + i);
			c.connection = m_bsp->WaypointConnections() + c.connectionIdx;
			
			int dir = c.connection->waypoints[0] == (U32)start.m_waypoint;
			c.nextWaypointIdx = (int)c.connection->waypoints[dir];
			c.nextWaypoint = m_bsp->Waypoints() + c.nextWaypointIdx;

			c.nextFloorIdx = (int)c.nextWaypoint->floorNum;
			if (c.nextFloorIdx != -1) {
				c.nextFloor = m_bsp->Floors() + c.nextFloorIdx;
			} else {
				c.nextFloor = 0;
			}

			const Vec3 kPos(c.nextWaypoint->pos[0], c.nextWaypoint->pos[1], c.nextWaypoint->pos[2]);
			c.cost = (kPos - start.m_pos).MagnitudeSquared();
			c.distance = (kPos - end.m_pos).MagnitudeSquared();

			c.pos.m_floor = (int)c.nextWaypoint->floorNum;
			c.pos.m_waypoint = (int)c.nextWaypointIdx;
			c.pos.m_tri = (int)c.nextWaypoint->triNum;
			c.pos.m_nextWaypoint = -1;
			c.pos.m_pos = kPos;

			cur.connections->push_back(c);
		}

		waypoints.set(start.m_waypoint);

	} else {
		// at an arbitrary position on a floor
		RAD_ASSERT(start.m_floor != -1); 

		const bsp_file::BSPFloor *floor = m_bsp->Floors() + start.m_floor;

		for (S32 i = 0; i < floor->numWaypoints; ++i) {
			Connection c;
			c.nextFloor = 0;
			c.connection = 0;
			c.nextFloorIdx = -1;
			c.connectionIdx = -1;
			c.nextWaypointIdx = (int)*(m_bsp->WaypointIndices() + floor->firstWaypoint + i);
			c.nextWaypoint = m_bsp->Waypoints() + c.nextWaypointIdx ;

			const Vec3 kPos(c.nextWaypoint->pos[0], c.nextWaypoint->pos[1], c.nextWaypoint->pos[2]);
			c.cost = (kPos - start.m_pos).MagnitudeSquared();
			c.distance = (kPos - end.m_pos).MagnitudeSquared();

			c.pos.m_floor = (int)c.nextWaypoint->floorNum;
			c.pos.m_waypoint = (int)c.nextWaypointIdx;
			c.pos.m_tri = (int)c.nextWaypoint->triNum;
			c.pos.m_nextWaypoint = -1;
			c.pos.m_pos = kPos;

			cur.connections->push_back(c);
		}

		floors.set(start.m_floor);

	}

	std::sort(cur.connections->begin(), cur.connections->end());
	cur.distance = 0.f;
	cur.pos = start;
	cur.idx = 0;

	for (;;) {

		// found a path?
		bool goal = false;
		bool pop  = true;

		if ((end.m_floor != -1) && (end.m_waypoint == -1)) {
			// we are traveling to an arbitrary spot on a floor.

			if (cur.pos.m_floor == end.m_floor) {
				float distance = cur.distance;
				distance += (end.m_pos - cur.pos.m_pos).MagnitudeSquared();

				if (distance < bestDistance) {
					bestDistance = distance;
					plan = planSoFar;
					goal = true;
				}
			}
		} else {
			RAD_ASSERT(end.m_waypoint != -1);

			// we are traveling to a specific waypoint
			if (cur.pos.m_waypoint == end.m_waypoint) {
				if (cur.distance < bestDistance) {
					bestDistance = cur.distance;
					plan = planSoFar;
					goal = true;
				}
			}
		}

		foundPath = foundPath || goal;

		if (!goal && (cur.distance < bestDistance)) {
			for (;cur.idx < (int)cur.connections->size(); ++cur.idx) {

				const Connection &c = cur.connections[cur.idx];

				// check for recursion:
				RAD_ASSERT(c.nextWaypoint);

				if (waypoints.test(c.nextWaypointIdx))
					continue;
			
				if (c.nextFloor && (c.nextFloorIdx != cur.pos.m_floor)) {
					// we are changing floors check recursion
					if (floors.test(c.nextFloorIdx))
						continue;
				}

				float distance = cur.distance + c.cost;

				if (distance >= bestDistance)
					continue; // this will only get worse

				const Waypoint &dwaypoint = m_waypoints[c.nextWaypointIdx];

				// have we passed through this waypoint already?
				if (dwaypoint.floodNum == m_floodNum) {
					// if so, are we any closer than we were last time?
					if (distance >= dwaypoint.floodDistance)
						continue;
				}

				// NOTE: mutable members
				dwaypoint.floodNum = m_floodNum;
				dwaypoint.floodDistance = distance;

				// we are gonna cross this connection
				waypoints.set(c.nextWaypointIdx);
				if (c.nextFloor)
					floors.set(c.nextFloorIdx);

				// we must be crossing a waypoint connection or be at an arbitrary floor position
				RAD_ASSERT((c.connectionIdx != -1) || (cur.pos.m_floor != -1));

				if (c.connectionIdx != -1) {
					MoveStep step;
					step.waypoint = c.nextWaypointIdx;
					step.connection = c.connectionIdx;
					planSoFar.steps->push_back(step);
				}

				stack->push_back(cur);

				cur.idx = 0;
				cur.pos = c.pos;
				cur.distance = distance;
				cur.connections->clear();

				// generate sorted connects
				RAD_ASSERT(cur.pos.m_waypoint != -1);

				// standing on a waypoint:

				const bsp_file::BSPWaypoint *waypoint = m_bsp->Waypoints() + cur.pos.m_waypoint;

				for (U32 i = 0; i < waypoint->numConnections; ++i) {
					Connection c;
					c.connectionIdx = (int)*(m_bsp->WaypointIndices() + waypoint->firstConnection + i);
					c.connection = m_bsp->WaypointConnections() + c.connectionIdx;
			
					int dir = c.connection->waypoints[0] == (U32)cur.pos.m_waypoint;
					c.nextWaypointIdx = (int)c.connection->waypoints[dir];
					c.nextWaypoint = m_bsp->Waypoints() + c.nextWaypointIdx;

					c.nextFloorIdx = (int)c.nextWaypoint->floorNum;
					if (c.nextFloorIdx != -1) {
						c.nextFloor = m_bsp->Floors() + c.nextFloorIdx;
					} else {
						c.nextFloor = 0;
					}

					const Vec3 kPos(c.nextWaypoint->pos[0], c.nextWaypoint->pos[1], c.nextWaypoint->pos[2]);
					c.cost = (kPos - cur.pos.m_pos).MagnitudeSquared();
					c.distance = (kPos - end.m_pos).MagnitudeSquared();

					c.pos.m_floor = (int)c.nextWaypoint->floorNum;
					c.pos.m_waypoint = (int)c.nextWaypointIdx;
					c.pos.m_tri = (int)c.nextWaypoint->triNum;
					c.pos.m_nextWaypoint = -1;
					c.pos.m_pos = kPos;

					cur.connections->push_back(c);
				}

				std::sort(cur.connections->begin(), cur.connections->end());
				pop = false;
				break;
			}
		}

		if (pop) {
			if (stack->empty())
				break;

			cur = stack->back();
			stack->pop_back();

			if (cur.idx < (int)cur.connections->size()) {
				// pop our last state.
				const Connection &c = cur.connections[cur.idx];
				if (c.nextWaypoint)
					waypoints.reset(c.nextWaypointIdx);
				if (c.nextFloor)
					floors.reset(c.nextFloorIdx);
				if (!planSoFar.steps->empty())
					planSoFar.steps->pop_back();
				++cur.idx;
			}
		}
	}

	return foundPath;
}

bool Floors::ClipToFloor(
	U32 floorNum,
	const Vec3 &start,
	const Vec3 &end,
	FloorPosition &pos,
	float &bestDistSq
) const {
	// TODO: optimize this with a AA BSP
	
	const bsp_file::BSPFloor *floor = m_bsp->Floors() + floorNum;

	RAD_ASSERT(floor->numTris > 0);

	int bestTri = -1;

	for (U32 i = 0; i < floor->numTris; ++i) {
		U32 triNum = floor->firstTri + i;
		const bsp_file::BSPFloorTri *tri = m_bsp->FloorTris() + triNum;

		const bsp_file::BSPPlane *plane = m_bsp->Planes() + tri->planenum;
		const Plane kTriPlane(plane->p[0], plane->p[1], plane->p[2], plane->p[3]);

		Vec3 clip;

		if (!kTriPlane.IntersectLineSegment(clip, start, end, 0.f))
			continue;

		float distSq = (clip-start).MagnitudeSquared();
		if (distSq >= bestDistSq) // can't be better
			continue;

		int k;
		
		for (k = 0; k < 3; ++k) {
			const bsp_file::BSPFloorEdge *edge = m_bsp->FloorEdges() + tri->edges[k];
			plane = m_bsp->Planes() + edge->planenum;

			int side = edge->tris[1] == triNum;

			Plane edgePlane(plane->p[0], plane->p[1], plane->p[2], plane->p[3]);

			if (side)
				edgePlane.Flip();

			if (edgePlane.Side(clip) == Plane::Back) {
				break;
			}
		}

		if (k == 3) {
			// inside triangle hull
			bestDistSq = distSq;
			pos.m_pos = clip;
			pos.m_floor = (int)floorNum;
			bestTri = triNum;
			pos.m_tri = bestTri;
		}
	}

	return bestTri != -1;
}

} // world

namespace lua {

void Marshal<world::FloorPosition>::Push(lua_State *L, const world::FloorPosition &val) {
	RAD_ASSERT(L);
	lua_createtable(L, 0, 3);
	Marshal<Vec3>::Push(L, val.m_pos);
	lua_setfield(L, -2, "pos"); // <-- public
	Marshal<Vec3>::Push(L, val.m_pos);
	lua_setfield(L, -2, "@pos");
	lua_pushinteger(L, (int)val.m_floor);
	lua_setfield(L, -2, "@floor");
	lua_pushinteger(L, (int)val.m_tri);
	lua_setfield(L, -2, "@tri");
	lua_pushinteger(L, (int)val.m_waypoint);
	lua_setfield(L, -2, "@waypoint");
	lua_pushinteger(L, (int)val.m_nextWaypoint);
	lua_setfield(L, -2, "@nextWaypoint");
}

world::FloorPosition Marshal<world::FloorPosition>::Get(lua_State *L, int index, bool luaError) {
	world::FloorPosition p;

	if (lua_type(L, index) != LUA_TTABLE) {
		if (luaError)
			luaL_typerror(L, index, "table");
		return p;
	}

	lua_getfield(L, index, "@pos");
	p.m_pos = Marshal<Vec3>::Get(L, -1, luaError);
	lua_pop(L, 1);

	lua_getfield(L, index, "@floor");
	if (lua_type(L, -1) != LUA_TNUMBER) {
		lua_pop(L, 1);
		if (luaError)
			luaL_typerror(L, index, "number");
		return p;
	}
	p.m_floor = (int)lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, index, "@tri");
	if (lua_type(L, -1) != LUA_TNUMBER) {
		lua_pop(L, 1);
		if (luaError)
			luaL_typerror(L, index, "tri");
		return p;
	}

	p.m_tri = (int)lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, index, "@waypoint");
	if (lua_type(L, -1) != LUA_TNUMBER) {
		lua_pop(L, 1);
		if (luaError)
			luaL_typerror(L, index, "waypoint");
		return p;
	}

	p.m_waypoint = (int)lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, index, "@nextWaypoint");
	if (lua_type(L, -1) != LUA_TNUMBER) {
		lua_pop(L, 1);
		if (luaError)
			luaL_typerror(L, index, "nextWaypoint");
		return p;
	}

	p.m_nextWaypoint = (int)lua_tointeger(L, -1);
	lua_pop(L, 1);
	return p;
}

bool Marshal<world::FloorPosition>::IsA(lua_State *L, int index) {
	return lua_type(L, index) == LUA_TTABLE;
}

} // lua
