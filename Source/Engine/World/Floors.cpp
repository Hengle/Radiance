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
	if (flags&bsp_file::kWaypointConnectionFlag_AutoPitch)
		z |= FloorMove::State::kStateFlag_AutoPitch;

	return z;
}
}

///////////////////////////////////////////////////////////////////////////////

void FloorMove::Step::Reverse() {
	std::swap(events[0], events[1]);
	std::swap(waypoints[0], waypoints[1]);
	std::swap(floors[0], floors[1]);
	spline.Reverse();
	path.Load(spline);
}

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
		state.flags = step.flags;
		step.path.Eval(0.f, state.pos.m_pos, &state.facing);
		state.pos.m_waypoint = step.waypoints[0];
		state.pos.m_nextWaypoint = step.waypoints[1];
		state.pos.m_floor = (state.pos.m_waypoint != -1) ? -1 : step.floors[0];
	}
}

float FloorMove::Move(
	State &state, 
	float distance, 
	float &distanceRemainingAfterMove,
	StringVec &events,
	String &moveAnim
) {
	distanceRemainingAfterMove = 0.f;
	if (m_route.steps->empty())
		return 0.f;
	float moved = 0.f;

	if (state.m_stepIdx >= (int)m_route.steps->size())
		return 0.f;

	while (moved < distance) {
		
		const Step *step = &m_route.steps[state.m_stepIdx];

		if (state.m_first) {
			state.m_first = false;
			state.flags = step->flags;
			if (!step->events[0].empty)
				events.push_back(step->events[0]);
			moveAnim = step->anim;

			if (!moveAnim.empty) {
				state.moveAnim = true;
				state.m_m.Eval(step->path, 0.f, state.pos.m_pos, &state.facing, &distanceRemainingAfterMove);
				break; // animation system must move us now.
			}
		}

		float dd = distance - moved;

		float dx = state.m_m.Eval(step->path, dd, state.pos.m_pos, &state.facing, &distanceRemainingAfterMove);
		moved += dx;

		if (dx < dd) {
			if (!step->events[1].empty)
				events.push_back(step->events[1]);
			++state.m_stepIdx;
			state.m_m.Init();
			state.m_first = true;

			if (state.m_stepIdx >= (int)m_route.steps->size()) {
				step = &m_route.steps[state.m_stepIdx-1];
				state.pos.m_waypoint = step->waypoints[1];
				state.pos.m_nextWaypoint = -1;
				state.pos.m_floor = (state.pos.m_waypoint != -1) ? -1 : step->floors[1];
				break;
			} else {
				step = &m_route.steps[state.m_stepIdx];
				state.pos.m_waypoint = step->waypoints[0];
				state.pos.m_nextWaypoint = step->waypoints[1];
				state.pos.m_floor = (state.pos.m_waypoint != -1) ? -1 : step->floors[0];
			}
		}
	}

	for (int i = state.m_stepIdx+1; i < (int)m_route.steps->size(); ++i) {
		distanceRemainingAfterMove += m_route.steps[i].path.length;
	}

	return moved;
}

bool FloorMove::NextStep(State &state, StringVec &events) {
	if (m_route.steps->empty())
		return true;
	if (state.m_stepIdx >= (int)m_route.steps->size())
		return true;

	const Step *step = &m_route.steps[state.m_stepIdx];

	if (!step->events[1].empty)
		events.push_back(step->events[1]);

	step->path.Eval(1.f, state.pos.m_pos, &state.facing);

	++state.m_stepIdx;
	state.m_m.Init();
	state.m_first = true;

	if (state.m_stepIdx >= (int)m_route.steps->size()) {
		step = &m_route.steps[state.m_stepIdx-1];
		state.pos.m_waypoint = step->waypoints[1];
		state.pos.m_nextWaypoint = -1;
		state.pos.m_floor = step->floors[1];
	} else {
		step = &m_route.steps[state.m_stepIdx];
		state.pos.m_waypoint = step->waypoints[0];
		state.pos.m_nextWaypoint = step->waypoints[1];
		state.pos.m_floor = step->floors[0];
	}

	return (state.m_stepIdx >= (int)m_route.steps->size());
}

void FloorMove::Merge(const Ref &old, State &state) {
	if (state.m_stepIdx >= (int)old->m_route.steps->size()) {
		InitMove(state);
		return;
	}

	if (m_route.steps->empty()) {
		InitMove(state);
		return;
	}

	const Step &firstStep = m_route.steps->front();
	if (firstStep.waypoints[0] == -1) {
		// arbitrary floor position no merging to do
		InitMove(state);
		return;
	}

	RAD_ASSERT(state.pos.m_nextWaypoint != -1);

	if (firstStep.waypoints[0] != state.pos.m_waypoint) {
		// disjointed move.
		InitMove(state);
		return;
	}

	if (firstStep.waypoints[1] != state.pos.m_nextWaypoint) {
		// we are backtracking to our starting waypoint
		if (state.m_m.distance < 0.01f) {
			InitMove(state);
			return; // we haven't traveled so just proceed with new move.
		}

		// reverse course back to our starting waypoint
		const Step &curStep = old->m_route.steps[state.m_stepIdx];

		Step newStep(curStep);
		newStep.Reverse();

		float distance = curStep.path.length - state.m_m.distance;
		RAD_ASSERT(distance >= 0.f);
		
		Vec3 unused;
		state.m_m.Init();
		float moved = state.m_m.Eval(newStep.path, distance, unused, &state.facing);

		RAD_ASSERT(math::Abs(moved-distance) < 0.1f);

		state.m_first = false;
		state.m_stepIdx = 0;
		
		Route newRoute;
		newRoute.steps->push_back(newStep);
		std::copy(m_route.steps->begin(), m_route.steps->end(), std::back_inserter(*newRoute.steps));

		m_route.steps = newRoute.steps;
	} else {
		RAD_ASSERT(firstStep.waypoints[1] == state.pos.m_nextWaypoint);
		state.m_stepIdx = 0; // just reset our step counter
	}
}

bool FloorMove::ClampToEnd(State &state, bool posUpdate) {
	// this only applies to waypoint moves
	if (state.m_stepIdx < (int)m_route.steps->size()) {
		const Step &curStep = m_route.steps[state.m_stepIdx];
		if (curStep.waypoints[0] != -1 &&
			curStep.waypoints[1] != -1) {
			if (state.m_stepIdx == (int)(m_route.steps->size()-1)) {
				// last step in move.
				if ((curStep.floors[1] == -1) || posUpdate) {
					state.pos.m_waypoint = curStep.waypoints[1];
					state.pos.m_nextWaypoint = -1;
					state.pos.m_floor = curStep.floors[1];
					
					if (posUpdate) {
						curStep.path.Eval(1.f, state.pos.m_pos, &state.facing);
					}

					return true;
				}
			}
		}
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////

Floors::Floors() : m_floodNum(-1), m_world(0) {
}

Floors::~Floors() {
}

bool Floors::Load(World &world) {
	m_world = &world;
	return Load(*world.bspFile);
}

bool Floors::Load(const bsp_file::BSPFile &bsp) {
	m_bsp = &bsp;

	m_waypoints.reserve(m_bsp->numWaypoints);
	m_waypointIds.reserve(m_bsp->numWaypoints);

	for (U32 i = 0; i < m_bsp->numWaypoints; ++i) {
		const bsp_file::BSPWaypoint *waypoint = m_bsp->Waypoints() + i;

		Waypoint w;
		w.waypointId = waypoint->uid; // serializable
		w.flags = (int)waypoint->flags;
		w.floodNum = -1;
		w.floodDistance = 0.f;

		m_idToWaypoint.insert(IntMap::value_type(w.waypointId, (int)i));
		m_waypointIds.push_back(w.waypointId);

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

bool Floors::ClipToFloor(
	int floorNum,
	const Vec3 &start,
	const Vec3 &end,
	FloorPosition &pos
) const {
	RAD_ASSERT(floorNum < (int)m_bsp->numFloors);
	float bestDistSq = std::numeric_limits<float>::max();
	return ClipToFloor(floorNum, start, end, pos, bestDistSq);
}

FloorMove::Ref Floors::CreateMove(
	const FloorPosition &start,
	const FloorPosition &end
) const {
	WalkStep::Vec route;
	if (!Walk(start, end, route))
		return FloorMove::Ref();

	FloorMove::Ref move(new (ZWorld) FloorMove());
	GenerateFloorMove(route, move->m_route);
	return move;
}

FloorMove::Ref Floors::CreateMoveSeq(
	const FloorPosition::Vec &seq
) const {
	if (seq.size() < 2)
		return FloorMove::Ref();

	FloorPosition cur(seq[0]);

	WalkStep::Vec route;
	WalkStep::Vec work;

	FloorPosition::Vec::const_iterator start = seq.begin()+1;

	for (FloorPosition::Vec::const_iterator it = start; it != seq.end(); ++it) {
		const FloorPosition &next = *it;
		if (it != start) {
			if (!Walk(cur, next, work))
				return FloorMove::Ref(); // no connection
			RAD_ASSERT(!work->empty());
			std::copy(work->begin()+1, work->end(), std::back_inserter(*route));
			work->clear();
		} else { // done in-place
			if (!Walk(cur, next, route))
				return FloorMove::Ref();
		}
		cur = next;
	}

	FloorMove::Ref move(new (ZWorld) FloorMove());
	GenerateFloorMove(route, move->m_route);
	return move;
}

bool Floors::WaypointPosition(int waypoint, FloorPosition &pos) const {
	IntMap::const_iterator it = m_idToWaypoint.find(waypoint);
	if (it == m_idToWaypoint.end())
		return 0;

	waypoint = it->second;

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
	IntMap::const_iterator it = m_idToWaypoint.find(waypoint);
	if (it == m_idToWaypoint.end())
		return 0;

	waypoint = it->second;

	if (waypoint < 0 || (waypoint >= (int)m_waypoints.size()))
		return 0;

	return m_waypoints[waypoint].flags;
}

int Floors::WaypointStateByIdx(int waypointIdx) const {
	if (waypointIdx < 0 || (waypointIdx >= (int)m_waypoints.size()))
		return 0;

	return m_waypoints[waypointIdx].flags;
}

void Floors::SetWaypointState(int waypoint, int state) {
	IntMap::const_iterator it = m_idToWaypoint.find(waypoint);
	if (it == m_idToWaypoint.end())
		return;

	waypoint = it->second;

	if (waypoint < 0 || (waypoint >= (int)m_waypoints.size()))
		return;

	m_waypoints[waypoint].flags = state;
}

IntVec Floors::WaypointsForTargetname(const char *targetname) const {
	String s(CStr(targetname));
	std::pair<Waypoint::MMap::const_iterator,
		      Waypoint::MMap::const_iterator> pair = m_waypointTargets.equal_range(s);

	IntVec vec;
	vec.reserve(64);
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
	vec.reserve(64);
	while (pair.first != pair.second) {
		vec.push_back(pair.first->second);
		++pair.first;
	}

	return vec;
}

int Floors::PickWaypoint(
	float x,
	float y,
	float d,
	float dropDist
) {
	RAD_ASSERT(m_world);
	if (!m_world) // debug tools.
		return -1;

	d = d*d; // squared distances.

	struct Candidate {
		typedef stackify<zone_vector<Candidate, ZWorldT>::type, 16> Vec;
		int idx;
		float dd;
		float dist;

		bool operator < (const Candidate &c) const {
			return dist < c.dist;
		}
	};

	Trace trace;
	const Vec3 &fwd = m_world->camera->fwd;

	Candidate::Vec waypoints;

	for (U32 i = 0; i < m_bsp->numWaypoints; ++i) {
		if (!(m_waypoints[i].flags&kWaypointState_Enabled))
			continue;
		const bsp_file::BSPWaypoint *waypoint = m_bsp->Waypoints() + i;
		const Vec3 kPos(waypoint->pos[0], waypoint->pos[1], waypoint->pos[2]);

		Vec3 out;
		if (!m_world->draw->Project(kPos, out))
			continue;

		float dx = out[0]-x;
		float dy = out[1]-y;
		float dd = dx*dx + dy*dy;

		if (dd <= d) {
			// check line of sight
			trace.start = m_world->camera->pos;
			trace.end = kPos;
			trace.contents = bsp_file::kContentsFlag_SolidContents|bsp_file::kContentsFlag_Clip;
			if (!m_world->LineTrace(trace)) {
				// did not collide with world
				Candidate c;
				c.idx = (int)i;
				c.dd = dd;
				c.dist = fwd.Dot(kPos);
				waypoints->push_back(c);
			}
		}
	}

	std::sort(waypoints->begin(), waypoints->end());

	if (waypoints->empty())
		return -1;

	// drop waypoints that are further than dropDist from our best candidate.
	if (dropDist > 0.f) {
		for (int i = 1; i < (int)waypoints->size();) {
			const Candidate &c = waypoints[0];
			const Candidate &x = waypoints[i];
			if (math::Abs(x.dist-c.dist) > dropDist) {
				waypoints->erase(i+waypoints->begin());
			} else {
				++i;
			}
		}
	}

	int best = -1;
	float bestDist = std::numeric_limits<float>::max();

	for (int i = 0; i < (int)waypoints->size(); ++i) {
		const Candidate &x = waypoints[i];
		if (x.dd < bestDist) {
			best = x.idx;
			bestDist = x.dd;
		}
	}

	return m_waypoints[best].waypointId;
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

const char *Floors::FloorName(int floor) const {
	
	if (floor < 0 || floor >= (int)m_bsp->numFloors)
		return 0;

	const bsp_file::BSPFloor *fp = m_bsp->Floors() + floor;
	return m_bsp->String(fp->name);
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
		typedef stackify<zone_vector<Stack, ZWorldT>::type, 256> Vec;
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
		
	const bsp_file::BSPFloorTri *tri = m_bsp->FloorTris() + floor->firstTri + start.m_tri;

	// sort edges
	
	for (int i = 0; i < 3; ++i) {
		const bsp_file::BSPFloorEdge *edge = m_bsp->FloorEdges() + tri->edges[i];
		const Vec3 kMid = FindEdgePoint(cur.pos.m_pos, edge);
		cur.edges[i].distance = (kMid-end.m_pos).MagnitudeSquared();
		cur.edges[i].idx = i;
		cur.edges[i].visited = false;
	}

	std::sort(cur.edges.begin(), cur.edges.end());

	visited.set(cur.pos.m_tri);

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
			visited.reset(cur.pos.m_tri);
			cur = stack->back();
			stack->pop_back();
			tri = m_bsp->FloorTris() + floor->firstTri + cur.pos.m_tri;
			continue;
		}

		while (cur.numVisited < 3) {
			int curEdge = cur.edgeNum++;

			if (!cur.edges[curEdge].visited) {
				++cur.numVisited;
				cur.edges[curEdge].visited = true;

				int edgeNum = tri->edges[cur.edges[curEdge].idx];
				
				const bsp_file::BSPFloorEdge *edge = m_bsp->FloorEdges() + edgeNum;
				int otherSide = edge->tris[0] == (cur.pos.m_tri + floor->firstTri);
				int otherTriNum = (int)edge->tris[otherSide];
				if (otherTriNum != -1)
					otherTriNum -= (int)floor->firstTri;

				if ((otherTriNum != -1) && !visited.test(otherTriNum)) {
					Vec3 mid = FindEdgePoint(cur.pos.m_pos, edge);
					
					float d = (mid - cur.pos.m_pos).MagnitudeSquared();
					float cost = (mid - end.m_pos).MagnitudeSquared();

					if ((cur.distance+d+cost) < bestDistance) { // valid

						const bsp_file::BSPFloorTri *otherTri = m_bsp->FloorTris() + floor->firstTri + otherTriNum;

						// cross to next triangle.
						visited.set(otherTriNum);

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

						tri = m_bsp->FloorTris() + floor->firstTri + cur.pos.m_tri;
						
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

	const bsp_file::BSPFloor *floor = m_bsp->Floors() + start.m_floor;

	FloorPosition cur(start);

	while (cur.m_tri != end.m_tri) {
		float bestDist = std::numeric_limits<float>::max();
		int bestEdge = -1;
		Vec3 bestPos;

		const bsp_file::BSPFloorTri * tri = m_bsp->FloorTris() + floor->firstTri + cur.m_tri;

		for (int i = 0; i < 3; ++i) {
			if (tri->edges[i] == skipEdge)
				continue;

			const bsp_file::BSPFloorEdge *edge = m_bsp->FloorEdges() + tri->edges[i];
			const bsp_file::BSPPlane *plane = m_bsp->Planes() + edge->planenum;
			const Plane kPlane(plane->p[0], plane->p[1], plane->p[2], plane->p[3]);

			Vec3 x;
			if (kPlane.IntersectLineSegment(x, cur.m_pos, end.m_pos, 0.01f)) {
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
		int otherTriNum = (edge->tris[0] == (cur.m_tri+(int)floor->firstTri)) ? edge->tris[1] : edge->tris[0];

		if (otherTriNum == -1)
			return false; // hit solid space no direct path.

		const bsp_file::BSPFloorTri *otherTri = m_bsp->FloorTris() + otherTriNum;

		if (tri->planenum != otherTri->planenum) {
			step.pos = bestPos;
			step.tri = otherTriNum - floor->firstTri;
			step.slopeChange = true;
			route->push_back(step);
		}

		skipEdge = tri->edges[bestEdge]; // don't test this edge.
		cur.m_tri = otherTriNum - floor->firstTri;
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
	
	const int kSize = (int)original->size();

	for (int i = 0; i < kSize - 1; ++i) {
		const WalkStep &curStep = original[i];

		FloorPosition curPos;
		curPos.m_floor = curStep.floors[0];
		curPos.m_tri = curStep.tri;
		curPos.m_pos = curStep.pos;
		
		for (int k = kSize-1; k > i; --k) {
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

	// remove duplicates.
	bool removed = true;

	while (removed) {
		removed = false;
		for (size_t i = 0; i < route->size()-1; ++i) {
			const WalkStep &a = route[i];
			const WalkStep &b = route[i+1];
			if (a.pos.NearlyEquals(b.pos, 1.f)) {
				route->erase(route->begin()+i+1);
				removed = true;
				break;
			}
		}
	}
}

void Floors::OptimizeRoute2(WalkStep::Vec &route) const {
	if (route->empty())
		return;
	
	bool optimized = false;

	do {
		const int kSize = (int)route->size();

		if (kSize > 2) {
			WalkStep::Vec original(route);
			route->clear();

			int i;
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

bool Floors::Walk(
	const FloorPosition &_start,
	const FloorPosition &_end,
	WalkStep::Vec &walkRoute
) const {
	MovePlan plan;
	MovePlan planStack;
	FloorBits floors;
	WaypointBits waypoints;
	float bestDistance = std::numeric_limits<float>::max();

	// A floor move never sets the triangle number since motion along the move is arbitrary
	// and doing a floor intersection test is expensive for every move step. We can fill this 
	// information in here:

	FloorPosition start(_start);
	FloorPosition end(_end);

	if (start.m_waypoint != -1) {
		const bsp_file::BSPWaypoint *waypoint = m_bsp->Waypoints() + start.m_waypoint;
		start.m_floor = (int)waypoint->floorNum;
		start.m_tri = (int)waypoint->triNum;
		start.m_pos = Vec3(waypoint->pos[0], waypoint->pos[1], waypoint->pos[2]);
	} else if (start.m_floor != -1) { // arbitrary floor position
		if (start.m_tri == -1) {
			Vec3 x(start.m_pos + Vec3(0, 0, 16.f));
			Vec3 y(start.m_pos - Vec3(0, 0, 16.f));
			if (!ClipToFloor(start.m_floor, x, y, start)) {
				COut(C_Error) << "Floor starting position is outside floor!" << std::endl;
				return false;
			}
		}
	}
	
	if (end.m_waypoint != -1) {
		const bsp_file::BSPWaypoint *waypoint = m_bsp->Waypoints() + end.m_waypoint;
		end.m_floor = (int)waypoint->floorNum;
		end.m_tri = (int)waypoint->triNum;
		end.m_pos = Vec3(waypoint->pos[0], waypoint->pos[1], waypoint->pos[2]);
	} else if (end.m_floor != -1) { // arbitrary floor position
		if (end.m_tri == -1) {
			Vec3 x(end.m_pos + Vec3(0, 0, 16.f));
			Vec3 y(end.m_pos - Vec3(0, 0, 16.f));
			if (!ClipToFloor(end.m_floor, x, y, end)) {
				COut(C_Error) << "Floor ending position is outside floor!" << std::endl;
				return false;
			}
		}
	}

	++m_floodNum;
	if (!PlanMove(start, end, 0.f, plan, planStack, floors, waypoints, bestDistance))
		return false; // there is no path between these points.

	// We have a rough movement plan from start->end, build the walk command
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
		
		if (step.connection != -1) {
			// walk from waypoint->waypoint (through connection)
			WalkConnection(step.waypoint, step.connection, walkRoute);
		} else {
			if (waypointPos.m_pos.NearlyEquals(current.m_pos, 0.1f)) {
#if !defined(RAD_OPT_SHIP)
				RAD_VERIFY(current.m_floor == waypointPos.m_floor);
				RAD_VERIFY(current.m_tri == waypointPos.m_tri);
#endif
			} else {
				// walk from floor -> waypoint (waypoint exiting floor)
				// check that we are exiting (may be on an exit waypoint at this step).
#if !defined(RAD_OPT_SHIP)
				RAD_VERIFY(current.m_floor >= 0);
				RAD_VERIFY(current.m_floor == waypointPos.m_floor);
#endif
				workRoute->clear();
				WalkFloor(current, waypointPos, workRoute);
#if !defined(RAD_OPT_SHIP)
				if (workRoute->size() < 2) {
					int b = 0;
				}
				RAD_VERIFY(workRoute->size() > 1);
#endif
				if (workRoute->size() < 2)
					return false;
				std::copy(workRoute->begin(), workRoute->end(), std::back_inserter(*walkRoute)); 
			}
		}

		current = waypointPos;
	}

	// walk.
	if ((current.m_floor >= 0) && (end.m_waypoint == -1)) {
		if (end.m_pos.NearlyEquals(current.m_pos, 0.1f)) {
#if !defined(RAD_OPT_SHIP)
			RAD_VERIFY(current.m_floor == end.m_floor);
			RAD_VERIFY(current.m_tri == end.m_tri);
#endif
		} else {
			// entering a floor...
#if !defined(RAD_OPT_SHIP)
			RAD_VERIFY(current.m_floor == end.m_floor);
#endif
			workRoute->clear();
			WalkFloor(current, end, workRoute);
#if !defined(RAD_OPT_SHIP)
			if (workRoute->size() < 2) {
				int b = 0;
			}
			RAD_VERIFY(workRoute->size() > 1);
#endif
			if (workRoute->size() < 2)
				return false;
			std::copy(workRoute->begin(), workRoute->end(), std::back_inserter(*walkRoute));
		}
	}

	if (walkRoute->empty())
		return false; // waypoint move didn't generate a plan because we are at our destination waypoint already.
	
	return true;
}

#if defined(RAD_OPT_SHIP)
#define RANGE_CHECK(v)
#else
#define RANGE_CHECK(v) \
	RAD_VERIFY(math::Abs(v[0]) < 16000.f); \
	RAD_VERIFY(math::Abs(v[1]) < 16000.f); \
	RAD_VERIFY(math::Abs(v[2]) < 16000.f)
#endif

void Floors::GenerateFloorMove(const WalkStep::Vec &walkRoute, FloorMove::Route &moveRoute) const {
	if (walkRoute->empty())
		return;

	const float kSmoothness = 32.f;

	const int kSize = (int)walkRoute->size();

	for (int i = 0; i < kSize; ++i) {
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
				step.events[0] = CStr(m_bsp->String(connection->cmds[cmdSet*2+0]));
			if (connection->cmds[cmdSet*2+1] != -1)
				step.events[1] = CStr(m_bsp->String(connection->cmds[cmdSet*2+1]));

			if (connection->anims[dir] != -1)
				step.anim = CStr(m_bsp->String(connection->anims[dir]));

			step.spline.Init(
				cur.pos,
				Vec3(connection->ctrls[dir][0], connection->ctrls[dir][1], connection->ctrls[dir][2]),
				Vec3(connection->ctrls[!dir][0], connection->ctrls[!dir][1], connection->ctrls[!dir][2]),
				Vec3(waypoint->pos[0], waypoint->pos[1], waypoint->pos[2])
			);

			step.path.Load(step.spline);
			moveRoute.steps->push_back(step);
			continue;
		}

		if (i >= (kSize-1)) {
#if !defined(RAD_OPT_SHIP)
			RAD_VERIFY(!moveRoute.steps->empty());
#endif
			break; // no more moves
		}

		const WalkStep &next = walkRoute[i+1];

		// we walk to waypoints, cur.pos == next.waypoints.pos
		if (next.waypoints[0] != -1)
			continue;
		
		Vec3 ctrls[2];
		Vec3 vNext = next.pos - cur.pos;
	
		float nextLen = vNext.Normalize();
		
		RANGE_CHECK(cur.pos);

		// make move continous in X, Y. Preserve motion exactly in Z (don't want to float above/below
		// floor).
		
		if (i > 0) {
			const WalkStep &prev = walkRoute[i-1];
			Vec3 prevPos;

			if (prev.waypoints[0] != -1) {
				const bsp_file::BSPWaypoint *waypoint = m_bsp->Waypoints() + prev.waypoints[0];
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


				
		} else {
			ctrls[0] = vNext;
		}

		float len = std::min(nextLen / 4.f, kSmoothness);
		ctrls[0] = cur.pos + (ctrls[0] * len);

		if (i < (kSize-2)) {
			const WalkStep &nextNext = walkRoute[i+2];
			Vec3 vNextNext;
				
			if (nextNext.waypoints[0] != -1) {
				const bsp_file::BSPWaypoint *waypoint = m_bsp->Waypoints() + nextNext.waypoints[1];
				vNextNext = Vec3(waypoint->pos[0], waypoint->pos[1], waypoint->pos[2]) - next.pos;
			} else {
				vNextNext = nextNext.pos - next.pos;
			}

			float nnLen = vNextNext.Normalize();

			ctrls[1] = vNextNext + vNext;
			// isolate Z
			ctrls[1][2] = 0.f;
			ctrls[1].Normalize();
			ctrls[1][2] = vNext[2];

			ctrls[1] = -ctrls[1];
		} else {
			ctrls[1] = -vNext;
		}

		ctrls[1] = next.pos + (ctrls[1]*len);
		
		RANGE_CHECK(ctrls[0]);
		RANGE_CHECK(ctrls[1]);
		RANGE_CHECK(next.pos);

		physics::CubicBZSpline spline(cur.pos, ctrls[0], ctrls[1], next.pos);
		step.floors[0] = cur.floors[0];
		step.floors[1] = next.floors[0];
		step.waypoints[0] = -1;
		step.waypoints[1] = next.waypoints[0];
		step.connection = -1;
		step.flags = FloorMove::State::kStateFlag_AutoFace|FloorMove::State::kStateFlag_Interruptable;
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
	// done on stack to avoid overflow

	if ((start.m_waypoint != -1) && (start.m_waypoint == end.m_waypoint))
		return true; // at destination
	if ((start.m_floor != -1) && (start.m_floor == end.m_floor) && (end.m_waypoint == -1))
		return true; // at destination.

	++m_floodNum;

	planSoFar.start = start;
	planSoFar.end   = end;
	planSoFar.steps->clear();
	
	bestDistance = std::numeric_limits<float>::max();
	
	struct Connection {
		typedef stackify<zone_vector<Connection, ZWorldT>::type, 16> Vec;

		FloorPosition pos;

		const bsp_file::BSPWaypoint *nextWaypoint;
		const bsp_file::BSPFloor *nextFloor;
		const bsp_file::BSPWaypointConnection *connection;

		int nextWaypointIdx;
		int connectionIdx;
		int nextFloorIdx;

		float cost; // <-- distace from position to the waypoint
		float distance; // <-- distance from waypoint to the target

		bool operator < (const Connection &c) const {
			return distance < c.distance;
		}
	};

	struct Stack {
		typedef stackify<zone_vector<Stack, ZWorldT>::type, 512> Vec;
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

			// waypoint is not enabled?
			if (!(m_waypoints[c.nextWaypointIdx].flags&kWaypointState_Enabled))
				continue;

			if ((dir == 1) && !(c.connection->flags&bsp_file::kWaypointConnectionFlag_AtoB))
				continue; // cannot go this way.
			if ((dir == 0) && !(c.connection->flags&bsp_file::kWaypointConnectionFlag_BtoA))
				continue; // cannot go this way.

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

		if (waypoint->floorNum >= 0) {
			// on a floor, add waypoints
			const bsp_file::BSPFloor *floor = m_bsp->Floors() + waypoint->floorNum;

			for (S32 i = 0; i < floor->numWaypoints; ++i) {
				Connection c;
				c.nextFloor = 0;
				c.connection = 0;
				c.nextFloorIdx = -1;
				c.connectionIdx = -1;
				c.nextWaypointIdx = (int)*(m_bsp->WaypointIndices() + floor->firstWaypoint + i);

				if (c.nextWaypointIdx == start.m_waypoint)
					continue; // don't add ourselves

				c.nextWaypoint = m_bsp->Waypoints() + c.nextWaypointIdx;

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

			RAD_ASSERT(start.m_floor != -1);
			floors.set(start.m_floor);
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

			// waypoint is not enabled?
			if (!(m_waypoints[c.nextWaypointIdx].flags&kWaypointState_Enabled))
				continue;

			c.nextWaypoint = m_bsp->Waypoints() + c.nextWaypointIdx;

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

				MoveStep step;
				step.waypoint = c.nextWaypointIdx;
				step.connection = c.connectionIdx;
				planSoFar.steps->push_back(step);
				
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

					// waypoint is not enabled?
					if (!(m_waypoints[c.nextWaypointIdx].flags&kWaypointState_Enabled))
						continue;
					if ((dir == 1) && !(c.connection->flags&bsp_file::kWaypointConnectionFlag_AtoB))
						continue; // cannot go this way.
					if ((dir == 0) && !(c.connection->flags&bsp_file::kWaypointConnectionFlag_BtoA))
						continue; // cannot go this way.

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

				if (waypoint->floorNum >= 0) {
					if (m_floorState[waypoint->floorNum]&kFloorState_Enabled) {
						// on a floor, add waypoints
						const bsp_file::BSPFloor *floor = m_bsp->Floors() + waypoint->floorNum;

						for (S32 i = 0; i < floor->numWaypoints; ++i) {
							Connection c;
							c.nextFloor = 0;
							c.connection = 0;
							c.nextFloorIdx = -1;
							c.connectionIdx = -1;
							c.nextWaypointIdx = (int)*(m_bsp->WaypointIndices() + floor->firstWaypoint + i);

							if (c.nextWaypointIdx == cur.pos.m_waypoint)
								continue; // don't add ourselves

							// waypoint is not enabled?
							if (!(m_waypoints[c.nextWaypointIdx].flags&kWaypointState_Enabled))
								continue;

							c.nextWaypoint = m_bsp->Waypoints() + c.nextWaypointIdx;

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
					}
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
	Trace trace;

	RAD_ASSERT(floor->numTris > 0);

	int bestTri = -1;

	for (U32 i = 0; i < floor->numTris; ++i) {
		U32 triNum = floor->firstTri + i;
		const bsp_file::BSPFloorTri *tri = m_bsp->FloorTris() + triNum;

		const bsp_file::BSPPlane *plane = m_bsp->Planes() + tri->planenum;
		const Plane kTriPlane(plane->p[0], plane->p[1], plane->p[2], plane->p[3]);

		Vec3 clip;

		if (!kTriPlane.IntersectLineSegment(clip, start, end, 0.01f))
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

			if ((edge->tris[0] == -1) || (edge->tris[1] == -1)) {
				// must be well within floor edge
				if (edgePlane.Side(clip, 0.01f) != Plane::Front)
					break;
			} else if (edgePlane.Side(clip) == Plane::Back) {
				break;
			}
		}

		if (k == 3) {
			// valid line trace?
			trace.start = start;
			trace.end = clip;
			trace.contents = bsp_file::kContentsFlag_Solid|bsp_file::kContentsFlag_Clip;
			
			// debug tools don't give us a world object.
			if (m_world && !m_world->LineTrace(trace)) {
				// did not collide with world.
				bestDistSq = distSq;
				pos.m_pos = clip;
				pos.m_floor = (int)floorNum;
				pos.m_waypoint = -1;
				pos.m_nextWaypoint = -1;
				bestTri = (int)i;
				pos.m_tri = bestTri;
			}
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
	lua_setfield(L, -2, "pos");
	lua_pushinteger(L, (int)val.m_floor);
	lua_setfield(L, -2, "floor");
	lua_pushinteger(L, (int)val.m_tri);
	lua_setfield(L, -2, "tri");
	lua_pushinteger(L, (int)val.m_waypoint);
	lua_setfield(L, -2, "waypoint");
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

	lua_getfield(L, index, "pos");
	p.m_pos = Marshal<Vec3>::Get(L, -1, luaError);
	lua_pop(L, 1);

	lua_getfield(L, index, "floor");
	if (lua_type(L, -1) != LUA_TNUMBER) {
		lua_pop(L, 1);
		if (luaError)
			luaL_typerror(L, index, "floor");
		return p;
	}
	p.m_floor = (int)lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, index, "tri");
	if (lua_type(L, -1) != LUA_TNUMBER) {
		lua_pop(L, 1);
		if (luaError)
			luaL_typerror(L, index, "tri");
		return p;
	}

	p.m_tri = (int)lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, index, "waypoint");
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
