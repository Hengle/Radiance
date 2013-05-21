// WorldBSP.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "World.h"
#include "Occupant.h"
#include "Light.h"
#include "../MathUtils.h"
#include <algorithm>

namespace world {

void World::SetAreaportalState(int areaportalNum, bool open, bool relinkOccupants) {
	RAD_ASSERT(areaportalNum < (int)m_bsp->numAreaportals.get());
	dAreaportal &areaportal = m_areaportals[areaportalNum];
	areaportal.open = open;

	if (!relinkOccupants)
		return;

	UpdateAreaVis(-1);

	EntityPtrSet ents;
	MBatchOccupantPtrSet occupants;
	LightPtrSet lights;

	for (dBSPLeaf::Vec::const_iterator it = m_leafs.begin(); it != m_leafs.end(); ++it) {
		const dBSPLeaf &leaf = *it;

		for (EntityPtrSet::iterator it = leaf.entities.begin(); it != leaf.entities.end(); ++it) {
			Entity *entity = *it;
			RAD_ASSERT(entity);

			if (leaf.area != entity->m_leaf->area) // foreign occupant?
				ents.insert(entity);
		}

		for (MBatchOccupantPtrSet::iterator it = leaf.occupants.begin(); it != leaf.occupants.end(); ++it) {
			MBatchOccupant *occupant = *it;
			if (leaf.area != occupant->m_leaf->area)
				occupants.insert(occupant);
		}

		for (LightPtrSet::iterator it = leaf.lights.begin(); it != leaf.lights.end(); ++it) {
			Light *light = *it;
			if (leaf.area != light->m_leaf->area)
				lights.insert(light);
		}
	}

	for (EntityPtrSet::const_iterator it = ents.begin(); it != ents.end(); ++it) {
		(*it)->Link();
	}

	for (MBatchOccupantPtrSet::iterator it = occupants.begin(); it != occupants.end(); ++it) {
		(*it)->Link();
	}

	for (LightPtrSet::const_iterator it = lights.begin(); it != lights.end(); ++it) {
		(*it)->Link();
	}
}

void World::UpdateAreaVis(int area) {
	AreaBits visited;

	if (area >= 0) {
		m_areaVis[area].reset();
		UpdateAreaVis(area, m_areaVis[area], visited);
		return;
	}

	for (int i = 0; i < (int)m_bsp->numAreas; ++i) {
		visited.reset();
		m_areaVis[i].reset();
		UpdateAreaVis(i, m_areaVis[i], visited);
	}
}

void World::UpdateAreaVis(int area, AreaBits &vis, AreaBits &visited) {
	vis.set(area);
	visited.set(area);

	const bsp_file::BSPArea *bspArea = m_bsp->Areas() + area;

	for (int i = 0; i < (int)bspArea->numPortals; ++i) {
		const int kPortalIndex = (int)*(m_bsp->AreaportalIndices() + bspArea->firstPortal + i);

		if (!m_areaportals[kPortalIndex].open)
			continue;

		const bsp_file::BSPAreaportal *portal = m_bsp->Areaportals() + kPortalIndex;
		const int kOtherSide = portal->areas[0] == area;
		const int kOtherArea = portal->areas[kOtherSide];

		if (!visited.test(kOtherArea)) {
			UpdateAreaVis(kOtherArea, vis, visited);
		}
	}
}

Entity::Vec World::BBoxTouching(const BBox &bbox, int classbits) const {
	Entity::Vec touching;
	EntityBits checked;

	if (m_nodes.empty()) {
		BBoxTouching(bbox, classbits, -1, touching, checked);
	} else {
		BBoxTouching(bbox, classbits, 0, touching, checked);
	}

	return touching;
}

void World::BBoxTouching(
	const BBox &bbox,
	int classbits,
	int nodeNum,
	Entity::Vec &out,
	EntityBits &checked
) const {
	if (nodeNum < 0) {
		nodeNum = -(nodeNum + 1);
		RAD_ASSERT(nodeNum < (int)m_leafs.size());
		const dBSPLeaf &leaf = m_leafs[nodeNum];

		for (EntityPtrSet::const_iterator it = leaf.entities.begin(); it != leaf.entities.end(); ++it) {
			Entity *entity = *it;
			if (checked[entity->m_id])
				continue;
			checked.set(entity->m_id);
			if (!entity->classbits)
				continue;
			if ((classbits != kEntityClassBits_Any) && !(classbits&entity->classbits))
				continue;
			
			BBox b(entity->ps->bbox);
			b.Translate(entity->ps->worldPos);
					
			if (bbox.Touches(b))
				out.push_back(entity->shared_from_this());
		}

		return;
	}

	const dBSPNode &node = m_nodes[nodeNum];
	const Plane &plane = m_planes[node.planenum];

	Plane::SideType s = plane.Side(bbox);

	// PERFORMANCE NOTE: since we do not split the box at the node we may end up over-checking the node children
	// (the portion of the box contained in the half-space of the node would not cross some children
	// however the unrestricted box does).

	if ((s == Plane::Front) || (s == Plane::Cross))
		BBoxTouching(bbox, classbits, node.children[0], out, checked);
	if ((s == Plane::Back) || (s == Plane::Cross))
		BBoxTouching(bbox, classbits, node.children[1], out, checked);
}

Entity::Ref World::FirstBBoxTouching(const BBox &bbox, int classbits) const {
	Entity::Ref touching;
	EntityBits checked;

	if (m_nodes.empty()) {
		touching = FirstBBoxTouching(bbox, classbits, -1, checked);
	} else {
		touching = FirstBBoxTouching(bbox, classbits, 0, checked);
	}

	return touching;
}

Entity::Ref World::FirstBBoxTouching(
	const BBox &bbox,
	int classbits,
	int nodeNum,
	EntityBits &bits
) const {
	if (nodeNum < 0) {
		nodeNum = -(nodeNum + 1);
		RAD_ASSERT(nodeNum < (int)m_leafs.size());
		const dBSPLeaf &leaf = m_leafs[nodeNum];

		for (EntityPtrSet::const_iterator it = leaf.entities.begin(); it != leaf.entities.end(); ++it) {
			Entity *entity = *it;
			if (bits[entity->m_id])
				continue;
			bits.set(entity->m_id);
			if (!entity->classbits)
				continue;
			if ((classbits != kEntityClassBits_Any) && !(classbits&entity->classbits))
				continue;
			

			BBox b(entity->ps->bbox);
			b.Translate(entity->ps->worldPos);
					
			if (bbox.Touches(b))
				return entity->shared_from_this();
		}

		return Entity::Ref();
	}

	const dBSPNode &node = m_nodes[nodeNum];
	const Plane &plane = m_planes[node.planenum];

	Plane::SideType s = plane.Side(bbox);

	// PERFORMANCE NOTE: since we do not split the box at the node we may end up over-checking the node children
	// (the portion of the box contained in the half-space of the node would not cross some children
	// however the unrestricted box does).

	Entity::Ref touching;

	if ((s == Plane::Front) || (s == Plane::Cross))
		touching = FirstBBoxTouching(bbox, classbits, node.children[0], bits);
	if (!touching && ((s == Plane::Back) || (s == Plane::Cross)))
		touching = FirstBBoxTouching(bbox, classbits, node.children[1], bits);

	return touching;
}

void World::LinkEntity(Entity &entity, const BBox &bounds) {
	m_draw->InvalidateInteractions(entity);
	InternalUnlinkEntity(entity);

	if (entity.ps->otype == kOccupantType_None)
		return;

	entity.m_leaf = LeafForPoint(bounds.Origin());
	if (!entity.m_leaf)
		return;

	if (entity.ps->otype == kOccupantType_Sky) {
		entity.m_areas.insert(kSkyArea);
		dBSPArea &area = m_areas[kSkyArea];
		area.entities.insert(&entity);
		return;
	}

	AreaBits visible;
	
	if (entity.m_leaf->area > -1) {
		visible.set(entity.m_leaf->area);

		if (entity.ps->otype == kOccupantType_Volume) {
			StackWindingStackVec bbox;
			BoundWindings(bounds, bbox);
			ClipOccupantVolume(&entity.ps->pos, &bbox, bounds, 0, entity.m_leaf->area, -1, visible);
		} else {
			RAD_ASSERT(entity.ps->otype == kOccupantType_BBox);
			ClipOccupantVolume(0, 0, bounds, 0, entity.m_leaf->area, -1, visible);
		}
	}

	LinkEntityParms constArgs(entity, bounds, visible);

	entity.m_bspLeafs.reserve(8);
	if (m_nodes.empty()) {
		LinkEntity(constArgs, -1);
	} else {
		LinkEntity(constArgs, 0);
	}
}

void World::UnlinkEntity(Entity &entity) {
	m_draw->UnlinkEntity(entity);
	InternalUnlinkEntity(entity);
}

void World::InternalUnlinkEntity(Entity &entity) {
	
	for (dBSPLeaf::PtrVec::const_iterator it = entity.m_bspLeafs.begin(); it != entity.m_bspLeafs.end(); ++it) {
		dBSPLeaf *leaf = *it;
		leaf->entities.erase(&entity);
	}

	for (IntSet::const_iterator it = entity.m_areas.begin(); it != entity.m_areas.end(); ++it) {
		dBSPArea &area = m_areas[*it];
		area.entities.erase(&entity);
	}

	entity.m_bspLeafs.clear();
	entity.m_areas.clear();
	entity.m_leaf = 0;
}

void World::LinkEntity(const LinkEntityParms &constArgs, int nodeNum) {
	if (nodeNum < 0) {
		nodeNum = -(nodeNum + 1);
		RAD_ASSERT(nodeNum < (int)m_leafs.size());
		dBSPLeaf &leaf = m_leafs[nodeNum];

		if ((leaf.area > -1) && constArgs.visible.test(leaf.area)) {
			leaf.entities.insert(&constArgs.entity);
			constArgs.entity.m_bspLeafs.push_back(&leaf);
			constArgs.entity.m_areas.insert(leaf.area);
			dBSPArea &area = m_areas[leaf.area];
			area.entities.insert(&constArgs.entity);
			m_draw->LinkEntity(
				constArgs.entity, 
				constArgs.bounds, 
				nodeNum,
				leaf,
				area
			);
		}

		return;
	}

	RAD_ASSERT(nodeNum < (int)m_nodes.size());
	const dBSPNode &node = m_nodes[nodeNum];
	RAD_ASSERT(node.planenum < (int)m_planes.size());
	const Plane &p = m_planes[node.planenum];

	Plane::SideType side = p.Side(constArgs.bounds, 0.0f);
	
	if ((side == Plane::Cross) || (side == Plane::Front))
		LinkEntity(constArgs, node.children[0]);
	if ((side == Plane::Cross) || (side == Plane::Back))
		LinkEntity(constArgs, node.children[1]);
}

void World::LinkOccupant(MBatchOccupant &occupant, const BBox &bounds) {
	m_draw->InvalidateInteractions(occupant);
	InternalUnlinkOccupant(occupant);
	
	occupant.m_leaf = LeafForPoint(bounds.Origin());
	if (!occupant.m_leaf)
		return;

	if (occupant.sky) {
		occupant.m_areas.insert(kSkyArea);
		dBSPArea &area = m_areas[kSkyArea];
		area.occupants.insert(&occupant);
		return;
	}

	AreaBits visible;
	
	if (occupant.m_leaf->area > -1) {
		visible.set(occupant.m_leaf->area);
		ClipOccupantVolume(0, 0, bounds, 0, occupant.m_leaf->area, -1, visible);
	}

	LinkOccupantParms constArgs(occupant, bounds, visible);

	occupant.m_bspLeafs.reserve(8);
	if (m_nodes.empty()) {
		LinkOccupant(constArgs, -1);
	} else {
		LinkOccupant(constArgs, 0);
	}
}

void World::UnlinkOccupant(MBatchOccupant &occupant) {
	m_draw->UnlinkOccupant(occupant);
	InternalUnlinkOccupant(occupant);
}

void World::InternalUnlinkOccupant(MBatchOccupant &occupant) {
	
	for (dBSPLeaf::PtrVec::const_iterator it = occupant.m_bspLeafs.begin(); it != occupant.m_bspLeafs.end(); ++it) {
		dBSPLeaf *leaf = *it;
		leaf->occupants.erase(&occupant);
	}

	for (IntSet::const_iterator it = occupant.m_areas.begin(); it != occupant.m_areas.end(); ++it) {
		dBSPArea &area = m_areas[*it];
		area.occupants.erase(&occupant);
	}

	occupant.m_bspLeafs.clear();
	occupant.m_areas.clear();
	occupant.m_leaf = 0;
}

void World::LinkOccupant(const LinkOccupantParms &constArgs, int nodeNum) {
	if (nodeNum < 0) {
		nodeNum = -(nodeNum + 1);
		RAD_ASSERT(nodeNum < (int)m_leafs.size());
		dBSPLeaf &leaf = m_leafs[nodeNum];

		if ((leaf.area > -1) && constArgs.visible.test(leaf.area)) {
			leaf.occupants.insert(&constArgs.occupant);
			constArgs.occupant.m_bspLeafs.push_back(&leaf);
			constArgs.occupant.m_areas.insert(leaf.area);
			dBSPArea &area = m_areas[leaf.area];
			area.occupants.insert(&constArgs.occupant);
			m_draw->LinkOccupant(
				constArgs.occupant, 
				constArgs.bounds, 
				nodeNum,
				leaf,
				area
			);
		}

		return;
	}

	RAD_ASSERT(nodeNum < (int)m_nodes.size());
	const dBSPNode &node = m_nodes[nodeNum];
	RAD_ASSERT(node.planenum < (int)m_planes.size());
	const Plane &p = m_planes[node.planenum];

	Plane::SideType side = p.Side(constArgs.bounds, 0.0f);
	
	if ((side == Plane::Cross) || (side == Plane::Front))
		LinkOccupant(constArgs, node.children[0]);
	if ((side == Plane::Cross) || (side == Plane::Back))
		LinkOccupant(constArgs, node.children[1]);
}

void World::LinkLight(Light &light, const BBox &bounds) {
	m_draw->InvalidateInteractions(light);
	InternalUnlinkLight(light);

	light.m_leaf = LeafForPoint(light.m_pos);
	if (!light.m_leaf)
		return;

	AreaBits visible;
	
	if (light.m_leaf->area > -1) {
		visible.set(light.m_leaf->area);

		StackWindingStackVec bbox;
		BoundWindings(bounds, bbox);
		ClipOccupantVolume(&light.m_pos, &bbox, bounds, 0, light.m_leaf->area, -1, visible);
	}

	LinkLightParms constArgs(light, bounds, visible);

	light.m_bspLeafs.reserve(8);
	if (m_nodes.empty()) {
		LinkLight(constArgs, -1);
	} else {
		LinkLight(constArgs, 0);
	}

	m_draw->LinkLight(light, bounds);
}

void World::UnlinkLight(Light &light) {
	m_draw->UnlinkLight(light);
	InternalUnlinkLight(light);
}

void World::InternalUnlinkLight(Light &light) {
	
	for (dBSPLeaf::PtrVec::const_iterator it = light.m_bspLeafs.begin(); it != light.m_bspLeafs.end(); ++it) {
		dBSPLeaf *leaf = *it;
		leaf->lights.erase(&light);
	}

	for (IntSet::const_iterator it = light.m_areas.begin(); it != light.m_areas.end(); ++it) {
		dBSPArea &area = m_areas[*it];
		area.lights.erase(&light);
	}

	light.m_bspLeafs.clear();
	light.m_areas.clear();
	light.m_leaf = 0;
}

void World::LinkLight(const LinkLightParms &constArgs, int nodeNum) {
	if (nodeNum < 0) {
		nodeNum = -(nodeNum + 1);
		RAD_ASSERT(nodeNum < (int)m_leafs.size());
		dBSPLeaf &leaf = m_leafs[nodeNum];

		if ((leaf.area > -1) && constArgs.visible.test(leaf.area)) {
			leaf.lights.insert(&constArgs.light);
			constArgs.light.m_bspLeafs.push_back(&leaf);
			constArgs.light.m_areas.insert(leaf.area);
			dBSPArea &area = m_areas[leaf.area];
			area.lights.insert(&constArgs.light);
			m_draw->LinkLight(
				constArgs.light, 
				constArgs.bounds, 
				nodeNum,
				leaf,
				area
			);
		}

		return;
	}

	RAD_ASSERT(nodeNum < (int)m_nodes.size());
	const dBSPNode &node = m_nodes[nodeNum];
	RAD_ASSERT(node.planenum < (int)m_planes.size());
	const Plane &p = m_planes[node.planenum];

	Plane::SideType side = p.Side(constArgs.bounds, 0.0f);
	
	if ((side == Plane::Cross) || (side == Plane::Front))
		LinkLight(constArgs, node.children[0]);
	if ((side == Plane::Cross) || (side == Plane::Back))
		LinkLight(constArgs, node.children[1]);
}

bool World::ClipOccupantVolume(
	const Vec3 *pos,
	const StackWindingStackVec *volume,
	const BBox &volumeBounds,
	ClippedAreaVolumeStackVec *clipped,
	int fromArea,
	int toArea,
	AreaBits &visible,
	WorldDraw::Counters *counters
) {
	if (fromArea == toArea) {
		if (clipped && volume)
			(*clipped)->push_back(ClippedAreaVolume(fromArea, *volume, volumeBounds));
		return true;
	}

	visible.set(fromArea);

	AreaBits stack;
	ClipOccupantVolumeParms constArgs(pos, clipped, toArea, visible, stack, counters);
	return ClipOccupantVolume(constArgs, volume, volumeBounds, fromArea) || (toArea == -1);
}

bool World::ClipOccupantVolume(
	const ClipOccupantVolumeParms &constArgs,
	const StackWindingStackVec *volume,
	const BBox &volumeBounds,
	int fromArea
) {
	RAD_ASSERT(fromArea < (int)m_bsp->numAreas.get());
	RAD_ASSERT(constArgs.toArea < (int)m_bsp->numAreas.get());

	constArgs.stack.set(fromArea);

	const world::bsp_file::BSPArea *area = m_bsp->Areas() + fromArea;

	bool canSee = false;

	// follow all portals into adjecent areas.
	for (U32 i = 0; i < area->numPortals; ++i) {

		U32 areaportalNum = *(m_bsp->AreaportalIndices() + area->firstPortal + i);
		RAD_ASSERT(areaportalNum < m_areaportals.size());
		const dAreaportal &areaportal = m_areaportals[areaportalNum];

		if (!areaportal.open)
			continue;

		int side = areaportal.areas[1] == fromArea;
		RAD_ASSERT(areaportal.areas[side] == fromArea);
		int otherArea = areaportal.areas[side ^ 1];
		int planenum = areaportal.planenum ^ side ^ 1; // put fromArea on back (we want to clip away volume in our area).
		const Plane &portalPlane = m_planes[planenum];

		if (constArgs.stack.test(otherArea))
			continue; // came from here.

		if (constArgs.counters)
			++constArgs.counters->testedPortals;

		if (!constArgs.pos || !volume) {
			
			// trivial rejection only.
			if (!volumeBounds.Touches(areaportal.bounds))
				continue;

			if (portalPlane.Side(volumeBounds, 0.f) != Plane::Cross)
				continue;

			constArgs.visible.set(otherArea);

			canSee = otherArea == constArgs.toArea;

			if (constArgs.counters)
				++constArgs.counters->drawnPortals;

			if (!canSee)
				canSee = ClipOccupantVolume(constArgs, volume, volumeBounds, otherArea);
		} else {

			float planeDist = portalPlane.Distance(*constArgs.pos);
			if (planeDist > 1.f)
				continue; // on wrong side of portal.

			// quick reject, bounds touches this portal?
			if (!volumeBounds.Touches(areaportal.bounds))
				continue;
			
			Plane::SideType s = portalPlane.Side(volumeBounds, 1.f);

			if (s == Plane::Back)
				continue;

			StackWinding winding(
				&areaportal.winding.Vertices()[0],
				areaportal.winding.NumVertices(),
				areaportal.winding.Plane()
			);

			if (planeDist < -4.f) { // avoid clipping away this portal due to numerical issues
				if (!ChopWindingToVolume(*volume, winding))
					continue; // portal clipped away.
			
				// Generate planes that bound the portal.
				PlaneVec boundingPlanes;
				MakeBoundingPlanes(*constArgs.pos, winding, boundingPlanes, side == 0);

				// Constrain volume to portal bounding planes.
				ClippedAreaVolume areaVolume(otherArea, *volume, volumeBounds);
				BBox areaVolumeBounds(volumeBounds);

				if (!ChopVolume(areaVolume.volume, areaVolumeBounds, boundingPlanes))
					continue; // volume clipped away.

				// Bound by portal.
		
				if (!ChopVolume(areaVolume.volume, areaVolumeBounds, portalPlane))
					continue;

				constArgs.visible.set(otherArea);

				// NOTE: this push_back involves a large amount of memory copying due to the use
				// of these being stackify<> vectors. 
				if (constArgs.clipped) 
					(*constArgs.clipped)->push_back(areaVolume);

				if (constArgs.counters)
					++constArgs.counters->drawnPortals;

				canSee = otherArea == constArgs.toArea;

				if (!canSee)
					canSee = ClipOccupantVolume(constArgs, &areaVolume.volume, areaVolumeBounds, otherArea);
			} else { // to close to portal to generate restricting planes.
				constArgs.visible.set(otherArea);

				ClippedAreaVolume areaVolume(otherArea, *volume, volumeBounds);

				// NOTE: this push_back involves a large amount of memory copying due to the use
				// of these being stackify<> vectors. 
				if (constArgs.clipped) 
					(*constArgs.clipped)->push_back(areaVolume);

				if (constArgs.counters)
					++constArgs.counters->drawnPortals;

				canSee = otherArea == constArgs.toArea;

				if (!canSee)
					canSee = ClipOccupantVolume(constArgs, volume, volumeBounds, otherArea);
			}
		}

		if (canSee)
			break;
	}

	constArgs.stack.reset(fromArea);
	return canSee;
}

dBSPLeaf *World::LeafForPoint(const Vec3 &pos) {
	if (m_nodes.empty())
		return &m_leafs[0];
	return LeafForPoint(pos, 0);
}

dBSPLeaf *World::LeafForPoint(const Vec3 &pos, int nodeNum) {
	if (nodeNum < 0) {
		nodeNum = -(nodeNum + 1);
		RAD_ASSERT(nodeNum < (int)m_leafs.size());
		dBSPLeaf &leaf = m_leafs[nodeNum];
		return &leaf;
	}

	RAD_ASSERT(nodeNum < (int)m_nodes.size());
	const dBSPNode &node = m_nodes[nodeNum];
	RAD_ASSERT(node.planenum < (int)m_planes.size());
	const Plane &p = m_planes[node.planenum];

	Plane::SideType side = p.Side(pos, 0.f);
	if (side == Plane::Back)
		return LeafForPoint(pos, node.children[1]);
	return LeafForPoint(pos, node.children[0]);
}

void World::BoundWindings(const BBox &bounds, StackWindingStackVec &windings) {
	
	// make bounding windings that face inward

	Plane planes[6];
	int planeNum = 0;

	for (int i = 0; i < 2; ++i) {
		for (int k = 0; k < 3; ++k) {

			Vec3 normal(Vec3::Zero);
			normal[k] = (i==0) ? 1.f : -1.f;
			float dist = (i==0) ? (bounds.Mins()[k]) : (-bounds.Maxs()[k]);

			planes[planeNum] = Plane(normal, dist);
			++planeNum;
		}
	}

	BBox unused;
	MakeVolume(planes, 6, windings, unused);
}

bool World::ChopWindingToVolume(const StackWindingStackVec &volume, StackWinding &out) {

	StackWinding f;

	for (StackWindingStackVec::const_iterator it = volume->begin(); it != volume->end(); ++it) {
		const StackWinding &w = *it;

		f.Clear();
		out.Chop(w.Plane(), Plane::Front, f, 1.f);
		out = f;

		if (out.Empty())
			break;
	}

	return !out.Empty();
}

bool World::ChopVolume(StackWindingStackVec &volume, BBox &bounds, const Plane &p) {

	Plane::SideType side = p.Side(bounds, 1.f);
	if (side == Plane::Back)
		return false;
	if ((side == Plane::Front) || (side == Plane::On))
		return true;

	StackWindingStackVec src(volume);
	volume->clear();
	bounds.Initialize();

	StackWinding f;
	for (StackWindingStackVec::const_iterator it = src->begin(); it != src->end(); ++it) {
		const StackWinding &w = *it;

		f.Clear();
		w.Chop(p, Plane::Front, f, 1.f);
		if (!f.Empty()) {
			for (int i = 0; i < f.NumVertices(); ++i)
				bounds.Insert(f.Vertices()[i]);
			volume->push_back(f);
		}
	}

	StackWinding pw(p, 16384);

	for (StackWindingStackVec::const_iterator it = volume->begin(); it != volume->end(); ++it) {
		const StackWinding &w = *it;

		f.Clear();
		pw.Chop(w.Plane(), Plane::Front, f, 1.f);
		pw = f;
		if (pw.Empty())
			break;
	}

	if (!pw.Empty()) {
		for (int i = 0; i < pw.NumVertices(); ++i)
			bounds.Insert(pw.Vertices()[i]);
		volume->push_back(pw);
	}

	return !volume->empty();
}

bool World::ChopVolume(StackWindingStackVec &volume, BBox &bounds, const PlaneVec &planes) {

	for (PlaneVec::const_iterator it = planes.begin(); it != planes.end(); ++it) {
		const Plane &p = *it;
		if (!ChopVolume(volume, bounds, p))
			return false;
	}

	return !volume->empty();
}

bool World::IntersectVolumes(const StackWindingStackVec &a, StackWindingStackVec &out, BBox &bounds) {

	for (StackWindingStackVec::const_iterator it = a->begin(); it != a->end(); ++it) {
		const StackWinding &w = *it;
		if (!ChopVolume(out, bounds, w.Plane()))
			return false;
	}

	return !out->empty();
}

void World::MakeVolume(const Plane *planes, int num, StackWindingStackVec &volume, BBox &bounds) {
	volume->clear();
	volume->reserve(num);
	bounds.Initialize();

	StackWinding f;

	for (int i = 0; i < num; ++i) {
		StackWinding w(planes[i], 16384);
		for (int k = 0; k < num; ++k) {
			if (k == i)
				continue;
			
			f.Clear();
			w.Chop(planes[k], Plane::Front, f, 1.f);
			w = f;
			if (w.Empty())
				break;
		}

		if (!w.Empty()) {
			for (int k = 0; k < w.NumVertices(); ++k)
				bounds.Insert(w.Vertices()[k]);
			volume->push_back(w);
		}
	}
}

void World::MakeBoundingPlanes(const Vec3 &pos, const StackWinding &portal, PlaneVec &planes, bool side) {

	RAD_ASSERT(portal.NumVertices() >= 3);

	planes.clear();
	planes.reserve(portal.NumVertices());

	for (int i = 0; i < portal.NumVertices(); ++i) {
		int k = i+1;
		if (k >= portal.NumVertices())
			k = 0;
		
		const Vec3 &a = portal.Vertices()[i];
		const Vec3 &b = portal.Vertices()[k];
		
		Plane p(a, b, pos);

		if (side)
			p = -p;

		planes.push_back(p);
	}
}

bool World::LineTrace(Trace &trace) {

	const dBSPLeaf *leaf = LeafForPoint(trace.start);
	if (!leaf) {
		trace.startSolid = true;
		trace.contents = bsp_file::kContentsFlag_Solid;
		trace.traceEnd = trace.start;
		trace.frac = 0.f;
		return true;
	}

	int solidTrace = trace.contents & ~bsp_file::kContentsFlag_Clip;

	if (leaf->contents & solidTrace) {
		trace.contents = leaf->contents;
		trace.startSolid = true;
		trace.traceEnd = trace.start;
		trace.frac = 0.f;
		return true;
	}

	trace.startSolid = false;
	trace.traceEnd = trace.start;
	trace.frac = 1.f;

	bool r;
	if (m_nodes.empty()) {
		r = LineTrace(trace, trace.start, trace.end, -1);
	} else {
		r = LineTrace(trace, trace.start, trace.end, 0);
	}

	if (r) {
		float l = (trace.end - trace.start).MagnitudeSquared();
		float d = (trace.traceEnd - trace.start).MagnitudeSquared();
		trace.frac = (l != 0.f) ? (d / l) : 1.f;
	}

	return r;
}

bool World::LineTrace(Trace &trace, const Vec3 &a, const Vec3 &b, int nodeNum) {
	if (nodeNum < 0) {
		int leafNum = -(nodeNum + 1);
		const dBSPLeaf &leaf = m_leafs[leafNum];

		// it's not gonna hit this leaf.
		if (!(leaf.contents&trace.contents))
			return false;
		
		if (trace.contents&bsp_file::kContentsFlag_Clip) {
			if (leaf.numClipModels < 1)
				return false;

			float bestDistance = std::numeric_limits<float>::max();
			const bsp_file::BSPClipSurface *surface = 0;
			Vec3 intersection;

			for (int i = 0; i < leaf.numClipModels; ++i) {
				RayIntersectsClipModel(
					i+leaf.firstClipModel,
					a,
					b,
					bestDistance,
					intersection,
					surface
				);
			}

			if (!surface)
				return false;

			trace.contents = surface->contents;
			trace.normal = m_planes[surface->planenum].Normal();
			trace.traceEnd = intersection;
			return true;
		}

		return false;
	}

	// trace from result->end
	const dBSPNode &node = m_nodes[nodeNum];
	const Plane &plane = m_planes[node.planenum];

	Plane::SideType sides[2];
	sides[0] = plane.Side(a);
	sides[1] = plane.Side(b);

	if (sides[0] == Plane::On)
		sides[0] = sides[1];
	if (sides[1] == Plane::On)
		sides[1] = sides[0];

	if (sides[0] == sides[1]) {
		if (sides[0] == Plane::On) {
			sides[0] = Plane::Front;
			sides[1] = Plane::Front;
		}
		return LineTrace(trace, a, b, node.children[sides[0] == Plane::Back]);
	}

	// cross
	Vec3 mid;
	if (plane.IntersectLineSegment(mid, a, b)) {
		if (LineTrace(trace, a, mid, node.children[sides[0] == Plane::Back]))
			return true;
		if (LineTrace(trace, mid, b, node.children[sides[1] == Plane::Back]))
			return true;
	}

	return false;
}

Entity::Ref World::FirstEntityTouchingBrush(int classbits, int brushNum, const Vec3 &xform) const {
	RAD_ASSERT(brushNum >= 0 && brushNum < (int)m_bsp->numBrushes);
	const bsp_file::BSPBrush *brush = m_bsp->Brushes() + brushNum;

	const BBox kBrushBBox(
		Vec3(brush->mins[0], brush->mins[1], brush->mins[2]) + xform,
		Vec3(brush->maxs[0], brush->maxs[1], brush->maxs[2]) + xform
	);

	Entity::Ref touching = FirstBBoxTouching(kBrushBBox, classbits);
	
	if (touching) {
		if (brush->numPlanes != 6) {
			// non-axial brush hull, do planes test
			BBox bbox(touching->ps->bbox);
			bbox.Translate(touching->ps->worldPos);
			bbox.Translate(-xform);

			if (!IsBBoxInsideBrushHull(bbox, *brush))
				touching.reset();
		}
	}

	return touching;
}

Entity::Vec World::EntitiesTouchingBrush(int classbits, int brushNum, const Vec3 &xform) const {
	RAD_ASSERT(brushNum >= 0 && brushNum < (int)m_bsp->numBrushes);
	const bsp_file::BSPBrush *brush = m_bsp->Brushes() + brushNum;

	const BBox kBrushBBox(
		Vec3(brush->mins[0], brush->mins[1], brush->mins[2]) + xform,
		Vec3(brush->maxs[0], brush->maxs[1], brush->maxs[2]) + xform
	);

	Entity::Vec bboxTouching = BBoxTouching(kBrushBBox, classbits);

	if (brush->numPlanes == 6) {
		// pure axial brush no planes test necessary
		return bboxTouching;
	}

	Entity::Vec touching;
	touching.reserve(bboxTouching.size());

	for (Entity::Vec::const_iterator it = bboxTouching.begin(); it != bboxTouching.end(); ++it) {
		const Entity::Ref &entity = *it;

		BBox bbox(entity->ps->bbox);
		bbox.Translate(entity->ps->worldPos);
		bbox.Translate(-xform);

		if (IsBBoxInsideBrushHull(bbox, *brush))
			touching.push_back(entity);
	}

	return touching;
}

bool World::EntityTouchesBrush(const Entity &entity, int brushNum, const Vec3 &xform) const {
	RAD_ASSERT(brushNum >= 0 && brushNum < (int)m_bsp->numBrushes);
	const bsp_file::BSPBrush *brush = m_bsp->Brushes() + brushNum;

	const BBox kBrushBBox(
		Vec3(brush->mins[0], brush->mins[1], brush->mins[2]),
		Vec3(brush->maxs[0], brush->maxs[1], brush->maxs[2])
	);

	BBox bbox(entity.ps->bbox);
	bbox.Translate(entity.ps->worldPos);
	bbox.Translate(-xform);

	if (!kBrushBBox.Instersects(bbox))
		return false;

	if (brush->numPlanes == 6)
		return true; // pure axial brush, bbox test was enough

	return IsBBoxInsideBrushHull(bbox, *brush);
}

bool World::IsBBoxInsideBrushHull(const BBox &bbox, int brushNum) const {
	RAD_ASSERT(brushNum >= 0 && brushNum < (int)m_bsp->numBrushes);
	const bsp_file::BSPBrush *brush = m_bsp->Brushes() + brushNum;
	return IsBBoxInsideBrushHull(bbox, *brush);
}

bool World::IsBBoxInsideBrushHull(const BBox &bbox, const bsp_file::BSPBrush &brush) const {

	for (U32 i = 0; i < brush.numPlanes; ++i) {
		const bsp_file::BSPPlane *bspPlane = m_bsp->Planes() + brush.firstPlane + i;
		const Plane kPlane(bspPlane->p[0], bspPlane->p[1], bspPlane->p[2], bspPlane->p[3]);

		Vec3 p;

		// find bbox corner closest to or behind plane.
		for (int k = 0; k < 3; ++k) {
			if (kPlane.Normal()[k] < 0) {
				p[k] = bbox.Maxs()[k];
			} else {
				p[k] = bbox.Mins()[k];
			}
		}

		float d = kPlane.Normal().Dot(p);
		float dd = kPlane.Distance(bbox.Origin()) + d;

		if (dd >= 0.f) // on front of plane
			return false;
	}

	return true;
}

bool World::RayIntersectsClipModel(
	int modelNum, 
	const Vec3 &a, 
	const Vec3 &b, 
	float &bestDistance,
	Vec3 &intersection,
	const bsp_file::BSPClipSurface *& outSurf
) {
	const bsp_file::BSPClipModel *model = m_bsp->ClipModels() + modelNum;

	BBox bounds(
		model->mins[0], model->mins[1], model->mins[2],
		model->maxs[0], model->maxs[1], model->maxs[2]
	);

	if (!RayIntersectsBBox(a, b, bounds))
		return false;

	bool intersects = false;

	for (U32 i = 0; i < model->numClipSurfaces; ++i) {
		const bsp_file::BSPClipSurface *surf = m_bsp->ClipSurfaces() + model->firstClipSurface + i;
		const Plane &plane = m_planes[surf->planenum];

		Vec3 mid;
		if (plane.IntersectLineSegment(mid, a, b, 0.f)) {
			Vec3 dd = (mid - a);
			float len = dd.Normalize();
			if (len < bestDistance) {
				// inside polygon?
				U32 k;
				for (k = 0; k < surf->numEdgePlanes; ++k) {
					const bsp_file::BSPPlane *pplane = m_bsp->ClipEdgePlanes() + surf->firstEdgePlane + k;
					const Plane kPlane(pplane->p[0], pplane->p[1], pplane->p[2], pplane->p[3]);
					if (kPlane.Side(mid, 0.f) != Plane::Front)
						break;
				}

				if (k == surf->numEdgePlanes) {
					// inside hull
					outSurf = surf;
					bestDistance = len;
					intersection = mid;
					intersects = true;
				}
			}
		}
	}

	return intersects;
}

} // world
