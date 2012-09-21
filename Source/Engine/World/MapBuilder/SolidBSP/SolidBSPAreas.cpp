/*! \file SolidBSPSectors.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup map_builder
*/

#include RADPCH
#include "SolidBsp.h"

namespace tools {
namespace solid_bsp {

namespace {
	const ValueType kSplitAxisBounds[3] = { 1024.0, 1024.0, 1024.0 };
	enum {
		kSmallTriBatch = 8*Kilo // any leaf holding less than this is a candidate for merging.
	};
}

bool BSPBuilder::CompileAreas() {
	ResetProgress();
	m_numOutsideTris = 0;
	m_numOutsideModels = 0;
	m_numInsideTris = 0;
	m_numInsideModels = 0;
	m_work = 0;

	Log("\n------------\n");
	Log("Compiling Objects...\n");

	if (m_ui) {
		m_ui->title = "Compiling Models...";
		m_ui->total = (int)m_map->worldspawn->models.size();
		m_ui->totalProgress = 0;
		m_ui->Refresh();
	}

	for (SceneFile::TriModel::Vec::const_iterator it = m_map->worldspawn->models.begin(); it != m_map->worldspawn->models.end(); ++it) {
		const SceneFile::TriModel::Ref &m = *it;

		if (m->ignore || m->cinematic)
			continue;
		DecomposeAreaModel(*m);
		if (m_ui) {
			m_ui->Step();
			m_ui->Refresh();
		}
	}

	Log("\n------------\n");
	Log("Inside : %d model(s), %d tri(s)\n", m_numInsideModels, m_numInsideTris);
	Log("Outside: %d model(s), %d tri(s)\n", m_numOutsideModels, m_numOutsideTris);
	Log("Total  : %d model(s), %d tri(s)\n", m_numInsideModels+m_numOutsideModels, m_numInsideTris+m_numOutsideTris);

	Log("\n------------\n");

	if (m_ui) {
		m_ui->title = "Compiling Areas...";
		m_ui->total = (int)m_areas.size();
		m_ui->totalProgress = 0;
		m_ui->Refresh();
	}

	for (AreaVec::iterator it = m_areas.begin(); it != m_areas.end(); ++it) {
		AreaRef area = *it;
		ResetProgress();
		m_work = 0;
		Log("Compiling Area %d...\n", area->area);
		if (!BuildAreaTree(*area))
			return false;
		if (m_ui) {
			m_ui->Step();
			m_ui->Refresh();
		}
	}

	Log("\n------------\n");
	for (AreaVec::iterator it = m_areas.begin(); it != m_areas.end(); ++it) {
		AreaRef area = *it;
		Log("Area %d: %d node(s), %d leaf(s), %d/%d tri(s), bounds: (%dx%dx%d)\n", 
			area->area, area->numNodes, area->numLeafs, area->numTris[0], area->numTris[1], 
			(int)area->bounds.Size()[0], (int)area->bounds.Size()[1], (int)area->bounds.Size()[2]);
	}

	return true;
}

bool BSPBuilder::BuildAreaTree(Area &area) {
	MakeAreaRootNode(area);

	if (area.root->tris.empty()) {
		area.bounds.SetMins(Vec3::Zero);
		area.bounds.SetMaxs(Vec3::Zero);
		area.root.reset();
		return true;
	}

	for (int i = 0; i < 3; ++i) {
		if (area.bounds.Size()[i] > SceneFileD::kMaxRange) {
			Log("ERROR: Area %d is HUGE! Its bounds (%dx%dx%d) exceeds the limit of %dx%dx%d!\n", 
				(int)area.bounds.Size()[0], 
				(int)area.bounds.Size()[1], 
				(int)area.bounds.Size()[2], 
				(int)SceneFileD::kMaxRange,
				(int)SceneFileD::kMaxRange,
				(int)SceneFileD::kMaxRange
			);

			SetResult(pkg::SR_CompilerError);
			return false;
		}
	}

	AreaNodePolyVec tris(area.root->tris);

	AreaBoxBSP(area, area.root, 0);

	PartitionAreaTris(area, area.root, false);
	OptimizeAreaTree(area, area.root);

	area.root->tris.swap(tris);
	PartitionAreaTris(area, area.root, true);
	return true;
}

void BSPBuilder::MakeAreaRootNode(Area &area) {
	area.bounds.Initialize();
	area.numNodes = 0;
	area.numLeafs = 0;
	
	++m_numAreaNodes;
	AreaNodeRef root(new (world::bsp_file::ZBSPBuilder) AreaNode());

	if (area.area == 0) {
		// shared area only contains triangles that span areas.

		for (SceneFile::TriModel::Vec::const_iterator it = m_map->worldspawn->models.begin(); it != m_map->worldspawn->models.end(); ++it) {
			const SceneFile::TriModel::Ref &m = *it;

			if (m->ignore || m->outside || m->cinematic)
				continue;
			if (m->contents & kContentsFlag_Areaportal)
				continue;

			for (SceneFile::TriFaceVec::iterator it = m->tris.begin(); it != m->tris.end(); ++it) {
				
				SceneFile::TriFace &tri = *it;
				if (tri.areas.size() < 2)
					continue;
				if (tri.outside)
					continue;
				RAD_ASSERT(!(tri.surface & kSurfaceFlag_NoDraw));
				
				area.bounds.Insert(ToBSPType(tri.model->verts[tri.v[0]].pos));
				area.bounds.Insert(ToBSPType(tri.model->verts[tri.v[1]].pos));
				area.bounds.Insert(ToBSPType(tri.model->verts[tri.v[2]].pos));

				AreaNodePolyRef poly(new (world::bsp_file::ZBSPBuilder) AreaNodePoly());
				poly->tri = &tri;
				poly->plane = ToBSPType(tri.plane);
				poly->winding.Initialize(
					AreaNodeWinding::VertexType(ToBSPType(tri.model->verts[tri.v[0]])), 
					AreaNodeWinding::VertexType(ToBSPType(tri.model->verts[tri.v[1]])), 
					AreaNodeWinding::VertexType(ToBSPType(tri.model->verts[tri.v[2]])),
					ToBSPType(tri.plane)
				);
				root->tris.push_back(poly);
				++area.numTris[0];
			}
		}
	} else {
		for (TriFacePtrVec::iterator it = area.tris.begin(); it != area.tris.end(); ++it) {
			SceneFile::TriFace *tri = *it;
			RAD_ASSERT(!(tri->contents & kContentsFlag_Areaportal));
			RAD_ASSERT(!(tri->surface & kSurfaceFlag_NoDraw));

			if (tri->areas.size() == 1) { // only use tris that aren't shared by any other areas.
				area.bounds.Insert(ToBSPType(tri->model->verts[tri->v[0]].pos));
				area.bounds.Insert(ToBSPType(tri->model->verts[tri->v[1]].pos));
				area.bounds.Insert(ToBSPType(tri->model->verts[tri->v[2]].pos));

				AreaNodePolyRef poly(new (world::bsp_file::ZBSPBuilder) AreaNodePoly());
				poly->tri = tri;
				poly->plane = ToBSPType(tri->plane);
				poly->winding.Initialize(
					AreaNodeWinding::VertexType(ToBSPType(tri->model->verts[tri->v[0]])), 
					AreaNodeWinding::VertexType(ToBSPType(tri->model->verts[tri->v[1]])), 
					AreaNodeWinding::VertexType(ToBSPType(tri->model->verts[tri->v[2]])),
					ToBSPType(tri->plane)
				);
				root->tris.push_back(poly);
				++area.numTris[0];
			}
		}
	}

	root->bounds = area.bounds;
	area.root = root;
}

void BSPBuilder::AreaBoxBSP(Area &area, const AreaNodeRef &node, int planebits) {
	Vec3 size = node->bounds.Size();

	int axis;

	// find best axial split...
	for (axis = 2; axis >= 0; --axis) {
		if (planebits & (1<<axis))
			continue;

		if (size[axis] > kSplitAxisBounds[axis]+32.f)
			break;

		planebits |= (1<<axis);
	}

	if (axis < 0) {
		for (axis = 2; axis >= 0; --axis)
		{
			if (size[axis] > kSplitAxisBounds[axis]+32.f)
				break;
		}
	} else {
		planebits |= (1<<axis);
	}

	if (planebits == 7)
		planebits = 0; // completed rotation.

	if (axis < 0) { // leaf node
		node->planenum = kPlaneNumLeaf;
		++m_numAreaLeafs;
		++area.numLeafs;
		return;
	}

	Vec3 normal(Vec3::Zero);
	normal[axis] = 1.f;
	Plane pl(normal, node->bounds.Mins()[axis] + (size[axis]*0.5f));

	node->planenum = m_planes.FindPlaneNum(pl);

	AreaNodeRef front(new (world::bsp_file::ZBSPBuilder) AreaNode());
	AreaNodeRef back(new (world::bsp_file::ZBSPBuilder) AreaNode());

	front->parent = node.get();
	back->parent = node.get();

	RAD_DEBUG_ONLY(int bits = )SplitBounds(axis, pl.D(), node->bounds, front->bounds, back->bounds);
	RAD_ASSERT(bits == 3); // should have split!

	node->children[0] = front;
	node->children[1] = back;

	++m_numAreaNodes;
	++area.numNodes;

	AreaBoxBSP(area, front, planebits);
	AreaBoxBSP(area, back, planebits);
}

void BSPBuilder::OptimizeAreaTree(Area &area, AreaNodeRef &node) {
	if (node->planenum == kPlaneNumLeaf)
		return;

	OptimizeAreaTree(area, node->children[0]);
	OptimizeAreaTree(area, node->children[1]);

	// can only optimize nodes spanning a leaf on at least one side.
	if (node->children[0]->planenum != kPlaneNumLeaf &&
		node->children[1]->planenum != kPlaneNumLeaf) {
		return;
	}

	// tris on front side, can merge if other side is leaf
	if (!node->children[0]->tris.empty() && node->children[1]->planenum != kPlaneNumLeaf)
		return;

	// tris on back side, can merge if other side is  leaf
	if (!node->children[1]->tris.empty() && node->children[0]->planenum != kPlaneNumLeaf)
		return;

	if (node->children[0]->planenum == kPlaneNumLeaf &&
		node->children[1]->planenum == kPlaneNumLeaf) { // merge empty leaf

		// candidates for merging based on tri count?
		if ((node->children[0]->tris.size() > kSmallTriBatch) && (node->children[1]->tris.size() > kSmallTriBatch))
			return;
		if ((node->children[0]->tris.size() > kSmallTriBatch*2) && !node->children[1]->tris.empty())
			return;
		if ((node->children[1]->tris.size() > kSmallTriBatch*2) && !node->children[0]->tris.empty())
			return;

		node->bounds.Initialize();

		// merge
		if (node->children[0]->tris.empty() || node->children[1]->tris.empty()) {
			int s = node->children[0]->tris.empty() == true;
			node->tris.swap(node->children[s]->tris);
		} else {
			node->tris.swap(node->children[0]->tris);
			node->tris.reserve(node->tris.size() + node->children[1]->tris.size());
			std::copy(node->children[1]->tris.begin(), node->children[1]->tris.end(), std::back_inserter(node->tris));
		}

		// preserve bounds.

		node->bounds.Insert(node->children[0]->bounds);
		node->bounds.Insert(node->children[1]->bounds);

		// empty nodes, merge.

		node->planenum = kPlaneNumLeaf;

		node->children[0].reset();
		node->children[1].reset();

		--m_numAreaLeafs;
		--m_numAreaNodes;
		--area.numLeafs;
		--area.numNodes;
	} else {
		// remove seperating plane.

		int s = node->children[0]->planenum == kPlaneNumLeaf;
		RAD_ASSERT(node->children[s]->planenum != kPlaneNumLeaf);

		AreaNode* parent = node->parent;

		node = node->children[s];
		node->parent = parent;

		--m_numAreaNodes;
		--area.numNodes;
	}
}

int BSPBuilder::SplitBounds(int axis, float distance, const BBox &bounds, BBox &front, BBox &back) {
	// range?
	if (bounds.Mins()[axis] >= distance) {
		front = bounds;
		return 1;
	}

	if (bounds.Maxs()[axis] <= distance) {
		back = bounds;
		return 2;
	}

	Vec3 v = bounds.Mins();
	v[axis] = distance;
	front.SetMins(v);
	front.SetMaxs(bounds.Maxs());

	v = bounds.Maxs();
	v[axis] = distance;
	back.SetMins(bounds.Mins());
	back.SetMaxs(v);

	return 3;
}

void BSPBuilder::PartitionAreaTris(Area &area, const AreaNodeRef &node, bool split) {

	if (node->planenum == kPlaneNumLeaf) {
		if (split)
			area.numTris[1] += (int)node->tris.size();
		return;
	}

	const Plane &p = m_planes.Plane(node->planenum);

	AreaNodePolyVec front;
	AreaNodePolyVec back;

	while (!node->tris.empty()) {
		AreaNodePolyRef tri = node->tris.back();
		node->tris.pop_back();

		Plane::SideType s = tri->winding.Side(p, kAreaPartitionEpsilon);

		if (s == Plane::On) {
			ValueType d = tri->plane.Normal().Dot(p.Normal());
			if (d > ValueType(0)) {
				s = Plane::Back;
			} else {
				s = Plane::Front;
			}
		}

		if (s == Plane::Front) {
			front.push_back(tri);
			continue;
		} else if (s == Plane::Back) {
			back.push_back(tri);
			continue;
		}

		RAD_ASSERT(s == Plane::Cross);

		if (split) {
			AreaNodePolyRef f(new (world::bsp_file::ZBSPBuilder) AreaNodePoly(*tri));
			AreaNodePolyRef b(new (world::bsp_file::ZBSPBuilder) AreaNodePoly(*tri));

			tri->winding.Split(p, &f->winding, &b->winding, kAreaPartitionEpsilon);

			if (!f->winding.Empty())
				front.push_back(f);
			if (!b->winding.Empty())
				back.push_back(b);
		} else {
			s = tri->winding.MajorSide(p, kAreaPartitionEpsilon);
			if (s == Plane::Front) {
				front.push_back(tri);
			} else if (s == Plane::Back) {
				back.push_back(tri);
			} else {
				SOLID_BSP_ICE();
			}
		}
	}

	node->children[0]->tris = front;
	node->children[1]->tris = back;

	PartitionAreaTris(area, node->children[0], split);
	PartitionAreaTris(area, node->children[1], split);
}

void BSPBuilder::DecomposeAreaModel(SceneFile::TriModel &model) {
	if (model.contents&kContentsFlag_Areaportal)
		return;
//	if (model.contents & Map::ContentsNoDraw) return;

	// push each triangle into the bsp, and assign areas.
	bool solid = (model.contents&kContentsFlag_Solid) ? true : false;

	// skip
	if (m_flood && solid && model.outside) {
		++m_numOutsideModels;
		m_numOutsideTris += (int)model.tris.size();
		return;
	}

	bool foundArea = false;

	for (SceneFile::TriFaceVec::const_iterator it = model.tris.begin(); it != model.tris.end(); ++it) {
		if (++m_work % 1000 == 0)
			EmitProgress();

		const SceneFile::TriFace &tri = *it;
		if (tri.outside)
			continue;
		if (tri.surface & kSurfaceFlag_NoDraw)
			continue;

		AreaPoly *poly = new (world::bsp_file::ZBSPBuilder) AreaPoly();
		poly->plane = ToBSPType(tri.plane);
		poly->winding.Initialize(
			ToBSPType(model.verts[tri.v[0]].pos), 
			ToBSPType(model.verts[tri.v[1]].pos), 
			ToBSPType(model.verts[tri.v[2]].pos),
			ToBSPType(tri.plane));
		poly->tri = const_cast<SceneFile::TriFace*>(&tri);
		DecomposeAreaPoly(m_root.get(), poly);

		//RAD_ASSERT(!tri.areas.empty());

		if (tri.areas.empty()) {
			++m_numOutsideTris;
		} else {
			foundArea = true;
			++m_numInsideTris;
		}
	}

	if (!foundArea) {
		model.outside = true;
		Log("\nWARNING: '%s' has no visible surfaces inside hull.\n", model.name.c_str.get());
	}

	if (model.outside) {
		++m_numOutsideModels;
	} else {
		++m_numInsideModels;
	}
}

void BSPBuilder::DecomposeAreaPoly(Node *node, AreaPoly *poly) {
	if (node->planenum == kPlaneNumLeaf) {
		if ((!m_flood || node->occupied) && node->area) {
			zone_vector<int, tools::Z3DXT>::type::iterator it;
			for (it = poly->tri->areas.begin(); it != poly->tri->areas.end(); ++it) {
				if (*it == node->area->area) 
					break;
			}

			if (it == poly->tri->areas.end()) {
				poly->tri->areas.push_back(node->area->area);
				node->area->tris.push_back(poly->tri);
			}

			poly->tri->model->outside = false;
		}
		delete poly;
		return;
	}

	Plane::SideType s = poly->winding.Side(m_planes.Plane(node->planenum), ValueType(0));
	
	if (s == Plane::On) {
		float d = poly->plane.Normal().Dot(m_planes.Plane(node->planenum).Normal());
		if (d > ValueType(0)) {
			s = Plane::Back;
		} else {
			s = Plane::Front;
		}
	}

	if (s == Plane::Front) {
		DecomposeAreaPoly(node->children[0].get(), poly);
		return;
	}

	if (s == Plane::Back) {
		DecomposeAreaPoly(node->children[1].get(), poly);
		return;
	}

	AreaPoly *front = 0;
	AreaPoly *back  = 0;

	{
		Winding f, b;
		poly->winding.Split(m_planes.Plane(node->planenum), &f, &b, ValueType(0));

		if (f.Empty() && b.Empty()) {
			Log("WARNING: DecomposeAreaPoly triangle clipped away.\n");
		}
		
		if (!f.Empty()) {
			front = new AreaPoly(*poly);
			front->winding = f;
		}
		if (!b.Empty()) {
			back = new AreaPoly(*poly);
			back->winding = b;
		}

		delete poly;
	}

	if (front)
		DecomposeAreaPoly(node->children[0].get(), front);
	if (back)
		DecomposeAreaPoly(node->children[1].get(), back);
}

} // solid_bsp
} // tools
