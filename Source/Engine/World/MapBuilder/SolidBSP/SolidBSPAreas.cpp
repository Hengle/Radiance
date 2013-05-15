/*! \file SolidBSPSectors.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup map_builder
*/

#include RADPCH
#include "SolidBsp.h"

using namespace world::bsp_file;

namespace tools {
namespace solid_bsp {

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
		if (m->contents&kContentsFlag_Floor)
			continue;
		if (m->contents&kContentsFlag_Areaportal)
			continue;
		if (m->contents&kContentsFlag_Clip)
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
	for (AreaVec::iterator it = m_areas.begin(); it != m_areas.end(); ++it) {
		AreaRef area = *it;
		Log("Area %d: %d model(s)\n", area->area, area->numModels);
	}
	Log("------------\n");

	return true;
}

void BSPBuilder::DecomposeAreaModel(SceneFile::TriModel &model) {
	
	// push each triangle into the bsp, and assign areas.
	bool solid = (model.contents&kContentsFlag_Solid) ? true : false;

	// skip
	if (m_flood && solid && model.outside) {
		++m_numOutsideModels;
		m_numOutsideTris += (int)model.tris.size();
		return;
	}

	bool foundArea = false;
	int numNoDrawFaces = 0;

	for (SceneFile::TriFaceVec::const_iterator it = model.tris.begin(); it != model.tris.end(); ++it) {
		if (++m_work % 1000 == 0)
			EmitProgress();

		const SceneFile::TriFace &tri = *it;
		if (tri.outside)
			continue;
		if (tri.surface & kSurfaceFlag_NoDraw) {
			++numNoDrawFaces;
			continue;
		}

		AreaPoly *poly = new (world::bsp_file::ZBSPBuilder) AreaPoly();
		poly->plane = ToBSPType(tri.plane);
		poly->winding.Initialize(
			ToBSPType(model.verts[tri.v[0]].pos), 
			ToBSPType(model.verts[tri.v[1]].pos), 
			ToBSPType(model.verts[tri.v[2]].pos),
			ToBSPType(tri.plane));
		poly->tri = const_cast<SceneFile::TriFace*>(&tri);
		DecomposeAreaPoly(m_root.get(), poly);

		if (tri.areas.empty()) {
			++m_numOutsideTris;
		} else {
			foundArea = true;
			++m_numInsideTris;
		}
	}

	if (!foundArea) {
		model.outside = true;
		if (!(model.contents&kContentsFlag_Solid)) {
			Log("\nWARNING: '%s' has no visible surfaces inside hull.\n", model.name.c_str.get());
		}
	}

	if (model.outside) {
		++m_numOutsideModels;
	} else {
		++m_numInsideModels;
	}
}

void BSPBuilder::DecomposeAreaPoly(Node *node, AreaPoly *poly) {

	if (poly->tri->contents == kContentsFlag_Sky) {
		poly->tri->areas.insert(0); // sky area
		if (poly->tri->model->areas.find(0) == poly->tri->model->areas.end()) {
			poly->tri->model->areas.insert(0);
			++m_areas[0]->numModels;
		}
		poly->tri->model->outside = false;
		return;
	}

	if (node->planenum == kPlaneNumLeaf) {
		if ((!m_flood || node->occupied) && node->area) {

			poly->tri->areas.insert(node->area->area);
			if (poly->tri->model->areas.find(node->area->area) == poly->tri->model->areas.end()) {
				poly->tri->model->areas.insert(node->area->area);
				++node->area->numModels;
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
			s = Plane::Front;
		} else {
			s = Plane::Back;
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
