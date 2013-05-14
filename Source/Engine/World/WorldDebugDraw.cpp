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
	
	int r = LoadMaterial("Sys/DebugWireframe_M", m_dbgVars.debugWireframe_M);
	if (r != pkg::SR_Success)
		return r;

	r = LoadMaterial("Sys/DebugPortalEdge_M", m_dbgVars.debugPortal_M[0]);
	if (r != pkg::SR_Success)
		return r;

	r = LoadMaterial("Sys/DebugPortal_M", m_dbgVars.debugPortal_M[1]);
	if (r != pkg::SR_Success)
		return r;

	r = LoadMaterial("Sys/DebugWorldBBox_M", m_dbgVars.debugWorldBBox_M);
	if (r != pkg::SR_Success)
		return r;

	r = LoadMaterial("Sys/DebugEntityBBox_M", m_dbgVars.debugEntityBBox_M);
	if (r != pkg::SR_Success)
		return r;

	r = LoadMaterial("Sys/DebugWaypoint_M", m_dbgVars.debugWaypoint_M);
	if (r != pkg::SR_Success)
		return r;

	r = LoadMaterial("Sys/DebugLight0_M", m_dbgVars.debugLightPasses_M[0]);
	if (r != pkg::SR_Success)
		return r;

	r = LoadMaterial("Sys/DebugLight1_M", m_dbgVars.debugLightPasses_M[1]);
	if (r != pkg::SR_Success)
		return r;

	r = LoadMaterial("Sys/DebugLight2_M", m_dbgVars.debugLightPasses_M[2]);
	if (r != pkg::SR_Success)
		return r;

	r = LoadMaterial("Sys/DebugLight3_M", m_dbgVars.debugLightPasses_M[3]);
	if (r != pkg::SR_Success)
		return r;

	r = LoadMaterial("Sys/DebugLight4_M", m_dbgVars.debugLightPasses_M[4]);
	if (r != pkg::SR_Success)
		return r;

	r = LoadMaterial("Sys/DebugLight5_M", m_dbgVars.debugLightPasses_M[5]);
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
		m_dbgVars.debugPortal_M[style].mat->BindStates();
		m_dbgVars.debugPortal_M[style].mat->BindTextures(m_dbgVars.debugPortal_M[style].loader);
		m_dbgVars.debugPortal_M[style].mat->shader->Begin(r::Shader::kPass_Default, *m_dbgVars.debugPortal_M[style].mat);


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
				numIndices = m_rb->DebugTesselateVerts(portal.winding.NumVertices());

			m_dbgVars.debugPortal_M[style].mat->shader->BindStates();
			m_rb->CommitStates();

			if (style == 0) {
				m_rb->DebugDrawLineLoop(portal.winding.NumVertices());
			} else {
				m_rb->DebugDrawIndexedTris(numIndices);
			}
		}

		m_dbgVars.debugPortal_M[style].mat->shader->End();
	}
}

void WorldDraw::DebugDrawLightScissors() {
	m_rb->SetScreenLocalMatrix();
	DebugDrawRects(m_dbgVars.debugWireframe_M, m_dbgVars.debugLightScissors);
}

void WorldDraw::DebugDrawRects(const LocalMaterial &material, const Vec4Vec &rects) {
	material.mat->BindStates();
	material.mat->BindTextures(material.loader);
	material.mat->shader->Begin(r::Shader::kPass_Default, *material.mat);

	for (Vec4Vec::const_iterator it = rects.begin(); it != rects.end(); ++it)
		DebugDrawRectBatch(material, *it);

	material.mat->shader->End();
}

void WorldDraw::DebugDrawRect(const LocalMaterial &material, const Vec4 &rect) {
	m_rb->SetScreenLocalMatrix();
	material.mat->BindStates();
	material.mat->BindTextures(material.loader);
	material.mat->shader->Begin(r::Shader::kPass_Default, *material.mat);
	DebugDrawRectBatch(material, rect);
	material.mat->shader->End();
}

void WorldDraw::DebugDrawRectBatch(const LocalMaterial &material, const Vec4 &rect) {
	Vec3 verts[4];

	verts[0] = Vec3(rect[0], rect[1], 0.f);
	verts[1] = Vec3(rect[0], rect[3], 0.f);
	verts[2] = Vec3(rect[2], rect[3], 0.f);
	verts[3] = Vec3(rect[2], rect[1], 0.f);

	m_rb->DebugUploadVerts(verts, 4);
	material.mat->shader->BindStates();
	m_rb->CommitStates();
	m_rb->DebugDrawLineLoop(4);
}

void WorldDraw::DebugDrawBBoxes(const LocalMaterial &material, const BBoxVec &bboxes, bool wireframe) {
	if (bboxes.empty())
		return;

	material.mat->BindStates();
	material.mat->BindTextures(material.loader);
	material.mat->shader->Begin(r::Shader::kPass_Default, *material.mat);

	for (BBoxVec::const_iterator it = bboxes.begin(); it != bboxes.end(); ++it)
		DebugDrawBBoxBatch(material, *it, wireframe);

	material.mat->shader->End();
}

void WorldDraw::DebugDrawBBox(const LocalMaterial &material, const BBox &bbox, bool wireframe) {
	material.mat->BindStates();
	material.mat->BindTextures(material.loader);
	material.mat->shader->Begin(r::Shader::kPass_Default, *material.mat);
	DebugDrawBBoxBatch(material, bbox, wireframe);
	material.mat->shader->End();
}

void WorldDraw::DebugDrawBBoxBatch(const LocalMaterial &material, const BBox &bbox, bool wireframe) {
	typedef stackify<zone_vector<Vec3, ZWorldT>::type, 4> StackVec;
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
	if (wireframe) {
		m_rb->DebugDrawLineLoop(4);
	} else {
		m_rb->DebugDrawPoly(4);
	}
	v->clear();
	
	// -X
	v->push_back(Vec3(mins[0], maxs[1], maxs[2]));
	v->push_back(Vec3(mins[0], maxs[1], mins[2]));
	v->push_back(mins);
	v->push_back(Vec3(mins[0], mins[1], maxs[2]));

	m_rb->DebugUploadVerts(&v[0], 4);
	material.mat->shader->BindStates();
	m_rb->CommitStates();
	if (wireframe) {
		m_rb->DebugDrawLineLoop(4);
	} else {
		m_rb->DebugDrawPoly(4);
	}
	v->clear();

	// +Y
	v->push_back(maxs);
	v->push_back(Vec3(maxs[0], maxs[1], mins[2]));
	v->push_back(Vec3(mins[0], maxs[1], mins[2]));
	v->push_back(Vec3(mins[0], maxs[1], maxs[2]));

	m_rb->DebugUploadVerts(&v[0], 4);
	material.mat->shader->BindStates();
	m_rb->CommitStates();
	if (wireframe) {
		m_rb->DebugDrawLineLoop(4);
	} else {
		m_rb->DebugDrawPoly(4);
	}
	v->clear();

	// -Y
	v->push_back(Vec3(maxs[0], mins[1], maxs[2]));
	v->push_back(Vec3(mins[0], mins[1], maxs[2]));
	v->push_back(mins);
	v->push_back(Vec3(maxs[0], mins[1], mins[2]));
	
	m_rb->DebugUploadVerts(&v[0], 4);
	material.mat->shader->BindStates();
	m_rb->CommitStates();
	if (wireframe) {
		m_rb->DebugDrawLineLoop(4);
	} else {
		m_rb->DebugDrawPoly(4);
	}
	v->clear();

	// +Z
	v->push_back(maxs);
	v->push_back(Vec3(mins[0], maxs[1], maxs[2]));
	v->push_back(Vec3(mins[0], mins[1], maxs[2]));
	v->push_back(Vec3(maxs[0], mins[1], maxs[2]));

	m_rb->DebugUploadVerts(&v[0], 4);
	material.mat->shader->BindStates();
	m_rb->CommitStates();
	if (wireframe) {
		m_rb->DebugDrawLineLoop(4);
	} else {
		m_rb->DebugDrawPoly(4);
	}
	v->clear();
	
	// -Z
	v->push_back(mins);
	v->push_back(Vec3(maxs[0], mins[1], mins[2]));
	v->push_back(Vec3(maxs[0], maxs[1], mins[2]));
	v->push_back(Vec3(mins[0], maxs[1], mins[2]));

	m_rb->DebugUploadVerts(&v[0], 4);
	material.mat->shader->BindStates();
	m_rb->CommitStates();
	if (wireframe) {
		m_rb->DebugDrawLineLoop(4);
	} else {
		m_rb->DebugDrawPoly(4);
	}
	v->clear();

}

void WorldDraw::DebugDrawActiveWaypoints() {
	bool begin = false;

	for (U32 i = 0; i < m_world->bspFile->numWaypoints; ++i) {
		if (!(m_world->floors->WaypointState((int)i) & Floors::kWaypointState_Enabled))
			continue;

		if (!begin) {
			begin = true;
			m_dbgVars.debugWaypoint_M.mat->BindStates();
			m_dbgVars.debugWaypoint_M.mat->BindTextures(m_dbgVars.debugWaypoint_M.loader);
			m_dbgVars.debugWaypoint_M.mat->shader->Begin(r::Shader::kPass_Default, *m_dbgVars.debugWaypoint_M.mat);
		}

		const bsp_file::BSPWaypoint *waypoint = m_world->bspFile->Waypoints() + i;
		const Vec3 kPos(waypoint->pos[0], waypoint->pos[1], waypoint->pos[2]);
		const BBox kBox(kPos - Vec3(12, 12, 12), kPos + Vec3(12, 12, 12));
		DebugDrawBBoxBatch(m_dbgVars.debugWaypoint_M, kBox, false);
	}

	if (begin) {
		m_dbgVars.debugWaypoint_M.mat->shader->End();
	}
}

void WorldDraw::DebugDrawFloorMoves() {
	LocalMaterial &material = m_dbgVars.debugWorldBBox_M;

	bool begin = false;

	for (Entity::IdMap::const_iterator it = m_world->m_ents.begin(); it != m_world->m_ents.end(); ++it) {
		const Entity::Ref &entity = it->second;

		if (entity->ps->activeMove) {
			if (!begin) {
				begin = true;
				material.mat->BindStates();
				material.mat->BindTextures(material.loader);
				material.mat->shader->Begin(r::Shader::kPass_Default, *material.mat);
			}

			DebugDrawFloorMoveBatch(material, *entity->ps->activeMove);
		}
	}

	if (begin)
		material.mat->shader->End();
}

void WorldDraw::DebugDrawFloorMoveBatch(const LocalMaterial &material, const FloorMove &move) {
	enum {
		kSplineTess = 8
	};

	const float kStep = 1.f / (kSplineTess-1);
	Vec3 pts[kSplineTess];

	if (move.route->steps->empty())
		return;

	for (FloorMove::Step::Vec::const_iterator it = move.route->steps->begin(); it != move.route->steps->end(); ++it) {
		const FloorMove::Step &step = *it;

		float t = 0.f;

		for (int i = 0; i < kSplineTess; ++i, t += kStep) {
			step.path.Eval(t, pts[i], 0);
		}

		BBox bbox(pts[kSplineTess-1] - Vec3(16, 16, 16), pts[kSplineTess-1] + Vec3(16, 16, 16));
		DebugDrawBBoxBatch(material, bbox, true);

		m_rb->DebugUploadVerts(&pts[0], kSplineTess);
		material.mat->shader->BindStates();
		m_rb->CommitStates();
		m_rb->DebugDrawLineStrip(kSplineTess);
	}
}

void WorldDraw::DebugDrawLightPasses(ViewDef &view) {
	for (int i = 0; i < r::Material::kNumSorts; ++i) {
		for (details::MBatchIdMap::const_iterator it = view.batches.begin(); it != view.batches.end(); ++it) {
			const details::MBatch &batch = *it->second;

			if (batch.matRef->mat->sort != (r::Material::Sort)i) {
				continue;
			}

			if (batch.matRef->mat->maxLights > 0) {
				DebugDrawLightPass(batch);
			} else {
				DrawUnlitBatch(batch, false);
			}
		}
	}
}

void WorldDraw::DebugDrawLightPass(const details::MBatch &batch) {

	r::Material *mat = batch.matRef->mat;
		
	Vec3 pos;
	Vec3 angles;
	
	bool diffuse = mat->shader->HasPass(r::Shader::kPass_Diffuse1);
	bool diffuseSpecular = mat->shader->HasPass(r::Shader::kPass_DiffuseSpecular1);

	RAD_ASSERT_MSG(diffuse, "All lighting shaders must have diffuse pass!");

	const int kMaxLights = std::min(mat->maxLights.get(), m_world->cvars->r_maxLightsPerPass.value.get());

	for (details::MBatchDrawLink *link = batch.head; link; link = link->next) {
		MBatchDraw *draw = link->draw;

		const bool tx = draw->GetTransform(pos, angles);
		if (tx) {
			m_rb->PushMatrix(pos, draw->scale, angles);
		}

		int numPasses = 0;
		
		details::LightInteraction *interaction = draw->m_interactions;
		while (interaction) {
			if (interaction->light->m_visFrame != m_markFrame) {
				interaction = interaction->nextOnBatch;
				continue; // not visible this frame.
			}
			const Light::LightStyle kStyle = interaction->light->style;
			if (kStyle & Light::kStyle_CastShadows) {
				++numPasses; // shadow casters cannot be batched.
			}
			interaction = interaction->nextOnBatch;
		}

		bool didDiffuse = false;
		bool didDiffuseSpecular = false;

		// count the # of lighting passes required to render this object.
		for (;;) {
			interaction = draw->m_interactions;

			while (interaction) {

				// batch up to kNumLights
				int numLights = 0;
	
				while (interaction && (numLights < kMaxLights)) {
					if (interaction->light->m_visFrame != m_markFrame) {
						interaction = interaction->nextOnBatch;
						continue; // not visible this frame.
					}

					const Light::LightStyle kStyle = interaction->light->style;

					if (!(kStyle & Light::kStyle_CastShadows)) {
						bool add = false;

						if ((kStyle&Light::kStyle_DiffuseSpecular) == Light::kStyle_DiffuseSpecular) {
							add = !didDiffuseSpecular;
						} else if (kStyle&Light::kStyle_Diffuse) {
							add = diffuse && (!diffuseSpecular || didDiffuseSpecular);
						}
					
						if (add) {
							++numLights;
						}
					}

					interaction = interaction->nextOnBatch;
				}

				if (numLights > 0) {
					++numPasses;
				}

				if (numPasses >= 5)
					break; // debug display doesn't do more than this.
			}

			if (numPasses >= 5)
				break; // debug display doesn't do more than this.

			if (diffuseSpecular && !didDiffuseSpecular) {
				didDiffuseSpecular = true;
			} else {
				break;
			}
		}

		const LocalMaterial &debugMat = m_dbgVars.debugLightPasses_M[numPasses];
		
		debugMat.mat->BindStates();
		debugMat.mat->BindTextures(debugMat.loader);
		debugMat.mat->shader->Begin(r::Shader::kPass_Default, *debugMat.mat);
		draw->Bind(debugMat.mat->shader.get().get());
		debugMat.mat->shader->BindStates();
		m_rb->CommitStates();
		draw->CompileArrayStates(*debugMat.mat->shader.get());
		draw->Draw();

		if (tx)
			m_rb->PopMatrix();

		m_rb->ReleaseArrayStates();
		debugMat.mat->shader->End();
	}
}

} // world

#endif // defined(WORLD_DEBUG_DRAW)
