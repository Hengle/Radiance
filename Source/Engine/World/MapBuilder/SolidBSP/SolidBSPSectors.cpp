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

void BSPBuilder::BuildSectors() {
	ResetProgress();
	Log("------------\n");
	Log("Sectorizing...\n");
	m_numOutsideTris = 0;
	m_numOutsideModels = 0;
	m_numInsideTris = 0;
	m_numInsideModels = 0;
	m_work = 0;

	for (SceneFile::TriModel::Vec::const_iterator it = m_map->worldspawn->models.begin(); it != m_map->worldspawn->models.end(); ++it) {
		if ((*it)->ignore)
			continue;
		DecomposeAreaModel(*(*it).get());
	}

	Log("------------\n");
	Log("Inside : %d model(s), %d tri(s)\n", m_numInsideModels, m_numInsideTris);
	Log("Outside: %d model(s), %d tri(s)\n", m_numOutsideModels, m_numOutsideTris);
	Log("Total  : %d model(s), %d tri(s)\n", m_numInsideModels+m_numOutsideModels, m_numInsideTris+m_numOutsideTris);

	m_work=m_numInsideTris=m_numSectors=m_numSharedSectors=0;

	BuildAreaSectors();
	BuildSharedSectors();

	Log("------------\n");
	Log("\nSectors (area/shared/total): %d/%d/%d\n", m_numSectors, m_numSharedSectors, m_numSectors+m_numSharedSectors);
	for (AreaVec::iterator it = m_areas.begin(); it != m_areas.end(); ++it) {
		AreaRef area = *it;
		Log("Area (%d): %d sector(s), %d tri(s)\n", area->area, area->sectors.size(), area->tris.size());
	}
	Log("------------\n");
	Log("%d Tri(s) in %d Area(s)\n", m_numInsideTris, (int)m_areas.size());
}

void BSPBuilder::BuildAreaSectors() {
	for (AreaVec::iterator it = m_areas.begin(); it != m_areas.end(); ++it) {
		const AreaRef &area = *it;
		BuildAreaSectors(*(area.get()));
	}
}

void BSPBuilder::BuildAreaSectors(Area &area) {
	area.bounds.Initialize();
	Sector *root = new Sector();
	root->areas.push_back(area.area);

	for (TriFacePtrVec::iterator it = area.tris.begin(); it != area.tris.end(); ++it) {
		SceneFile::TriFace *tri = *it;
		RAD_ASSERT(!(tri->contents & kContentsFlag_Areaportal));

		if (tri->surface & kSurfaceFlag_NoDraw)
			continue;

		if (tri->areas.size() == 1) { // only use tris that aren't shared by any other areas.
			area.bounds.Insert(ToBSPType(tri->model->verts[tri->v[0]].pos));
			area.bounds.Insert(ToBSPType(tri->model->verts[tri->v[1]].pos));
			area.bounds.Insert(ToBSPType(tri->model->verts[tri->v[2]].pos));

			SectorPolyRef poly(new SectorPoly());
			poly->tri = tri;
			poly->winding.Initialize(
				SectorWinding::VertexType(ToBSPType(tri->model->verts[tri->v[0]])), 
				SectorWinding::VertexType(ToBSPType(tri->model->verts[tri->v[1]])), 
				SectorWinding::VertexType(ToBSPType(tri->model->verts[tri->v[2]])),
				ToBSPType(tri->plane)
			);
			root->polys.push_back(poly);
		}
	}

	root->bounds = area.bounds;
	SubdivideSector(root);
}

void BSPBuilder::BuildSharedSectors() {

	Sector *root = new Sector();
	root->bounds.Initialize();

	for (SceneFile::TriModel::Vec::iterator it = m_map->worldspawn->models.begin(); it != m_map->worldspawn->models.end(); ++it) { 
		SceneFile::TriModel::Ref &m = *it;
		if (m->ignore)
			continue;
		if (!(m->contents & kContentsFlag_VisibleContents)) 
			continue;
		if (m->contents & kContentsFlag_Areaportal)
			continue;

		for (SceneFile::TriFaceVec::iterator it2 = m->tris.begin(); it2 != m->tris.end(); ++it2) {
			SceneFile::TriFace &tri = *it2;

			if (tri.surface & kSurfaceFlag_NoDraw)
				continue;
			if (tri.areas.size() < 2)
				continue; // only gather tris with multiple areas set.
			
			root->bounds.Insert(ToBSPType(tri.model->verts[tri.v[0]].pos));
			root->bounds.Insert(ToBSPType(tri.model->verts[tri.v[1]].pos));
			root->bounds.Insert(ToBSPType(tri.model->verts[tri.v[2]].pos));

			SectorPolyRef poly(new SectorPoly());
			poly->tri = &tri;
			poly->winding.Initialize(
				SectorWinding::VertexType(ToBSPType(tri.model->verts[tri.v[0]])), 
				SectorWinding::VertexType(ToBSPType(tri.model->verts[tri.v[1]])), 
				SectorWinding::VertexType(ToBSPType(tri.model->verts[tri.v[2]])),
				ToBSPType(tri.plane)
			);
			root->polys.push_back(poly);
		}
	}

	SubdivideSector(root);
}

void BSPBuilder::SubdivideSector(Sector *sector) {
	const ValueType kMaxSectorSize = ValueType(512);

	Vec3 size = sector->bounds.Size();
	for (int i = 0; i < 3; ++i) {
		if (size[i] > kMaxSectorSize) {
			Vec3 normal(0, 0, 0);
			normal[i] = ValueType(1);
			Plane p(normal, sector->bounds.Origin()[i]);
			
			Sector *front = new Sector();
			front->bounds = sector->bounds;
			normal = front->bounds.Mins();
			normal[i] = sector->bounds.Origin()[i];
			front->bounds.SetMins(normal);

			Sector *back  = new Sector();
			back->bounds = sector->bounds;
			normal = back->bounds.Maxs();
			normal[i] = sector->bounds.Origin()[i];
			back->bounds.SetMaxs(normal);

			SplitSector(p, *sector, *front, *back);

			if (front->polys.empty()) {
				delete sector;
				sector = back;
				delete front;
				SubdivideSector(sector);
				return;
			}

			if (back->polys.empty()) {
				delete sector;
				sector = front;
				delete back;
				SubdivideSector(sector);
				return;
			}

			delete sector;
			SubdivideSector(front);
			SubdivideSector(back);
			return;
		}
	}

	if (sector->polys.empty()) {
		delete sector;
		return;
	}

	m_numInsideTris += (int)sector->polys.size();

	SectorRef ref(sector);

	// add area(s) to sector
	for (SectorPolyVec::const_iterator polyIt = sector->polys.begin(); polyIt != sector->polys.end(); ++polyIt) {
		const SectorPolyRef &poly = *polyIt;
		const SceneFile::TriFace *tri = poly->tri;

		for (SceneFile::AreaNumVec::const_iterator triAreaNum = tri->areas.begin(); triAreaNum != tri->areas.end(); ++triAreaNum) {
			AreaNumVec::const_iterator sectorAreaNum;
			for (sectorAreaNum = sector->areas.begin(); sectorAreaNum != sector->areas.end(); ++sectorAreaNum) {
				if (*triAreaNum == *sectorAreaNum)
					break;
			}

			if (sectorAreaNum == sector->areas.end())
				sector->areas.push_back(*triAreaNum);
		}
	}

	for (AreaNumVec::const_iterator it = sector->areas.begin(); it != sector->areas.end(); ++it) {
		m_areas[*it]->sectors.push_back(ref);
	}

	if (sector->areas.size() > 1) {
		++m_numSharedSectors;
	} else {
		++m_numSectors;
	}

	if (m_numSectors % 100 == 0)
		EmitProgress();
}

void BSPBuilder::SplitSector(const Plane &p, Sector &sector, Sector &front, Sector &back) {
	
	while (!sector.polys.empty()) {
		SectorPolyRef poly = sector.polys.back();
		sector.polys.pop_back();

		Plane::SideType s = poly->winding.Side(p, ValueType(1.0));

		switch (s) {
		case Plane::Front:
			front.polys.push_back(poly);
			break;
		case Plane::Back:
			back.polys.push_back(poly);
			break;
		case Plane::Cross:
			{
				SectorPolyRef f(new SectorPoly(*poly));
				SectorPolyRef b(new SectorPoly(*poly));
				poly->winding.Split(p, &f->winding, &b->winding, ValueType(0));
				RAD_ASSERT(!f->winding.Empty());
				RAD_ASSERT(!b->winding.Empty());
				front.polys.push_back(f);
				back.polys.push_back(b);
			}
			break;
		case Plane::On:
			{
				s = poly->winding.MajorSide(p, ValueType(0));
				switch (s) {
				case Plane::Front:
					front.polys.push_back(poly);
					break;
				case Plane::Back:
					back.polys.push_back(poly);
					break;
				case Plane::On:
					if (p.Normal().Dot(poly->winding.Plane().Normal()) > 0) {
						front.polys.push_back(poly);
					} else {
						back.polys.push_back(poly);
					}
				break;
				}
			}
			break;
		}
	}
}

void BSPBuilder::DecomposeAreaModel(const SceneFile::TriModel &model) {
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

	for (SceneFile::TriFaceVec::const_iterator it = model.tris.begin(); it != model.tris.end(); ++it) {
		if (++m_work % 10000 == 0)
			EmitProgress();

		const SceneFile::TriFace &tri = *it;
		if (tri.outside)
			continue;

		AreaPoly *poly = new AreaPoly();
		poly->planenum = m_planes.FindPlaneNum(ToBSPType(tri.plane));
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
			Log("WARNING: Triangle is inside map but did not get assigned an area!\n");
		} else {
			++m_numInsideTris;
		}
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

	if (poly->planenum == node->planenum) {
		DecomposeAreaPoly(node->children[0].get(), poly);
		return;
	}

	if (poly->planenum == (node->planenum^1)) {
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

	if (front) {
		DecomposeAreaPoly(node->children[0].get(), front);
	}

	if (back) {
		DecomposeAreaPoly(node->children[1].get(), back);
	}
}

} // solid_bsp
} // tools
