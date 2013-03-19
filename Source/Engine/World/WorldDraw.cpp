/*! \file WorldDraw.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup ui
*/

#include RADPCH
#include "../App.h"
#include "../Engine.h"
#include "../Game/Game.h"
#include "../Game/GameCVars.h"
#include "../Renderer/Shader.h"
#include "../Packages/Packages.h"
#include "../UI/UIWidget.h"
#include "World.h"
#include "Occupant.h"
#include "ScreenOverlay.h"
#include <Runtime/Container/ZoneList.h>

using namespace r;

namespace world {

void RotateForDebugCamera();

namespace details {

MBatch::MBatch() : matRef(0), head(0), tail(0) {
}

MBatch::~MBatch() {
	RAD_ASSERT(owner);
	while (head) {
		tail = head->next;
		owner->m_linkPool.Destroy(head);
		head = tail;
	}
}

void MBatch::AddDraw(MBatchDraw &draw) {
	RAD_ASSERT(owner);
	MBatchDrawLink *link = owner->m_linkPool.Construct();
	link->draw = &draw;
	link->next = 0;

	if (tail)
		tail->next = link;
	tail = link;

	if (!head)
		head = link;
}

} // details

///////////////////////////////////////////////////////////////////////////////

Vec4 WorldDraw::MStaticWorldMeshBatch::s_rgba(Vec4(1, 1, 1, 1));
Vec3 WorldDraw::MStaticWorldMeshBatch::s_scale(Vec3(1, 1, 1));

WorldDraw::MStaticWorldMeshBatch::MStaticWorldMeshBatch(const r::Mesh::Ref &m, const BBox &bounds, int matId) :
MBatchDraw(matId), m_m(m), m_bounds(bounds) {
}

void WorldDraw::MStaticWorldMeshBatch::Bind(r::Shader *shader) {
	m_m->BindAll(shader);
}

void WorldDraw::MStaticWorldMeshBatch::CompileArrayStates(r::Shader &shader) {
	m_m->CompileArrayStates(shader);
}

void WorldDraw::MStaticWorldMeshBatch::FlushArrayStates(r::Shader *shader) {
	m_m->FlushArrayStates(shader);
}

void WorldDraw::MStaticWorldMeshBatch::Draw() {
	m_m->Draw();
}

///////////////////////////////////////////////////////////////////////////////

void WorldDraw::Counters::Clear() {
	fps = 0.f;
	drawnAreas = 0;
	testedPortals = 0;
	drawnPortals = 0;
	testedWorldModels = 0;
	drawnWorldModels = 0;
	drawnEntities = 0;
	testedEntityModels = 0;
	drawnEntityModels = 0;
	numBatches = 0;
	numTris = 0;
	numMaterials = 0;
}

WorldDraw::WorldDraw(World *w) : 
m_world(w), 
m_frame(-1), 
m_markFrame(-1),
m_uiOnly(false),
m_init(false) {
	m_rb = RB_WorldDraw::New(w);
}

WorldDraw::~WorldDraw() {
	m_worldModels.clear();
	m_overlays.clear();
	m_rb.reset();
	m_refMats.clear();

	if (m_init) {
		RAD_ASSERT(m_batchPool.numUsedObjects == 0);
		m_batchPool.Destroy();
		RAD_ASSERT(m_linkPool.numUsedObjects == 0);
		m_linkPool.Destroy();
	}
}

int WorldDraw::LoadMaterials() {
	
#if defined(WORLD_DEBUG_DRAW)
	int r = LoadDebugMaterials();
	if (r != pkg::SR_Success)
		return r;
#endif

	return m_rb->LoadMaterials();
}

int WorldDraw::LoadMaterial(const char *name, LocalMaterial &mat) {
	mat.asset = App::Get()->engine->sys->packages->Resolve(name, pkg::Z_Engine);
	if (!mat.asset || mat.asset->type != asset::AT_Material) {
		COut(C_Error) << "Error: Unable to load '" << name << "'." << std::endl;
		return pkg::SR_FileNotFound;
	}
	
	int r = mat.asset->Process(
		xtime::TimeSlice::Infinite,
		pkg::P_Load | pkg::P_FastPath
	);

	if (r != pkg::SR_Success) {
		COut(C_Error) << "Error: Unable to load '" << name << "'." << std::endl;
		return (r != pkg::SR_Pending) ? r : pkg::SR_MetaError;
	}

	asset::MaterialParser *parser = asset::MaterialParser::Cast(mat.asset);
	if (!parser || !parser->valid) {
		COut(C_Error) << "Error: Unable to load '" << name << "'." << std::endl;
		return pkg::SR_MetaError;
	}

	mat.mat = parser->material;
	mat.loader = asset::MaterialLoader::Cast(mat.asset);
	return pkg::SR_Success;
}

void WorldDraw::Init(const bsp_file::BSPFile::Ref &bsp) {

	m_batchPool.Create(ZWorld, "world-draw-batch", 64);
	m_linkPool.Create(ZWorld, "world-draw-link", 64);

	m_init = true;
}

int WorldDraw::Precache() {
	int r = m_rb->Precache();
	if (r != pkg::SR_Success)
		return r;

	// Precache materials.
	ScreenOverlay::Ref overlay;

	for (details::MatRefMap::const_iterator it = m_refMats.begin(); it != m_refMats.end(); ++it) {
		const details::MatRef &mat = it->second;

		if (overlay) {
			overlay->m_mat = &mat;
		} else {
			overlay = CreateScreenOverlay(mat.asset->id);
		}

		RAD_ASSERT(overlay);
		DrawOverlay(*overlay);
	}

	m_rb->Finish();

	return pkg::SR_Success;
}

void WorldDraw::AddEffect(int id, const PostProcessEffect::Ref &fx) {
	m_postFX[id] = fx;
	if (fx)
		AddMaterial(fx->asset->id);
}

void WorldDraw::Tick(float dt) {

	for (details::MatRefMap::const_iterator it = m_refMats.begin(); it != m_refMats.end(); ++it)
		it->second.mat->Sample(m_world->time, dt);

	for (ScreenOverlay::List::const_iterator it = m_overlays.begin(); it != m_overlays.end(); ++it)
		(*it)->Tick(dt);

	for (PostProcessEffect::Map::const_iterator it = m_postFX.begin(); it != m_postFX.end(); ++it)
		it->second->Tick(dt);
}

void WorldDraw::DeleteBatch(details::MBatch *batch) {
	RAD_ASSERT(batch->owner);
	batch->owner->m_batchPool.Destroy(batch);
}

details::MBatchRef WorldDraw::AllocateBatch() {
	details::MBatch *batch = m_batchPool.Construct();
	RAD_OUT_OF_MEM(batch);
	batch->owner = this;
	return details::MBatchRef(batch, &WorldDraw::DeleteBatch);
}

details::MatRef *WorldDraw::AddMaterialRef(int id) {
	details::MatRefMap::iterator it = m_refMats.find(id);
	if (it != m_refMats.end())
		return &it->second;

	pkg::Asset::Ref asset = App::Get()->engine->sys->packages->Asset(id, pkg::Z_Engine);
	if (!asset)
		return 0;

	asset::MaterialParser *parser = asset::MaterialParser::Cast(asset);
	RAD_VERIFY(parser);

	details::MatRef r;
	r.asset = asset;
	r.mat = parser->material;
	r.loader = asset::MaterialLoader::Cast(asset);
	RAD_VERIFY(r.loader);

	std::pair<details::MatRefMap::iterator, bool> p = m_refMats.insert(details::MatRefMap::value_type(id, r));
	return &p.first->second;
}

details::MBatchRef WorldDraw::AddViewBatch(ViewDef &view, int id) {
	details::MBatchIdMap::iterator it = view.batches.find(id);
	if (it != view.batches.end())
		return it->second;

	details::MatRef *matRef = AddMaterialRef(id);
	if (!matRef)
		return details::MBatchRef();

	details::MBatchRef b(AllocateBatch());
	b->matRef = matRef;
	view.batches.insert(details::MBatchIdMap::value_type(id, b));
	return b;
}

bool WorldDraw::AddMaterial(int id) {
	return AddMaterialRef(id);
}

ScreenOverlay::Ref WorldDraw::CreateScreenOverlay(int matId)
{
	details::MatRef *matRef = AddMaterialRef(matId);
	if (!matRef)
		return ScreenOverlay::Ref();
	return ScreenOverlay::Ref(new (ZWorld) ScreenOverlay(this, *matRef));
}

void WorldDraw::AddStaticWorldMesh(const r::Mesh::Ref &m, const BBox &bounds, int matId) {
	RAD_ASSERT(m);
	MStaticWorldMeshBatch::Ref r(new (ZWorld) MStaticWorldMeshBatch(m, bounds, matId));
	m_worldModels.push_back(r);
}

void WorldDraw::Draw(Counters *counters) {
	m_counters.Clear();

	bool postFX = false;

	if (!m_uiOnly) {
		for (PostProcessEffect::Map::const_iterator it = m_postFX.begin(); it != m_postFX.end(); ++it) {
			if (it->second->enabled) {
				postFX = true;
				break;
			}
		}
	}

	m_rb->BeginFrame();

	if (m_uiOnly) {
		m_rb->ClearBackBuffer();
	} else {
		if (postFX)
			m_rb->BindRenderTarget();

		m_rb->ClearBackBuffer();
		m_rb->SetPerspectiveMatrix();
		DrawView();
		m_rb->SetScreenLocalMatrix();
		DrawOverlays();

		if (postFX)
			PostProcess();
	}

	DrawUI();

	m_rb->Finish();
	m_rb->EndFrame();

	if (counters)
		*counters = m_counters;
}

void WorldDraw::FindViewArea(ViewDef &view) {
	dBSPLeaf *leaf = m_world->LeafForPoint(view.camera.pos);
	view.area = leaf ? leaf->area : -1;
}

void WorldDraw::SetupFrustumPlanes(ViewDef &view) {

	// Things on front of frustum planes are outside the frustum
	// Kind of reversed from normal but easier for convex volume clipping.

	int vpx, vpy, vpw, vph;
	m_world->game->Viewport(vpx, vpy, vpw, vph);
	float yaspect = ((float)vph/(float)vpw);
	float xfov = math::DegToRad(view.camera.fov.get()) * 0.5f;
	float yfov = xfov * yaspect;
	const Vec3 &fwd = view.camera.fwd;
	const Vec3 &left = view.camera.left;
	const Vec3 &up = view.camera.up;

	float s,c;
	Vec3 normals[ViewDef::kNumFrustumPlanes];
	int idx = 0;

	if (ViewDef::kNumFrustumPlanes > 4) {
		normals[idx++] = fwd;
	}

	if (ViewDef::kNumFrustumPlanes > 5) {
		normals[idx++] = -fwd;
	}
	
	math::SinAndCos(&s, &c, xfov);

	normals[idx++] = fwd * s + left * c;
	normals[idx++] = fwd * s + left * -c;

	math::SinAndCos(&s, &c, yfov);
	
	normals[idx++] = fwd * s + up * c;
	normals[idx++] = fwd * s + up * -c;

	for (int i = 0; i < ViewDef::kNumFrustumPlanes; ++i) {
		Plane p;

		for (int k = 0; k < 3; ++k) {
			float d = normals[i][k];
			if (d < 0.001f && d > -0.001f) {
				normals[i][k] = 0.f;
			}
		}

		normals[i].Normalize();

		if ((i == 0) && (ViewDef::kNumFrustumPlanes > 4)) {
			Vec3 t = normals[0] * 4.f;
			p.Initialize(normals[i], view.camera.pos.get() + t, Plane::Unit);
		} else if ((i == 1) && (ViewDef::kNumFrustumPlanes > 5)) {
			Vec3 t = normals[0] * view.camera.farClip.get();
			p.Initialize(normals[i], view.camera.pos.get() + t, Plane::Unit);
		} else {
			p.Initialize(normals[i], view.camera.pos.get(), Plane::Unit);
		}

		view.frustum.push_back(p);
	}
}

void WorldDraw::VisMarkAreas(ViewDef &view) {

	if (view.area < 0)
		return; // no area set.

	StackWindingStackVec frustumVolume;
	BBox frustumBounds;

	World::MakeVolume(&view.frustum[0], (int)view.frustum.size(), frustumVolume, frustumBounds);

	ClippedAreaVolumeStackVec frustumAreas;
	m_world->ClipOccupantVolume(
		&view.camera.pos.get(),
		&frustumVolume,
		frustumBounds,
		&frustumAreas,
		view.area,
		-1,
		view.areas,
		&m_counters
	);

	++m_markFrame;

	// add occupants from area the view is in.
	VisMarkArea(view, view.area, frustumVolume, frustumBounds);

	for (ClippedAreaVolumeStackVec::const_iterator it = frustumAreas->begin(); it != frustumAreas->end(); ++it) {
		const ClippedAreaVolume &volume = *it;
		VisMarkArea(view, volume.area, volume.volume, volume.bounds);
	}

}

void WorldDraw::VisMarkArea(
	ViewDef &view,
	int areaNum, 
	const StackWindingStackVec &volume, 
	const BBox &volumeBounds
) {

	RAD_ASSERT(areaNum > -1);
	RAD_ASSERT(areaNum < (int)m_world->m_areas.size());
	const dBSPArea &area = m_world->m_areas[areaNum];

	++m_counters.drawnAreas;

	for (int i = 0; i < area.numModels; ++i) {
		U16 modelNum = *(m_world->m_bsp->ModelIndices() + i + area.firstModel);
		RAD_ASSERT(modelNum < (U16)m_worldModels.size());

		const MStaticWorldMeshBatch::Ref &m = m_worldModels[modelNum];
		if (!m->visible)
			continue;

		if (m->m_markFrame != m_markFrame) {
			m->m_markFrame = m_markFrame;
			++m_counters.testedWorldModels;
		}

		if (m->m_visibleFrame != m_markFrame) {
#if defined(WORLD_DEBUG_DRAW)
			if (m_world->cvars->r_showworldbboxes.value)
				m_dbgVars.debugWorldBBoxes.push_back(m->bounds);
#endif
			if (!m_world->cvars->r_frustumcull.value || ClipBounds(volume, volumeBounds, m->bounds)) {
				m->m_visibleFrame = m_markFrame;
				++m_counters.drawnWorldModels;
				details::MBatchRef batch = AddViewBatch(view, m->m_matId);
				if (batch)
					batch->AddDraw(*m);
			}
		}
	}

	// add entities.
	
	for (EntityPtrSet::const_iterator it = area.entities.begin(); it != area.entities.end(); ++it) {
		Entity *e = *it;
		if (e->m_markFrame != m_markFrame) {
			e->m_markFrame = m_markFrame;
			++m_counters.drawnEntities;
		}

		for (DrawModel::Map::const_iterator it = e->models->begin(); it != e->models->end(); ++it) {
			const DrawModel::Ref &m = it->second;
			if (!m->visible)
				continue;

			if (m->m_markFrame != m_markFrame) {
				m->m_markFrame = m_markFrame;
				++m_counters.testedEntityModels;
			}

			if (m->m_visibleFrame != m_markFrame) {
				BBox bounds(m->bounds);
				bounds.Translate(e->ps->worldPos);
#if defined(WORLD_DEBUG_DRAW)
				if (m_world->cvars->r_showentitybboxes.value)
					m_dbgVars.debugEntityBBoxes.push_back(bounds);
#endif
				if (!m_world->cvars->r_frustumcull.value || ClipBounds(volume, volumeBounds, bounds)) {
					m->m_visibleFrame = m_markFrame;
					++m_counters.drawnEntityModels;
					for (MBatchDraw::RefVec::const_iterator it = m->m_batches.begin(); it != m->m_batches.end(); ++it) {
						const MBatchDraw::Ref &draw = *it;
						
						if (draw->m_markFrame != m_markFrame) {
							draw->m_markFrame = m_markFrame;
							draw->m_visibleFrame = m_markFrame;
							details::MBatchRef batch = AddViewBatch(view, draw->m_matId);
							if (batch)
								batch->AddDraw(*draw);
						}
					}
				}
			}
		}
	}

	// add batch occupants.
	
	for (MBatchOccupantPtrSet::const_iterator it = area.occupants.begin(); it != area.occupants.end(); ++it) {
		MBatchOccupant *o = *it;
		if (!o->visible)
			continue;

		if (o->m_markFrame != m_markFrame) {
			o->m_markFrame = m_markFrame;
			++m_counters.drawnEntities;
		}

		for (MBatchDraw::RefVec::const_iterator it = o->batches->begin(); it != o->batches->end(); ++it) {
			const MBatchDraw::Ref &m = *it;
			if (!m->visible)
				continue;

			if (m->m_markFrame != m_markFrame) {
				m->m_markFrame = m_markFrame;
				++m_counters.testedEntityModels;
			}

			if (m->m_visibleFrame != m_markFrame) {
				const BBox &bounds = m->bounds;
#if defined(WORLD_DEBUG_DRAW)
				if (m_world->cvars->r_showworldbboxes.value)
					m_dbgVars.debugWorldBBoxes.push_back(bounds);
#endif
				if (!m_world->cvars->r_frustumcull.value || ClipBounds(volume, volumeBounds, bounds)) {
					m->m_visibleFrame = m_markFrame;
					++m_counters.drawnEntityModels;
					details::MBatchRef batch = AddViewBatch(view, m->m_matId);
					if (batch)
						batch->AddDraw(*m);	
				}
			}
		}
	}
}

bool WorldDraw::ClipBounds(const StackWindingStackVec &volume, const BBox &volumeBounds, const BBox &bounds) {
	
	// volume is convex.

	if (!volumeBounds.Touches(bounds))
		return false;

	for (StackWindingStackVec::const_iterator it = volume->begin(); it != volume->end(); ++it) {
		const StackWinding &w = *it;
		if (w.Plane().Side(bounds) == Plane::Back)
			return false;
	}

	// TODO: better frustum clipping
	return true;
}

void WorldDraw::DrawView() {
	++m_frame;
	
	ViewDef view;
	view.camera = *m_world->camera.get();

	FindViewArea(view);
	SetupFrustumPlanes(view);

	VisMarkAreas(view);

	m_rb->RotateForCamera(view.camera);
	m_rb->SetWorldStates();

	m_rb->numTris = 0;

	DrawViewBatches(view, false);

	m_counters.numTris += m_rb->numTris;

#if defined(WORLD_DEBUG_DRAW)

	if (m_world->cvars->r_showportals.value)
		DebugDrawPortals(view);
	if (m_world->cvars->r_showtris.value) {
		m_rb->wireframe = true;
		DrawViewBatches(view, true);
		m_rb->wireframe = false;
	}

	if (m_world->cvars->r_showentitybboxes.value) {
		DebugDrawBBoxes(m_dbgVars.debugEntityBBox_M, m_dbgVars.debugEntityBBoxes, true);
		m_dbgVars.debugEntityBBoxes.clear();
	}
	
	if (m_world->cvars->r_showworldbboxes.value) {
		DebugDrawBBoxes(m_dbgVars.debugWorldBBox_M, m_dbgVars.debugWorldBBoxes, true);
		m_dbgVars.debugWorldBBoxes.clear();
	}

	if (m_world->cvars->r_showwaypoints.value) {
		DebugDrawActiveWaypoints();
	}

	if (m_world->cvars->r_showmovecmds.value) {
		DebugDrawFloorMoves();
	}

#endif
}

void WorldDraw::DrawOverlays() {
	for (ScreenOverlay::List::const_iterator it = m_overlays.begin(); it != m_overlays.end(); ++it)
		DrawOverlay(*(*it));
}

void WorldDraw::DrawUI() {
	m_world->uiRoot->Draw(0);
}

void WorldDraw::DrawViewBatches(ViewDef &view, bool wireframe) {
	if (!wireframe)
		m_counters.numMaterials += (int)view.batches.size();
	for (int i = 0; i < r::Material::kNumSorts; ++i)
		DrawViewBatches(view, (r::Material::Sort)i, wireframe);
}

void WorldDraw::DrawViewBatches(ViewDef &view, r::Material::Sort sort, bool wireframe) {
	for (details::MBatchIdMap::const_iterator it = view.batches.begin(); it != view.batches.end(); ++it) {
		if (it->second->matRef->mat->sort != sort)
			continue;
		DrawBatch(*it->second, wireframe);
	}
}

void WorldDraw::DrawBatch(const details::MBatch &batch, bool wireframe) {

	if (!batch.head)
		return;

	Vec3 pos;
	Vec3 angles;
#if defined(WORLD_DEBUG_DRAW)
	r::Material *mat = (wireframe) ? m_dbgVars.debugWireframe_M.mat : batch.matRef->mat;
#else
	r::Material *mat = batch.matRef->mat;
#endif
	bool first = true;

	mat->BindStates();
	if (!wireframe)
		mat->BindTextures(batch.matRef->loader);
	mat->shader->Begin(r::Shader::kPass_Default, *mat);

	for (details::MBatchDrawLink *link = batch.head; link; link = link->next) {
		MBatchDraw *draw = link->draw;

		bool tx = draw->GetTransform(pos, angles);
		if (tx)
			m_rb->PushMatrix(pos, draw->scale, angles);

		if (first && !wireframe) {
			first = false;
			++m_counters.numMaterials;
		}

		if (!wireframe)
			++m_counters.numBatches;

		draw->Bind(mat->shader.get().get());
		Shader::Uniforms u(draw->rgba.get());
		mat->shader->BindStates(u);
		m_rb->CommitStates();
		draw->CompileArrayStates(*mat->shader.get());
		draw->Draw();

		if (tx)
			m_rb->PopMatrix();
	}

	m_rb->ReleaseArrayStates();
	mat->shader->End();
}

void WorldDraw::DrawOverlay(ScreenOverlay &overlay) {
	if (overlay.alpha <= 0.f)
		return;

	const details::MatRef *matRef = overlay.m_mat;
	matRef->mat->BindStates();
	matRef->mat->BindTextures(matRef->loader);
	matRef->mat->shader->Begin(r::Shader::kPass_Default, *matRef->mat);
	m_rb->BindOverlay();
	Shader::Uniforms u(Vec4(1, 1, 1, overlay.alpha));
	matRef->mat->shader->BindStates(u);
	m_rb->CommitStates();
	m_rb->DrawOverlay();
	matRef->mat->shader->End();
}

void WorldDraw::AddScreenOverlay(ScreenOverlay &overlay) {
	m_overlays.push_back(&overlay);
	overlay.m_it = --(m_overlays.end());
}

void WorldDraw::RemoveScreenOverlay(ScreenOverlay &overlay) {
	m_overlays.erase(overlay.m_it);
}

void WorldDraw::PostProcess() {
	int num = 0;

	for (PostProcessEffect::Map::const_iterator it = m_postFX.begin(); it != m_postFX.end(); ++it) {
		const PostProcessEffect::Ref &fx = it->second;
		if (!fx->enabled)
			continue;
		++num;
	}

	for (PostProcessEffect::Map::const_iterator it = m_postFX.begin(); it != m_postFX.end(); ++it) {
		const PostProcessEffect::Ref &fx = it->second;
		if (!fx->enabled)
			continue;
		r::Material *m = fx->material;
		m->BindStates();
		m->BindTextures(fx->loader);
		m_rb->BindPostFXTargets(--num > 0);
		m->shader->Begin(r::Shader::kPass_Default, *m);
		m_rb->BindPostFXQuad();
		Shader::Uniforms u(fx->color.get());
		m->shader->BindStates(u);
		m_rb->CommitStates();
		m_rb->DrawPostFXQuad();
		m->shader->End();
	}
}

void WorldDraw::LinkEntity(Entity *entity, const BBox &bounds) {
	UnlinkEntity(entity);
}

void WorldDraw::UnlinkEntity(Entity *entity) {
}

void WorldDraw::LinkEntity(Entity *entity, const BBox &bounds, int nodeNum) {
}

void WorldDraw::LinkOccupant(MBatchOccupant *occupant, const BBox &bounds) {
	UnlinkOccupant(occupant);
}

void WorldDraw::UnlinkOccupant(MBatchOccupant *occupant) {
}

void WorldDraw::LinkOccupant(MBatchOccupant *occupant, const BBox &bounds, int nodeNum) {
}

} // world
