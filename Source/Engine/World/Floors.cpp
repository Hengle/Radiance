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

void FloorMove::InitMove(
	FloorPosition &pos,
	String &animState
) {
}

bool FloorMove::Move(
	FloorPosition &pos,
	float velocity,
	String &animState
) {
	return false;
}

bool FloorMove::RAD_IMPLEMENT_GET(busy) {
	return false;
}

///////////////////////////////////////////////////////////////////////////////

Floors::Floors() {
}

Floors::~Floors() {
}

bool Floors::Load(const bsp_file::BSPFile::Ref &bsp) {
	m_bsp = bsp;
	return true;
}

bool Floors::ClipToFloor(
	const Vec3 &start,
	const Vec3 &end,
	FloorPosition &pos
) {
	float bestDistSq = std::numeric_limits<float>::max();
	bool r = false;

	for (U32 i = 0; i < m_bsp->numFloors; ++i) {
		if (ClipToFloor(i, start, end, pos, bestDistSq)) {
			r = true;
		}
	}

	return r;
}

FloorMove::Ref Floors::CreateMove(
	const FloorPosition &start,
	const FloorPosition &end
) {
	MovePlan plan;
	MovePlan planStack;
	FloorBits stack;
	WalkStep::Vec walkRoute;
	float bestDistance = std::numeric_limits<float>::max();

	if (!PlanMove(start, end, 0.f, plan, planStack, stack, bestDistance))
		return FloorMove::Ref(); // there is no path between these points.

	// We have a rough movement plan from start->end, build the walk commands

	FloorMove::Ref move(new (ZWorld) FloorMove());
	move->m_pos = start;

	FloorPosition current(start);

	for (MoveStep::Vec::const_iterator it = plan.steps->begin(); it != plan.steps->end(); ++it) {
		const MoveStep &step = *it;

		// walk from current -> waypoint
		const bsp_file::BSPWaypoint *waypoint = m_bsp->Waypoints() + step.waypoint;
		
		FloorPosition waypointPos;
		waypointPos.m_floor = (int)waypoint->floorNum;
		waypointPos.m_tri = (int)waypoint->triNum;
		waypointPos.m_pos = Vec3(waypoint->pos[0], waypoint->pos[1], waypoint->pos[2]);

		RAD_ASSERT(current.m_floor == waypointPos.m_floor);
		walkRoute->clear();
		WalkFloor(current, waypointPos, walkRoute);
		AddWalkRoute(current.m_floor, walkRoute, move->m_route);
		
		// walk from waypoint->waypoint (through connection)
		WalkConnection(step.waypoint, step.connection, move->m_route);

		const bsp_file::BSPWaypointConnection *connection = m_bsp->WaypointConnections() + step.connection;
		int otherIdx = connection->waypoints[0] == step.waypoint;
		const bsp_file::BSPWaypoint *otherWaypoint = m_bsp->Waypoints() + connection->waypoints[otherIdx];

		current.m_floor = (int)otherWaypoint->floorNum;
		current.m_tri = (int)otherWaypoint->triNum;
		current.m_pos = Vec3(otherWaypoint->pos[0], otherWaypoint->pos[1], otherWaypoint->pos[2]);
	}

	// walk.

	RAD_ASSERT(current.m_floor == end.m_floor);
	walkRoute->clear();
	WalkFloor(current, end, walkRoute);
	AddWalkRoute(current.m_floor, walkRoute, move->m_route);

	SmoothRoute(move->m_route);

	return move;
}

void Floors::WalkFloor(
	const FloorPosition &start,
	const FloorPosition &end,
	WalkStep::Vec &route
) {
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
	step.required = true;
	routeSoFar->push_back(step);
		
	const bsp_file::BSPFloorTri *tri = m_bsp->FloorTris() + start.m_tri;

	// sort edges
	
	for (int i = 0; i < 3; ++i) {
		const bsp_file::BSPFloorEdge *edge = m_bsp->FloorEdges() + tri->edges[i];
		const Vec3 kMid(edge->midpoint[0], edge->midpoint[1], edge->midpoint[2]);
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
					step.required = true;
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
					Vec3 mid(edge->midpoint[0], edge->midpoint[1], edge->midpoint[2]);
					
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
						step.required = false;
						routeSoFar->push_back(step);

						tri = m_bsp->FloorTris() + cur.pos.m_tri;
						
						for (int i = 0; i < 3; ++i) {
							cur.edges[i].idx = i;
							if (tri->edges[i] == edgeNum) {
								cur.edges[i].visited = true;
								cur.edges[i].distance = std::numeric_limits<float>::max();
							} else {
								edge = m_bsp->FloorEdges() + tri->edges[i];
								mid.Initialize(edge->midpoint[0], edge->midpoint[1], edge->midpoint[2]);
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

	OptimizeRoute(start, route);
}

void Floors::WalkConnection(
	int waypointNum,
	int connectionNum,
	FloorMove::Route &route
) {
	const bsp_file::BSPWaypoint *waypoint = m_bsp->Waypoints() + waypointNum;
	const bsp_file::BSPWaypointConnection *connection = m_bsp->WaypointConnections() + connectionNum;

	int side = connection->waypoints[1] == waypointNum;

	const bsp_file::BSPWaypoint *otherWaypoint = m_bsp->Waypoints() + connection->waypoints[!side];

	const Vec3 kWaypointPos(waypoint->pos[0], waypoint->pos[1], waypoint->pos[2]);
	const Vec3 kOtherWaypointPos(otherWaypoint->pos[0], otherWaypoint->pos[1], otherWaypoint->pos[2]);
	const Vec3 kWaypointCtrl(connection->ctrls[side][0], connection->ctrls[side][1], connection->ctrls[side][2]);
	const Vec3 kOtherWaypointCtrl(connection->ctrls[!side][0], connection->ctrls[!side][1], connection->ctrls[!side][2]);

	physics::CubicBZSpline spline(kWaypointPos, kWaypointCtrl, kOtherWaypointCtrl, kOtherWaypointPos);

	FloorMove::Step step;
	step.floors[0] = (int)waypoint->floorNum;
	step.floors[1] = (int)otherWaypoint->floorNum;
	step.waypoints[0] = waypointNum;
	step.waypoints[1] = (int)connection->waypoints[!side];
	step.path.Load(spline);

	route.steps->push_back(step);
}

bool Floors::FindDirectRoute(const FloorPosition &start, const FloorPosition &end, WalkStep::Vec &route) {

	RAD_ASSERT(start.m_floor == end.m_floor);

	// follow the ray to the destination.
	WalkStep step;
	step.pos = start.m_pos;
	step.required = true;
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
			if (kPlane.IntersectLineSegment(x, cur.m_pos, end.m_pos)) {
				float d = (x - cur.m_pos).MagnitudeSquared();
				if (d < bestDist) {
					bestDist = d;
					bestEdge = i;
					bestPos = x;
				}
			}
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
			step.required = true;
		}

		skipEdge = tri->edges[bestEdge]; // don't test this edge.
		cur.m_tri = otherTriNum;
		cur.m_pos = bestPos;
	}

	step.pos = end.m_pos;
	step.required = true;
	route->push_back(step);

	return true;
}

void Floors::OptimizeRoute(const FloorPosition &start, WalkStep::Vec &route) {
}

void Floors::AddWalkRoute(int floor, const WalkStep::Vec &steps, FloorMove::Route &route) {
	const size_t kSize = steps->size();

	for (size_t i = 0; i < kSize - 1; ++i) {
		FloorMove::Step step;
		step.floors[0] = step.floors[1] = floor;
		step.waypoints[0] = step.waypoints[1] = -1;

		const WalkStep &step0 = steps[i];
		const WalkStep &step1 = steps[i+1];
		const Vec3 kMid((step0.pos + step1.pos) * 0.5f);
		
		physics::CubicBZSpline spline(step0.pos, kMid, kMid, step1.pos);
		step.path.Load(spline);
		route.steps->push_back(step);
	}
}

void Floors::SmoothRoute(FloorMove::Route &route) {
}

bool Floors::PlanMove(
	const FloorPosition &start,
	const FloorPosition &end,
	float distance,
	MovePlan &plan,
	MovePlan &planSoFar,
	FloorBits stack,
	float &bestDistance
) {
	RAD_ASSERT(!stack.test(start.m_floor));
	
	if (start.m_floor == end.m_floor) {
		Vec3 seg = end.m_pos - start.m_pos;
		distance += seg.MagnitudeSquared();
		if (distance < bestDistance) {
			bestDistance = distance;
			plan = planSoFar;
		}
		return true;
	}
	
	stack.set(start.m_floor);

	// follow all waypoints out.
	bool foundMove = false;

	const bsp_file::BSPFloor *floor = m_bsp->Floors() + start.m_floor;
	for (U32 i = 0; i < floor->numWaypoints; ++i) {
		U16 waypointNum = *(m_bsp->WaypointIndices() + floor->firstWaypoint + i);
		const bsp_file::BSPWaypoint *waypoint = m_bsp->Waypoints() + waypointNum;

		const Vec3 kWaypointPos(waypoint->pos[0], waypoint->pos[1], waypoint->pos[2]);

		float estimatedDistanceToWaypoint = (kWaypointPos - start.m_pos).MagnitudeSquared() + distance;

		// NOTE: Connections are sorted by distance in the bsp file.

		for (U32 k = 0; k < waypoint->numConnections; ++k) {
			U16 connectionNum = *(m_bsp->WaypointIndices() + waypoint->firstConnection + k);
			const bsp_file::BSPWaypointConnection *connection = m_bsp->WaypointConnections() + connectionNum;

			int otherIdx = connection->waypoints[0] == waypointNum;

			const bsp_file::BSPWaypoint *otherWaypoint = m_bsp->Waypoints() + connection->waypoints[otherIdx];

			if (stack.test(otherWaypoint->floorNum))
				continue; // came from here

			const Vec3 kOtherWaypointPos(otherWaypoint->pos[0], otherWaypoint->pos[1], otherWaypoint->pos[2]);

			float estimatedDistanceToOtherWaypoint = (kOtherWaypointPos - kWaypointPos).MagnitudeSquared() + estimatedDistanceToWaypoint;

			if (estimatedDistanceToOtherWaypoint >= bestDistance)
				continue; // can't be better

			// go to other floor.
			FloorPosition waypointStart;
			waypointStart.m_pos = kOtherWaypointPos;
			waypointStart.m_floor = (int)otherWaypoint->floorNum;
			waypointStart.m_tri = (int)otherWaypoint->triNum;

			MoveStep step;
			step.waypoint = (int)waypointNum;
			step.connection = (int)connectionNum;
			
			planSoFar.steps->push_back(step);

			if (PlanMove(
				waypointStart,
				end,
				estimatedDistanceToOtherWaypoint,
				plan,
				planSoFar,
				stack,
				bestDistance)) {
					foundMove = true;
			}

			planSoFar.steps->pop_back();
		}
	}

	stack.reset(start.m_floor);

	return foundMove;
}

bool Floors::ClipToFloor(
	U32 floorNum,
	const Vec3 &start,
	const Vec3 &end,
	FloorPosition &pos,
	float &bestDistSq
) {
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
