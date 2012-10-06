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

MBatch::MBatch() : mat(0) {
}

void MBatch::AddDraw(MBatchDraw &draw) {
	draws.push_back(&draw);
}

} // details

///////////////////////////////////////////////////////////////////////////////

Vec4 WorldDraw::MStaticWorldMeshBatch::s_rgba(Vec4(1, 1, 1, 1));
Vec3 WorldDraw::MStaticWorldMeshBatch::s_scale(Vec3(1, 1, 1));

WorldDraw::MStaticWorldMeshBatch::MStaticWorldMeshBatch(const r::Mesh::Ref &m, int matId) :
MBatchDraw(matId), m_m(m) {
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
m_wireframe(false), 
m_wireframeMat(0) {
	m_rb = RB_WorldDraw::New(w);
}

WorldDraw::~WorldDraw() {
	m_worldModels.clear();
	m_overlays.clear();
	m_rb.reset();
	m_refMats.clear();
	RAD_ASSERT(m_batchPool.NumUsedObjects() == 0);
	m_batchPool.Destroy();
}

int WorldDraw::LoadMaterials() {
	m_wireframeAsset = App::Get()->engine->sys->packages->Resolve("Sys/DebugWireframe_M", pkg::Z_Engine);
	if (!m_wireframeAsset || m_wireframeAsset->type != asset::AT_Material) {
		COut(C_Error) << "Error: Unable to load Sys/DebugWireframe_M." << std::endl;
		return pkg::SR_FileNotFound;
	}
	
	int r = m_wireframeAsset->Process(
		xtime::TimeSlice::Infinite,
		pkg::P_Load | pkg::P_FastPath
	);

	if (r != pkg::SR_Success) {
		COut(C_Error) << "Error: Unable to load Sys/DebugWireframe_M." << std::endl;
		return (r != pkg::SR_Pending) ? r : pkg::SR_MetaError;
	}

	asset::MaterialParser::Ref parser = asset::MaterialParser::Cast(m_wireframeAsset);
	if (!parser || !parser->valid) {
		COut(C_Error) << "Error: Unable to load Sys/DebugWireframe_M." << std::endl;
		return pkg::SR_MetaError;
	}

	m_wireframeMat = parser->material;

	return m_rb->LoadMaterials();
}

void WorldDraw::Init(const bsp_file::BSPFile::Ref &bsp) {

	m_batchPool.Create(ZWorld, "world-draw-batch", 64);

	int num = (int)bsp->numAreaNodes.get();
	m_nodes.reserve(num);
	for (int i = 0; i < num; ++i) {
		const bsp_file::BSPAreaNode *x = bsp->AreaNodes() + i;
		dBSPAreaNode n;
		n.parent = (int)x->parent;
		n.children[0] = x->children[0];
		n.children[1] = x->children[1];
		n.planenum = x->planenum;
		n.bounds.Initialize(x->mins[0], x->mins[1], x->mins[2], x->maxs[0], x->maxs[1], x->maxs[2]);
		m_nodes.push_back(n);
	}

	num = (int)bsp->numAreaLeafs.get();
	m_leafs.reserve(num);
	for (int i = 0; i < num; ++i) {
		const bsp_file::BSPAreaLeaf *x = bsp->AreaLeafs() + i;
		dBSPAreaLeaf l;
		l.firstModel = x->firstModel;
		l.numModels = x->numModels;
		l.bounds.Initialize(x->mins[0], x->mins[1], x->mins[2], x->maxs[0], x->maxs[1], x->maxs[2]);
		m_leafs.push_back(l);
	}
}

int WorldDraw::Precache() {
	int r = m_rb->Precache();
	if (r != pkg::SR_Success)
		return r;

	// gather/sort all materials into batches array.
	
	for (dBSPAreaLeaf::Vec::const_iterator it = m_leafs.begin(); it != m_leafs.end(); ++it) {
		const dBSPAreaLeaf &leaf = *it;

		// add static meshes
		for (int i = 0; i < leaf.numModels; ++i) {
			RAD_ASSERT((i+leaf.firstModel) < (int)m_worldModels.size());
			const MStaticWorldMeshBatch::Ref &m = m_worldModels[i + leaf.firstModel];
			details::MBatchRef batch = AddMaterialRef(m->m_matId);
			batch->draws.push_back(m.get());
		}

		// add any occupants.
		for (EntityPtrSet::const_iterator it = leaf.occupants.begin(); it != leaf.occupants.end(); ++it) {
			Entity *e = *it;

			const DrawModel::Map &models = e->models;
			for (DrawModel::Map::const_iterator it2 = models.begin(); it2 != models.end(); ++it2) {
				const DrawModel::Ref &model = it2->second;

				for (MBatchDraw::RefVec::const_iterator it3 = model->m_batches.begin(); it3 != model->m_batches.end(); ++it3) {
					const MBatchDraw::Ref &draw = *it3;
					
					details::MBatchRef batch = AddMaterialRef(draw->m_matId);
					batch->draws.push_back(draw.get());
				}
			}
		}
	}

	// Precache materials & meshes.
	ScreenOverlay::Ref overlay;

	for (details::MBatchIdMap::iterator it = m_refMats.begin(); it != m_refMats.end(); ++it) {
		details::MBatch &batch = *it->second;

		if (batch.draws.empty()) { 
			// make sure texures get precached (i.e. UI materials may be in here)
			if (overlay)
				overlay->m_mbatch = it->second;
			else
				overlay = CreateScreenOverlay(batch.asset->id);

			RAD_ASSERT(overlay);
			DrawOverlay(*overlay);
		} else {
			batch.mat->BindStates();
			batch.mat->BindTextures(batch.loader);
			batch.mat->shader->Begin(r::Shader::P_Default, *batch.mat);

			for (MBatchDrawPtrVec::const_iterator it = batch.draws.begin(); it != batch.draws.end(); ++it) {
				MBatchDraw *draw = *it;

				draw->Bind(batch.mat->shader.get().get());
				batch.mat->shader->BindStates(true, draw->rgba);
				m_rb->CommitStates();
				draw->CompileArrayStates(*batch.mat->shader.get());
				draw->Draw();		
			}

			m_rb->ReleaseArrayStates();
			batch.mat->shader->End();
		}

		batch.draws.clear();
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

	for (details::MBatchIdMap::const_iterator it = m_refMats.begin(); it != m_refMats.end(); ++it)
		it->second->mat->Sample(m_world->time, dt);

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

details::MBatchRef WorldDraw::AddMaterialRef(int id) {
	details::MBatchIdMap::iterator it = m_refMats.find(id);
	if (it != m_refMats.end())
		return it->second;

	pkg::Asset::Ref asset = App::Get()->engine->sys->packages->Asset(id, pkg::Z_Engine);
	if (!asset)
		return details::MBatchRef();

	asset::MaterialParser::Ref parser = asset::MaterialParser::Cast(asset);
	RAD_VERIFY(parser);

	details::MBatchRef b(AllocateBatch());
	b->asset = asset;
	b->mat = parser->material;
	b->loader = asset::MaterialLoader::Cast(asset);
	RAD_VERIFY(b->loader);

	m_refMats.insert(details::MBatchIdMap::value_type(id, b));
	return b;
}

details::MBatchRef WorldDraw::AddViewBatch(ViewDef &view, int id) {
	details::MBatchIdMap::iterator it = view.batches.find(id);
	if (it != view.batches.end())
		return it->second;

	details::MBatchRef src = AddMaterialRef(id);

	details::MBatchRef b(AllocateBatch());
	b->asset = src->asset;
	b->mat = src->mat;
	b->loader = src->loader;

	view.batches.insert(details::MBatchIdMap::value_type(id, b));
	return b;
}

bool WorldDraw::AddMaterial(int id) {
	return AddMaterialRef(id);
}

void WorldDraw::RemoveMaterial(int id) {
	m_refMats.erase(id);
}

ScreenOverlay::Ref WorldDraw::CreateScreenOverlay(int matId)
{
	details::MBatchRef mbatch = AddMaterialRef(matId);
	if (!mbatch)
		return ScreenOverlay::Ref();
	return ScreenOverlay::Ref(new (ZWorld) ScreenOverlay(this, mbatch));
}

void WorldDraw::AddStaticWorldMesh(const r::Mesh::Ref &m, int matId) {
	RAD_ASSERT(m);
	MStaticWorldMeshBatch::Ref r(new (ZWorld) MStaticWorldMeshBatch(m, matId));
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

	view.bspLeaf = m_world->LeafForPoint(view.camera.pos);

	if (view.bspLeaf && (view.bspLeaf->area > -1)) {
		view.areaLeaf = LeafForPoint(view.camera.pos, view.bspLeaf->area);
	} else {
		view.areaLeaf = 0;
	}
}

void WorldDraw::SetupFrustumPlanes(ViewDef &view) {
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

void WorldDraw::VisMark(ViewDef &view, int nodeNum) {

	RAD_ASSERT(view.areaLeaf);

	if (nodeNum < 0) {
		nodeNum = -(nodeNum + 1);
		const Leaf &leaf = m_leafs[nodeNum];
		if (!leaf.numModels)
			return;

		++m_counters.testedLeafs;
		if (!ClipBounds(view, leaf.bounds))
			return;
		++m_counters.drawnLeafs;

		for (int i = 0; i < leaf.numModels; ++i)
			m_worldModels[i+leaf.firstModel]->Mark(m_frame);

		return;
	}

	const Node &node = m_nodes[nodeNum];

	++m_counters.testedNodes;

	if (!ClipBounds(view, node.bounds))
		return;

	++m_counters.drawnNodes;

	VisMark(view, node.children[0]);
	VisMark(view, node.children[1]);
}

bool WorldDraw::ClipBounds(const ViewDef &view, const BBox &bounds) {
	if (bounds.Contains(view.camera.pos, 1.f))
		return true;

	for (int i = 0; i < ViewDef::kNumFrustumPlanes; ++i) {
		Plane::SideType side = view.frustum[i].Side(bounds, 0.f);
		if (side == Plane::Back)
			return false; // clipped away.
	}

	// TODO: better frustum clipping
	return true;
}

void WorldDraw::DrawView() {
	++m_frame;
	
	ViewDef view;

	FindViewArea(view);
	SetupFrustumPlanes(view);

	if (m_nodes.empty())
		VisMark(view, -1); // one leaf
	else
		VisMark(view, 0);

	m_rb->RotateForView(view);
	m_rb->SetWorldStates();

	m_rb->numTris = 0;

	DrawBatches(view, false);

	m_counters.numTris += m_rb->numTris;

	if (m_wireframe) {
		m_rb->wireframe = true;
		DrawBatches(view, true);
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

void WorldDraw::DrawBatches(ViewDef &view, bool wireframe) {
	for (int i = r::Material::S_Solid; i < r::Material::NumSorts; ++i)
		DrawBatches(view, (r::Material::Sort)i, wireframe);
}

void WorldDraw::DrawBatches(ViewDef &view, r::Material::Sort sort, bool wireframe) {
	for (details::MBatchIdMap::const_iterator it = view.batches.begin(); it != view.batches.end(); ++it) {
		if (it->second->mat->sort != sort)
			continue;
		DrawBatch(*it->second, wireframe);
	}
}

void WorldDraw::DrawBatch(const details::MBatch &batch, bool wireframe) {

	if (batch.draws.empty())
		return;

	Vec3 pos;
	Vec3 angles;
	r::Material *mat = (wireframe) ? m_wireframeMat : batch.mat;
	bool first = true;

	mat->BindStates();
	if (!wireframe)
		mat->BindTextures(batch.loader);
	mat->shader->Begin(r::Shader::P_Default, *mat);

	for (MBatchDrawPtrVec::const_iterator it = batch.draws.begin(); it != batch.draws.end(); ++it) {
		MBatchDraw *draw = *it;
		
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

	const details::MBatchRef &mbatch = overlay.mbatch;
	mbatch->mat->BindStates();
	mbatch->mat->BindTextures(mbatch->loader);
	mbatch->mat->shader->Begin(r::Shader::P_Default, *mbatch->mat);
	m_rb->BindOverlay();
	Vec4 c(1, 1, 1, overlay.alpha);	
	mbatch->mat->shader->BindStates(true, c);
	m_rb->CommitStates();
	m_rb->DrawOverlay();
	mbatch->mat->shader->End();
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

	if (entity->m_leaf->area < 0)
		return;
	
	entity->m_areaLeafs.reserve(8);
	if (m_nodes.empty()) {
		LinkEntity(entity, bounds, -1);
	} else {
		StackWindingVec bbox;
		m_world->BoundWindings(bounds, bbox);

		AreaBits visibleAreas;
		m_world->OccupantVolumeCanSeeArea(
			entity->ps->worldPos,
			bbox,
			0,
			entity->m_leaf->area,
			-1,
			visibleAreas
		);

		// NOTE: this sort of sucks, figure out a faster way of doing this.
		for (int i = 0; kMaxAreas; ++i) {
			if (visibleAreas.test(i)) {
				const bsp_file::BSPArea *area = m_world->m_bsp->Areas() + i;
				LinkEntity(entity, bounds, area->rootNode);
			}
		}
	}
}

void WorldDraw::UnlinkEntity(Entity *entity) {
	
	for (dBSPAreaLeaf::PtrVec::const_iterator it = entity->m_areaLeafs.begin(); it != entity->m_areaLeafs.end(); ++it) {
		dBSPAreaLeaf *leaf = *it;
		leaf->occupants.erase(entity);
	}

	entity->m_areaLeafs.clear();
}

void WorldDraw::LinkEntity(Entity *entity, const BBox &bounds, int nodeNum) {
	if (nodeNum < 0) {
		nodeNum = -(nodeNum + 1);
		RAD_ASSERT(nodeNum < (int)m_leafs.size());
		dBSPAreaLeaf &leaf = m_leafs[nodeNum];
		leaf.occupants.insert(entity);
		entity->m_areaLeafs.push_back(&leaf);
		return;
	}

	RAD_ASSERT(nodeNum < (int)m_nodes.size());
	const dBSPAreaNode &node = m_nodes[nodeNum];
	RAD_ASSERT(node.planenum < (int)m_planes.size());
	const Plane &p = m_world->m_planes[node.planenum];

	Plane::SideType side = p.Side(bounds, 0.0f);
	
	if ((side == Plane::Cross) || (side == Plane::Front))
		LinkEntity(entity, bounds, node.children[0]);
	if ((side == Plane::Cross) || (side == Plane::Back))
		LinkEntity(entity, bounds, node.children[1]);
}

dBSPAreaLeaf *WorldDraw::LeafForPoint(const Vec3 &pos, int areaNum) {
	if (m_nodes.empty())
		return &m_leafs[0];
	RAD_ASSERT(area < (int)m_world->m_bsp->numAreaNodes.get());
	const bsp_file::BSPArea *area = m_world->m_bsp->Areas() + areaNum;
	return LeafForPoint(pos, area->rootNode);
}

dBSPAreaLeaf *WorldDraw::LeafForPoint_r(const Vec3 &pos, int nodeNum) {
	if (nodeNum < 0) {
		nodeNum = -(nodeNum + 1);
		RAD_ASSERT(nodeNum < (int)m_leafs.size());
		dBSPAreaLeaf &leaf = m_leafs[nodeNum];
		return &leaf;
	}

	RAD_ASSERT(nodeNum < (int)m_nodes.size());
	dBSPAreaNode &node = m_nodes[nodeNum];

	const Plane &plane = m_world->m_planes[node.planenum];
	Plane::SideType side = plane.Side(pos, 0.f);

	if (side == Plane::Back)
		return LeafForPoint_r(pos, node.children[1]);
	return LeafForPoint_r(pos, node.children[0]);
}

} // world
