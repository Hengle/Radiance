/*! \file Floors.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#include RADPCH
#include "Floors.h"
#include <limits>

namespace world {

///////////////////////////////////////////////////////////////////////////////

FloorMove::FloorMove() {
}

void FloorMove::InitMove(State &state) {

}

bool FloorMove::Move(State &state, float velocity) {
	return false;
}

bool FloorMove::RAD_IMPLEMENT_GET(busy) {
	return false;
}

void FloorMove::Merge(const Ref &old, State &state) {
}

///////////////////////////////////////////////////////////////////////////////

Floors::Floors() {
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
	}
#if defined(RAD_OPT_DEBUG)
	else {
		RAD_ASSERT(!route->empty()); // if it was not a floor->floor move this has to have been waypoint->waypoint
		// and those always generate move commands.
	}
#endif

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
		      Waypoint::MMap::const_iterator> pair = m_waypointTargets.equal_range(s);

	IntVec vec;
	while (pair.first != pair.second) {
		vec.push_back(pair.first->second);
		++pair.first;
	}

	return vec;
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
	int waypointNum,
	int connectionNum,
	WalkStep::Vec &route
) const {
	const bsp_file::BSPWaypointConnection *connection = m_bsp->WaypointConnections() + connectionNum;
	const bsp_file::BSPWaypoint *waypoint = m_bsp->Waypoints() + waypointNum;

	int otherWaypointNum = (int)connection->waypoints[(connection->waypoints[1] == waypointNum) ? 0 : 1];
	const bsp_file::BSPWaypoint *otherWaypoint = m_bsp->Waypoints() + otherWaypointNum;

	WalkStep step;
	step.pos = Vec3(waypoint->pos[0], waypoint->pos[1], waypoint->pos[2]);
	step.connection = connectionNum;
	step.waypoints[0] = waypointNum;
	step.waypoints[1] = otherWaypointNum;
	step.floors[0] = (int)waypoint->floorNum;
	step.floors[1] = (int)otherWaypoint->floorNum;
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

	for (size_t i = 0; i < kSize - 1; ++i) {
		const WalkStep &cur = walkRoute[i];
		const WalkStep &next = walkRoute[i+1];

		FloorMove::Step step;

		if (cur.waypoints[0] != -1) { // waypoint move
			step.waypoints[0] = cur.waypoints[0];
			step.waypoints[1] = cur.waypoints[1];
			step.floors[0] = cur.floors[0];
			step.floors[1] = cur.floors[1];
			step.connection = cur.connection;

			const bsp_file::BSPWaypoint *waypoint = m_bsp->Waypoints() + cur.waypoints[1];
			const bsp_file::BSPWaypointConnection *connection = m_bsp->WaypointConnections() + cur.connection;
			int self = connection->waypoints[1] == cur.waypoints[0];

			physics::CubicBZSpline spline(
				cur.pos,
				Vec3(connection->ctrls[self][0], connection->ctrls[self][1], connection->ctrls[self][2]),
				Vec3(connection->ctrls[!self][0], connection->ctrls[!self][1], connection->ctrls[!self][2]),
				Vec3(waypoint->pos[0], waypoint->pos[1], waypoint->pos[2])
			);

			step.path.Load(spline);
			moveRoute.steps->push_back(step);
			continue;
		}

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
	if (start.m_floor >= 0)
		return PlanFloorMove(start, end, distance, plan, planSoFar, floors, waypoints, bestDistance);
	return PlanWaypointMove(start, end, distance, plan, planSoFar, floors, waypoints, bestDistance);
}

bool Floors::PlanFloorMove(
	const FloorPosition &start,
	const FloorPosition &end,
	float distance,
	MovePlan &plan,
	MovePlan &planSoFar,
	FloorBits &floors,
	WaypointBits &waypoints,
	float &bestDistance
) const {
	RAD_ASSERT(start.m_floor >= 0);
	RAD_ASSERT(!floors.test(start.m_floor));
	
	// end is a floor move?
	if ((end.m_floor >= 0) && (start.m_floor == end.m_floor)) {
		Vec3 seg = end.m_pos - start.m_pos;
		distance += seg.MagnitudeSquared();
		if (distance < bestDistance) {
			bestDistance = distance;
			plan = planSoFar;
			return true;
		}
		return false;
	}
	
	floors.set(start.m_floor);

	// follow all waypoints out.
	bool foundMove = false;

	const bsp_file::BSPFloor *floor = m_bsp->Floors() + start.m_floor;
	for (S32 i = 0; i < floor->numWaypoints; ++i) {
		U16 waypointNum = *(m_bsp->WaypointIndices() + floor->firstWaypoint + i);
		const bsp_file::BSPWaypoint *waypoint = m_bsp->Waypoints() + waypointNum;

		const Vec3 kWaypointPos(waypoint->pos[0], waypoint->pos[1], waypoint->pos[2]);

		float estimatedDistanceToWaypoint = (kWaypointPos - start.m_pos).MagnitudeSquared() + distance;

		// NOTE: Connections are sorted by distance in the bsp file.

		for (U32 k = 0; k < waypoint->numConnections; ++k) {
			U16 connectionNum = *(m_bsp->WaypointIndices() + waypoint->firstConnection + k);
			const bsp_file::BSPWaypointConnection *connection = m_bsp->WaypointConnections() + connectionNum;

			int otherIdx = connection->waypoints[0] == waypointNum;
			U32 otherWaypointIdx = connection->waypoints[otherIdx];

			if (!(m_waypoints[otherWaypointIdx].flags&kWaypointState_Enabled))
				continue; // waypoint is disabled right now.

			if (waypoints.test(otherWaypointIdx)) // came from here.
				continue;

			const bsp_file::BSPWaypoint *otherWaypoint = m_bsp->Waypoints() + otherWaypointIdx;

			if (otherWaypoint->floorNum >= 0) {
				// takes us to a floor.
				if (floors.test(otherWaypoint->floorNum))
					continue; // came from here
			}

			const Vec3 kOtherWaypointPos(otherWaypoint->pos[0], otherWaypoint->pos[1], otherWaypoint->pos[2]);

			float estimatedDistanceToOtherWaypoint = (kOtherWaypointPos - kWaypointPos).MagnitudeSquared() + estimatedDistanceToWaypoint;

			if (estimatedDistanceToOtherWaypoint >= bestDistance)
				continue; // can't be better

			// go to other waypoint.
			FloorPosition waypointStart;
			waypointStart.m_pos = kOtherWaypointPos;
			waypointStart.m_floor = (int)otherWaypoint->floorNum;
			waypointStart.m_tri = (int)otherWaypoint->triNum;
			waypointStart.m_waypoint = (int)otherWaypointIdx;
			waypointStart.m_nextWaypoint = -1;

			MoveStep step;
			step.waypoint = (int)otherWaypointIdx;
			step.connection = (int)connectionNum;
			
			planSoFar.steps->push_back(step);

			if (PlanMove(
				waypointStart,
				end,
				estimatedDistanceToOtherWaypoint,
				plan,
				planSoFar,
				floors,
				waypoints,
				bestDistance)) {
					foundMove = true;
			}

			planSoFar.steps->pop_back();
		}
	}

	floors.reset(start.m_floor);

	return foundMove;
}

bool Floors::PlanWaypointMove(
	const FloorPosition &start,
	const FloorPosition &end,
	float distance,
	MovePlan &plan,
	MovePlan &planSoFar,
	FloorBits &floors,
	WaypointBits &waypoints,
	float &bestDistance
) const {
	RAD_ASSERT(start.m_floor < 0);
	RAD_ASSERT(!waypoints.test(start.m_waypoint));
	
	// end is a waypoint?
	if ((end.m_floor< 0) && (start.m_waypoint == end.m_waypoint)) {
		if (distance < bestDistance) {
			bestDistance = distance;
			plan = planSoFar;
			return true;
		}
		
		return false;
	}
	
	waypoints.set(start.m_waypoint);

	// follow all waypoints out.
	bool foundMove = false;

	const bsp_file::BSPWaypoint *waypoint = m_bsp->Waypoints() + start.m_waypoint;

	const Vec3 kWaypointPos(waypoint->pos[0], waypoint->pos[1], waypoint->pos[2]);

	// NOTE: Connections are sorted by distance in the bsp file.
	// Waypoints are in a sparsly connected graph, a shorter euclidean distance between connections 
	// does not imply a path to the target.

	bool isNeighbor = false;

	for (U32 k = 0; k < waypoint->numConnections; ++k) {
		U16 connectionNum = *(m_bsp->WaypointIndices() + waypoint->firstConnection + k);
		const bsp_file::BSPWaypointConnection *connection = m_bsp->WaypointConnections() + connectionNum;

		int otherIdx = connection->waypoints[0] == start.m_waypoint;
		U32 otherWaypointIdx = connection->waypoints[otherIdx];

		if (!(m_waypoints[otherWaypointIdx].flags&kWaypointState_Enabled))
			continue; // waypoint is disabled right now.

		if (otherWaypointIdx == end.m_waypoint) {
			isNeighbor = true;

			// goal.
			const bsp_file::BSPWaypoint *otherWaypoint = m_bsp->Waypoints() + otherWaypointIdx;
			const Vec3 kOtherWaypointPos(otherWaypoint->pos[0], otherWaypoint->pos[1], otherWaypoint->pos[2]);
			float estimatedDistanceToOtherWaypoint = (kOtherWaypointPos - kWaypointPos).MagnitudeSquared();
			if (estimatedDistanceToOtherWaypoint < bestDistance) {
				// go to other waypoint.
				FloorPosition waypointStart;
				waypointStart.m_pos = kOtherWaypointPos;
				waypointStart.m_floor = (int)otherWaypoint->floorNum;
				waypointStart.m_tri = (int)otherWaypoint->triNum;
				waypointStart.m_waypoint = (int)otherWaypointIdx;
				waypointStart.m_nextWaypoint = -1;

				MoveStep step;
				step.waypoint = (int)otherWaypointIdx;
				step.connection = (int)connectionNum;
			
				planSoFar.steps->push_back(step);

				if (PlanMove(
					waypointStart,
					end,
					estimatedDistanceToOtherWaypoint,
					plan,
					planSoFar,
					floors,
					waypoints,
					bestDistance)) {
						foundMove = true;
				}

				planSoFar.steps->pop_back();
				RAD_ASSERT(foundMove == true);
			}

			break;
		}
	}

	if (!isNeighbor) {
		for (U32 k = 0; k < waypoint->numConnections; ++k) {
			U16 connectionNum = *(m_bsp->WaypointIndices() + waypoint->firstConnection + k);
			const bsp_file::BSPWaypointConnection *connection = m_bsp->WaypointConnections() + connectionNum;

			int otherIdx = connection->waypoints[0] == start.m_waypoint;
			U32 otherWaypointIdx = connection->waypoints[otherIdx];

			if (waypoints.test(otherWaypointIdx)) // came from here.
				continue;

			if (!(m_waypoints[otherWaypointIdx].flags&kWaypointState_Enabled))
				continue; // waypoint is disabled right now.

			const bsp_file::BSPWaypoint *otherWaypoint = m_bsp->Waypoints() + otherWaypointIdx;

			if (otherWaypoint->floorNum >= 0) {
				// takes us to a floor.
				if (floors.test(otherWaypoint->floorNum))
					continue; // came from here
			}

			const Vec3 kOtherWaypointPos(otherWaypoint->pos[0], otherWaypoint->pos[1], otherWaypoint->pos[2]);

			float estimatedDistanceToOtherWaypoint = (kOtherWaypointPos - kWaypointPos).MagnitudeSquared();

			if (estimatedDistanceToOtherWaypoint >= bestDistance)
				continue; // can't be better

			// go to other waypoint.
			FloorPosition waypointStart;
			waypointStart.m_pos = kOtherWaypointPos;
			waypointStart.m_floor = (int)otherWaypoint->floorNum;
			waypointStart.m_tri = (int)otherWaypoint->triNum;
			waypointStart.m_waypoint = (int)otherWaypointIdx;

			MoveStep step;
			step.waypoint = (int)otherWaypointIdx;
			step.connection = (int)connectionNum;
			
			planSoFar.steps->push_back(step);

			if (PlanMove(
				waypointStart,
				end,
				estimatedDistanceToOtherWaypoint,
				plan,
				planSoFar,
				floors,
				waypoints,
				bestDistance)) {
					foundMove = true;
			}

			planSoFar.steps->pop_back();
		}
	}

	waypoints.reset(start.m_waypoint);

	return foundMove;
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
	return p;
}

bool Marshal<world::FloorPosition>::IsA(lua_State *L, int index) {
	return lua_type(L, index) == LUA_TTABLE;
}

} // lua
