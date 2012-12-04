// WorldDraw.cpp
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "../App.h"
#include "../Engine.h"
#include "World.h"
#include "../Game/Game.h"
#include "ScreenOverlay.h"
#include "../Renderer/Shader.h"
#include "../Packages/Packages.h"
#include "../UI/UIWidget.h"
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
	drawnLeafs = 0;
	testedLeafs = 0;
	drawnNodes = 0;
	testedNodes = 0;
	numModels = 0;
	numTris = 0;
	numMaterials = 0;
}

WorldDraw::WorldDraw(World *w) : 
m_world(w), 
m_frame(-1), 
m_markFrame(-1),
m_wireframe(false),
m_init(false) {
	m_rb = RB_WorldDraw::New(w);
}

WorldDraw::~WorldDraw() {
	m_worldModels.clear();
	m_overlays.clear();
	m_rb.reset();
	m_refMats.clear();

	if (m_init) {
		RAD_ASSERT(m_batchPool.NumUsedObjects() == 0);
		m_batchPool.Destroy();
		RAD_ASSERT(m_linkPool.NumUsedObjects() == 0);
		m_linkPool.Destroy();
	}
}

int WorldDraw::LoadMaterials() {
	
	int r = LoadMaterial("Sys/DebugWireframe_M", m_debugWireframe);
	if (r != pkg::SR_Success)
		return r;

	r = LoadMaterial("Sys/DebugPortalEdge_M", m_debugPortal[0]);
	if (r != pkg::SR_Success)
		return r;

	r = LoadMaterial("Sys/DebugPortal_M", m_debugPortal[1]);
	if (r != pkg::SR_Success)
		return r;

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

	asset::MaterialParser::Ref parser = asset::MaterialParser::Cast(mat.asset);
	if (!parser || !parser->valid) {
		COut(C_Error) << "Error: Unable to load '" << name << "'." << std::endl;
		return pkg::SR_MetaError;
	}

	mat.mat = parser->material;
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

	asset::MaterialParser::Ref parser = asset::MaterialParser::Cast(asset);
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

	for (PostProcessEffect::Map::const_iterator it = m_postFX.begin(); it != m_postFX.end(); ++it) {
		if (it->second->enabled) {
			postFX = true;
			break;
		}
	}

	m_rb->BeginFrame();

	if (postFX)
		m_rb->BindRenderTarget();

	m_rb->ClearBackBuffer();
	m_rb->SetPerspectiveMatrix();
	DrawView();
	m_rb->SetScreenLocalMatrix();
	DrawOverlays();

	if (postFX)
		PostProcess();

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
		normals[i].Normalize();

		if (i == 1 && ViewDef::kNumFrustumPlanes == 6) {
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
		view.areas
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

	for (int i = 0; i < area.numModels; ++i) {
		U16 modelNum = *(m_world->m_bsp->ModelIndices() + i + area.firstModel);
		RAD_ASSERT(modelNum < (U16)m_worldModels.size());

		const MStaticWorldMeshBatch::Ref &m = m_worldModels[modelNum];
		if (m->m_markFrame != m_markFrame) {
			if (ClipBounds(volume, volumeBounds, m->bounds)) {
				m->m_markFrame = m_markFrame;
				details::MBatchRef batch = AddViewBatch(view, m->m_matId);
				if (batch)
					batch->AddDraw(*m);
			}
		}
	}

	// add entities.

	for (EntityPtrSet::const_iterator it = area.occupants.begin(); it != area.occupants.end(); ++it) {
		Entity *e = *it;
		if (e->m_markFrame != m_markFrame) {
			e->m_markFrame = m_markFrame;
			for (DrawModel::Map::const_iterator it = e->models->begin(); it != e->models->end(); ++it) {
				const DrawModel::Ref &m = it->second;
				if (m->m_markFrame != m_markFrame) {
					m->m_markFrame = m_markFrame;
					for (MBatchDraw::RefVec::const_iterator it = m->m_batches.begin(); it != m->m_batches.end(); ++it) {
						const MBatchDraw::Ref &draw = *it;
						if (draw->m_markFrame != m_markFrame) {
							draw->m_markFrame = m_markFrame;
							details::MBatchRef batch = AddViewBatch(view, draw->m_matId);
							if (batch)
								batch->AddDraw(*draw);
						}
					}
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

#if defined(RAD_OPT_PC)
//	DebugDrawPortals(view);
#endif

	if (m_wireframe) {
		m_rb->wireframe = true;
		DrawViewBatches(view, true);
		m_rb->wireframe = false;
	}
}

void WorldDraw::DrawOverlays() {
	for (ScreenOverlay::List::const_iterator it = m_overlays.begin(); it != m_overlays.end(); ++it)
		DrawOverlay(*(*it));
}

void WorldDraw::DrawUI() {
	m_world->uiRoot->Draw();
}

void WorldDraw::DrawViewBatches(ViewDef &view, bool wireframe) {
	for (int i = r::Material::S_Solid; i < r::Material::NumSorts; ++i)
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
	r::Material *mat = (wireframe) ? m_debugWireframe.mat : batch.matRef->mat;
	bool first = true;

	mat->BindStates();
	if (!wireframe)
		mat->BindTextures(batch.matRef->loader);
	mat->shader->Begin(r::Shader::P_Default, *mat);

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
			++m_counters.numModels;

		draw->Bind(mat->shader.get().get());
		mat->shader->BindStates(true, draw->rgba);
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
	matRef->mat->shader->Begin(r::Shader::P_Default, *matRef->mat);
	m_rb->BindOverlay();
	Vec4 c(1, 1, 1, overlay.alpha);	
	matRef->mat->shader->BindStates(true, c);
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
		m->shader->Begin(r::Shader::P_Default, *m);
		m_rb->BindPostFXQuad();
		m->shader->BindStates(true, fx->color);
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

#if !defined(RAD_OPT_SHIP)
void WorldDraw::DebugDrawPortals(ViewDef &view) {
	for (int i = 0; i < kMaxAreas; ++i) {
		if (view.areas.test(i))
			DebugDrawAreaportals(i);
	}
}

void WorldDraw::DebugDrawAreaportals(int areaNum) { 

	RAD_ASSERT(areaNum < (int)m_world->m_areas.size());

	for (int style = 0; style < 2; ++style) {
		m_debugPortal[style].mat->BindStates();
		m_debugPortal[style].mat->BindTextures(asset::MaterialLoader::Ref());
		m_debugPortal[style].mat->shader->Begin(r::Shader::P_Default, *m_debugPortal[style].mat);


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

			m_debugPortal[style].mat->shader->BindStates(true);
			m_rb->CommitStates();

			if (style == 0) {
				m_rb->DebugDrawLineLoop(portal.winding.NumVertices());
			} else {
				m_rb->DebugDrawIndexedTris(numIndices);
			}
		}

		m_debugPortal[style].mat->shader->End();
	}
}

#endif

} // world
