// WorldDebugDraw.cpp
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "World.h"
#if defined(WORLD_DEBUG_DRAW)
#include "../Game/Game.h"
#include "../Game/GameCVars.h"
#include "../Renderer/Shader.h"

using namespace r;

namespace world {

int WorldDraw::LoadDebugMaterials() {
	
	int r = LoadMaterial("Sys/DebugWireframe_M", m_dbgVars.debugWireframe);
	if (r != pkg::SR_Success)
		return r;

	r = LoadMaterial("Sys/DebugPortalEdge_M", m_dbgVars.debugPortal[0]);
	if (r != pkg::SR_Success)
		return r;

	r = LoadMaterial("Sys/DebugPortal_M", m_dbgVars.debugPortal[1]);
	if (r != pkg::SR_Success)
		return r;

	r = LoadMaterial("Sys/DebugWorldBBox_M", m_dbgVars.debugWorldBBox);
	if (r != pkg::SR_Success)
		return r;

	r = LoadMaterial("Sys/DebugEntityBBox_M", m_dbgVars.debugEntityBBox);
	if (r != pkg::SR_Success)
		return r;

	return m_rb->LoadMaterials();
}

void WorldDraw::DebugDrawPortals(ViewDef &view) {
	for (int i = 0; i < kMaxAreas; ++i) {
		if (view.areas.test(i))
			DebugDrawAreaportals(i);
	}
}

void WorldDraw::DebugDrawAreaportals(int areaNum) { 

	RAD_ASSERT(areaNum < (int)m_world->m_areas.size());

	for (int style = 0; style < 2; ++style) {
		m_dbgVars.debugPortal[style].mat->BindStates();
		m_dbgVars.debugPortal[style].mat->BindTextures(m_dbgVars.debugPortal[style].loader);
		m_dbgVars.debugPortal[style].mat->shader->Begin(r::Shader::kPass_Default, *m_dbgVars.debugPortal[style].mat);


		const dBSPArea &area = m_world->m_areas[areaNum];

		for(int i = 0; i < area.numPortals; ++i) {

			int areaportalNum = (int)*(m_world->m_bsp->AreaportalIndices() + area.firstPortal + i);
			const dAreaportal &portal = m_world->m_areaportals[areaportalNum];

			m_rb->DebugUploadVerts(
				&portal.winding.Vertices()[0],
				portal.winding.NumVertices()
			);

			int numIndices = 0;
			if (style == 1)
				numIndices = m_rb->DebugUploadAutoTessTriIndices(portal.winding.NumVertices());

			m_dbgVars.debugPortal[style].mat->shader->BindStates();
			m_rb->CommitStates();

			if (style == 0) {
				m_rb->DebugDrawLineLoop(portal.winding.NumVertices());
			} else {
				m_rb->DebugDrawIndexedTris(numIndices);
			}
		}

		m_dbgVars.debugPortal[style].mat->shader->End();
	}
}

void WorldDraw::DebugDrawBBoxes(const LocalMaterial &material, const BBoxVec &bboxes) {
	if (bboxes.empty())
		return;

	material.mat->BindStates();
	material.mat->BindTextures(material.loader);
	material.mat->shader->Begin(r::Shader::kPass_Default, *material.mat);

	for (BBoxVec::const_iterator it = bboxes.begin(); it != bboxes.end(); ++it)
		DebugDrawBBox(material, *it);

	material.mat->shader->End();
}

void WorldDraw::DebugDrawBBox(const LocalMaterial &material, const BBox &bbox) {
	typedef stackify<std::vector<Vec3>, 4> StackVec;
	StackVec v;

	const Vec3 &mins = bbox.Mins();
	const Vec3 &maxs = bbox.Maxs();

	// +X
	v->push_back(maxs);
	v->push_back(Vec3(maxs[0], mins[1], maxs[2]));
	v->push_back(Vec3(maxs[0], mins[1], mins[2]));
	v->push_back(Vec3(maxs[0], maxs[1], mins[2]));

	m_rb->DebugUploadVerts(&v[0], 4);
	material.mat->shader->BindStates();
	m_rb->CommitStates();
	m_rb->DebugDrawLineLoop(4);
	v->clear();
	
	// -X
	v->push_back(Vec3(mins[0], maxs[1], maxs[2]));
	v->push_back(Vec3(mins[0], maxs[1], mins[2]));
	v->push_back(mins);
	v->push_back(Vec3(mins[0], mins[1], maxs[2]));

	m_rb->DebugUploadVerts(&v[0], 4);
	material.mat->shader->BindStates();
	m_rb->CommitStates();
	m_rb->DebugDrawLineLoop(4);
	v->clear();

	// +Y
	v->push_back(maxs);
	v->push_back(Vec3(maxs[0], maxs[1], mins[2]));
	v->push_back(Vec3(mins[0], maxs[1], mins[2]));
	v->push_back(Vec3(mins[0], maxs[1], maxs[2]));

	m_rb->DebugUploadVerts(&v[0], 4);
	material.mat->shader->BindStates();
	m_rb->CommitStates();
	m_rb->DebugDrawLineLoop(4);
	v->clear();

	// -Y
	v->push_back(Vec3(maxs[0], mins[1], maxs[2]));
	v->push_back(Vec3(mins[0], mins[1], maxs[2]));
	v->push_back(mins);
	v->push_back(Vec3(maxs[0], mins[1], mins[2]));
	
	m_rb->DebugUploadVerts(&v[0], 4);
	material.mat->shader->BindStates();
	m_rb->CommitStates();
	m_rb->DebugDrawLineLoop(4);
	v->clear();

	// +Z
	v->push_back(maxs);
	v->push_back(Vec3(mins[0], maxs[1], maxs[2]));
	v->push_back(Vec3(mins[0], mins[1], maxs[2]));
	v->push_back(Vec3(maxs[0], mins[1], maxs[2]));

	m_rb->DebugUploadVerts(&v[0], 4);
	material.mat->shader->BindStates();
	m_rb->CommitStates();
	m_rb->DebugDrawLineLoop(4);
	v->clear();
	
	// -Z
	v->push_back(mins);
	v->push_back(Vec3(maxs[0], mins[1], mins[2]));
	v->push_back(Vec3(maxs[0], maxs[1], mins[2]));
	v->push_back(Vec3(mins[0], maxs[1], mins[2]));

	m_rb->DebugUploadVerts(&v[0], 4);
	material.mat->shader->BindStates();
	m_rb->CommitStates();
	m_rb->DebugDrawLineLoop(4);
	v->clear();

}

} // world

#endif // defined(WORLD_DEBUG_DRAW)
