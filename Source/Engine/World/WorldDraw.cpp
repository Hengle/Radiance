// WorldDraw.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
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

MBatch::MBatch() : mat(0)
{
}

void MBatch::AddDraw(MBatchDraw &draw)
{
	if (draw.m_batch)
		draw.m_batch->RemoveDraw(draw);

	draws.push_back(&draw);
	draw.m_it = --(draws.end());
	draw.m_batch = this;
}

void MBatch::RemoveDraw(MBatchDraw &draw)
{
	draws.erase(draw.m_it);
	draw.m_batch = 0;
}

} // details

MBatchDraw::~MBatchDraw()
{
	if (m_batch)
		m_batch->RemoveDraw(*this);
}

///////////////////////////////////////////////////////////////////////////////

Vec4 WorldDraw::MStaticWorldMeshBatch::s_rgba(Vec4(1, 1, 1, 1));
Vec3 WorldDraw::MStaticWorldMeshBatch::s_scale(Vec3(1, 1, 1));

WorldDraw::MStaticWorldMeshBatch::MStaticWorldMeshBatch(const r::Mesh::Ref &m, int matId) :
MBatchDraw(matId), m_m(m), m_frame(-1)
{
}

void WorldDraw::MStaticWorldMeshBatch::Bind(r::Shader *shader)
{
	m_m->BindAll(shader);
}

void WorldDraw::MStaticWorldMeshBatch::CompileArrayStates(r::Shader &shader)
{
	m_m->CompileArrayStates(shader);
}

void WorldDraw::MStaticWorldMeshBatch::FlushArrayStates(r::Shader *shader)
{
	m_m->FlushArrayStates(shader);
}

void WorldDraw::MStaticWorldMeshBatch::Draw()
{
	m_m->Draw();
}

///////////////////////////////////////////////////////////////////////////////

void WorldDraw::Counters::Clear()
{
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
m_wireframeMat(0)
{
	m_rb = RB_WorldDraw::New(w);
}

WorldDraw::~WorldDraw()
{
	m_drawRefs.clear();
}

int WorldDraw::LoadMaterials()
{
	m_wireframeAsset = App::Get()->engine->sys->packages->Resolve("Sys/DebugWireframe_M", pkg::Z_Engine);
	if (!m_wireframeAsset || m_wireframeAsset->type != asset::AT_Material)
	{
		COut(C_Error) << "Error: Unable to load Sys/DebugWireframe_M." << std::endl;
		return pkg::SR_FileNotFound;
	}
	
	int r = m_wireframeAsset->Process(
		xtime::TimeSlice::Infinite,
		pkg::P_Load | pkg::P_FastPath
	);

	if (r != pkg::SR_Success)
	{
		COut(C_Error) << "Error: Unable to load Sys/DebugWireframe_M." << std::endl;
		return (r != pkg::SR_Pending) ? r : pkg::SR_MetaError;
	}

	asset::MaterialParser::Ref parser = asset::MaterialParser::Cast(m_wireframeAsset);
	if (!parser || !parser->valid)
	{
		COut(C_Error) << "Error: Unable to load Sys/DebugWireframe_M." << std::endl;
		return pkg::SR_MetaError;
	}

	m_wireframeMat = parser->material;

	return m_rb->LoadMaterials();
}

void WorldDraw::Init(const bsp_file::BSPFile::Ref &bsp)
{
	int num = (int)bsp->numPlanes.get();
	m_planes.reserve(num);
	for (int i = 0; i < num; ++i)
	{
		const bsp_file::BSPPlane *x = bsp->Planes() + i;
		Plane pl(x->p[0], x->p[1], x->p[2], x->p[3]);
		m_planes.push_back(pl);
	}

	num = (int)bsp->numNodes.get();
	m_nodes.reserve(num);
	for (int i = 0; i < num; ++i)
	{
		const bsp_file::BSPNode *x = bsp->Nodes() + i;
		Node n;
		n.parent = (int)x->parent;
		n.children[0] = x->children[0];
		n.children[1] = x->children[1];
		n.planenum = x->planenum;
		n.bounds.Initialize(x->mins[0], x->mins[1], x->mins[2], x->maxs[0], x->maxs[1], x->maxs[2]);
		m_nodes.push_back(n);
	}

	num = (int)bsp->numLeafs.get();
	m_leafs.reserve(num);
	for (int i = 0; i < num; ++i)
	{
		const bsp_file::BSPLeaf *x = bsp->Leafs() + i;
		Leaf l;
		l.parent = (int)x->parent;
		l.bounds.Initialize(x->mins[0], x->mins[1], x->mins[2], x->maxs[0], x->maxs[1], x->maxs[2]);
		m_leafs.push_back(l);
	}
}

int WorldDraw::Precache()
{
	int r = m_rb->Precache();
	if (r != pkg::SR_Success)
		return r;

	// Precache materials & meshes.
	ScreenOverlay::Ref overlay;

	for (details::MBatchIdMap::const_iterator it = m_batches.begin(); it != m_batches.end(); ++it)
	{
		const details::MBatch &batch = *it->second;

		if (batch.draws.empty())
		{ // make sure texures get precached (i.e. UI materials may be in here)
			if (overlay)
				overlay->m_mbatch = it->second;
			else
				overlay = CreateScreenOverlay(batch.asset->id);

			RAD_ASSERT(overlay);
			DrawOverlay(*overlay);
		}
		else
		{
			batch.mat->BindStates();
			batch.mat->BindTextures(batch.loader);
			batch.mat->shader->Begin(r::Shader::P_Default, *batch.mat);

			for (MBatchDraw::PtrList::const_iterator it = batch.draws.begin(); it != batch.draws.end(); ++it)
			{
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
	}

	m_rb->Finish();

	return pkg::SR_Success;
}

void WorldDraw::AddEffect(int id, const PostProcessEffect::Ref &fx)
{
	m_postFX[id] = fx;
	if (fx)
		AddMaterial(fx->asset->id);
}

void WorldDraw::Tick(float dt)
{
	// tick materials.
	for (details::MBatchIdMap::const_iterator it = m_batches.begin(); it != m_batches.end(); ++it)
		it->second->mat->Sample(m_world->time, dt);

	for (ScreenOverlay::List::const_iterator it = m_overlays.begin(); it != m_overlays.end(); ++it)
		(*it)->Tick(dt);

	for (PostProcessEffect::Map::const_iterator it = m_postFX.begin(); it != m_postFX.end(); ++it)
		it->second->Tick(dt);

	for (ViewModel::Map::const_iterator it = m_viewModels.begin(); it != m_viewModels.end(); ++it)
		it->second->Tick(m_world->time, dt);
}

details::MBatchRef WorldDraw::AddMaterialBatch(int id)
{
	details::MBatchIdMap::iterator it = m_batches.find(id);
	if (it != m_batches.end())
		return it->second;

	pkg::Asset::Ref asset = App::Get()->engine->sys->packages->Asset(id, pkg::Z_Engine);
	if (!asset)
		return details::MBatchRef();

	asset::MaterialParser::Ref parser = asset::MaterialParser::Cast(asset);
	RAD_VERIFY(parser);

	details::MBatchRef b(new (ZWorld) details::MBatch());
	b->asset = asset;
	b->mat = parser->material;
	b->loader = asset::MaterialLoader::Cast(asset);
	RAD_VERIFY(b->loader);

	m_batches.insert(details::MBatchIdMap::value_type(id, b));
	return b;
}

void WorldDraw::SwapMaterial(int src, int dst)
{
	details::MBatchRef batch = AddMaterialBatch(src);

	pkg::Asset::Ref asset = App::Get()->engine->sys->packages->Asset(dst, pkg::Z_Engine);
	if (!asset)
		return;

	asset::MaterialParser::Ref parser = asset::MaterialParser::Cast(asset);
	RAD_VERIFY(parser);

	batch->asset = asset;
	batch->mat = parser->material;
	batch->loader = asset::MaterialLoader::Cast(asset);
	RAD_VERIFY(batch->loader);
}

bool WorldDraw::AddMaterial(int id)
{
	return AddMaterialBatch(id);
}

void WorldDraw::RemoveMaterial(int id)
{
	m_batches.erase(id);
}

bool WorldDraw::AddBatch(const MBatchDraw::Ref &draw, bool worldRef)
{
	RAD_ASSERT(draw);

	details::MBatchRef batch = AddMaterialBatch(draw->matId);
	if (!batch)
		return false;
	
	batch->AddDraw(*draw);
	
	if (worldRef)
		m_drawRefs.push_back(draw);
	
	return true;
}

void WorldDraw::AttachViewModel(const ViewModel::Ref &ref)
{
	m_viewModels[ref.get()] = ref;
}

void WorldDraw::RemoveViewModel(const ViewModel::Ref &ref)
{
	m_viewModels.erase(ref.get());
}

ScreenOverlay::Ref WorldDraw::CreateScreenOverlay(int matId)
{
	details::MBatchRef mbatch = AddMaterialBatch(matId);
	if (!mbatch)
		return ScreenOverlay::Ref();
	return ScreenOverlay::Ref(new (ZWorld) ScreenOverlay(this, mbatch));
}

bool WorldDraw::AddStaticWorldMesh(const r::Mesh::Ref &m, int matId)
{
	RAD_ASSERT(m);
	MStaticWorldMeshBatch::Ref r(new (ZWorld) MStaticWorldMeshBatch(m, matId));
	m_worldModels.push_back(r);
	return AddBatch(boost::static_pointer_cast<MBatchDraw>(r), true);
}

void WorldDraw::Draw(Counters *counters)
{
	m_counters.Clear();

	bool postFX = false;

	for (PostProcessEffect::Map::const_iterator it = m_postFX.begin(); it != m_postFX.end(); ++it)
	{
		if (it->second->enabled)
		{
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
	DrawViewModels();
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

void WorldDraw::SetupFrustumPlanes()
{
	int vpx, vpy, vpw, vph;
	m_world->game->Viewport(vpx, vpy, vpw, vph);
	float yaspect = ((float)vph/(float)vpw);
	float xfov = math::DegToRad(m_world->camera->fov.get()) * 0.5f;
	float yfov = xfov * yaspect;
	const Vec3 &fwd = m_world->camera->fwd;
	const Vec3 &left = m_world->camera->left;
	const Vec3 &up = m_world->camera->up;

	float s,c;
	Vec3 normals[NumFrustumPlanes];
	int idx = 0;

	if (NumFrustumPlanes > 4)
	{
		normals[idx++] = fwd;
	}

	if (NumFrustumPlanes > 5)
	{
		normals[idx++] = -fwd;
	}
	
	math::SinAndCos(&s, &c, xfov);

	normals[idx++] = fwd * s + left * c;
	normals[idx++] = fwd * s + left * -c;

	math::SinAndCos(&s, &c, yfov);
	
	normals[idx++] = fwd * s + up * c;
	normals[idx++] = fwd * s + up * -c;

	for (int i = 0; i < NumFrustumPlanes; ++i)
	{
		Plane &p = m_frustum[i];
		normals[i].Normalize();

		if (i == 1 && NumFrustumPlanes == 6)
		{
			Vec3 t = normals[0] * m_world->camera->farClip.get();
			p.Initialize(normals[i], m_viewPos + t, Plane::Unit);
		}
		else
		{
			p.Initialize(normals[i], m_viewPos, Plane::Unit);
		}
	}
}

void WorldDraw::VisMark(int nodeIdx)
{
	if (nodeIdx < 0)
	{
		nodeIdx = -(nodeIdx + 1);
		const Leaf &leaf = m_leafs[nodeIdx];
		if (!leaf.numModels)
			return;

		++m_counters.testedLeafs;
		if (!ClipBounds(leaf.bounds))
			return;
		++m_counters.drawnLeafs;

		for (int i = 0; i < leaf.numModels; ++i)
			m_worldModels[i+leaf.firstModel]->Mark(m_frame);

		return;
	}

	const Node &node = m_nodes[nodeIdx];

	++m_counters.testedNodes;

	if (!ClipBounds(node.bounds))
		return;

	++m_counters.drawnNodes;

	VisMark(node.children[0]);
	VisMark(node.children[1]);
}

bool WorldDraw::ClipBounds(const BBox &bounds)
{
	if (bounds.Contains(m_viewPos, 1.f))
		return true;

	int sides[NumFrustumPlanes];
	int planebits = 0;

	for (int i = 0; i < NumFrustumPlanes; ++i)
	{
		sides[i] = BoundsOnPlaneSide(bounds, m_frustum[i]);
		if (sides[i] == 2) // clipped away
			return false;
		if (sides[i] == 3) // cross
			planebits |= (1<<i);
	}

	// TODO: better frustum clipping
	return true;
}

int WorldDraw::BoundsOnPlaneSide(const BBox &bounds, const Plane &p)
{
	const Vec3 &mins = bounds.Mins();
	const Vec3 &maxs = bounds.Maxs();

	Vec3 mn, mx;

	// Get min/max corner

	for (int i = 0; i < 3; ++i)
	{
		if (p.Normal()[i] >= 0.f)
		{
			mn[i] = mins[i];
			mx[i] = maxs[i];
		}
		else
		{
			mn[i] = maxs[i];
			mx[i] = mins[i];
		}
	}

	float mnd = p.Distance(mn);
	float mxd = p.Distance(mx);

	if (mnd > 0.1f)
		return 1; // front
	if (mxd < -0.1f)
		return 2; // back;
	if (mxd > 0.1f)
	{
		if (mnd < -0.1f)
			return 3; // cross
		return 1; // front
	}
	if (mnd < -0.1f)
		return 2; // back
	return 3; // both
}

void WorldDraw::DrawView()
{
	++m_frame;
	m_viewPos = m_world->camera->pos;
	SetupFrustumPlanes();

	if (m_nodes.empty())
		VisMark(-1); // one leaf
	else
		VisMark(0);

	m_rb->RotateForCamera();
	m_rb->SetWorldStates();

	m_rb->numTris = 0;

	DrawBatches(true, false);

	m_counters.numTris += m_rb->numTris;

	if (m_wireframe)
	{
		m_rb->wireframe = true;
		DrawBatches(true, true);
		m_rb->wireframe = false;
	}
}

void WorldDraw::DrawViewModels()
{
	m_rb->RotateForCameraBasis();
	m_rb->SetWorldStates();

	m_rb->numTris = 0;

	DrawBatches(false, false);

	m_counters.numTris += m_rb->numTris;

	if (m_wireframe)
	{
		m_rb->wireframe = true;
		DrawBatches(false, true);
		m_rb->wireframe = false;
	}
}

void WorldDraw::DrawOverlays()
{
	for (ScreenOverlay::List::const_iterator it = m_overlays.begin(); it != m_overlays.end(); ++it)
		DrawOverlay(*(*it));
}

void WorldDraw::DrawUI()
{
	m_world->uiRoot->Draw();
}

void WorldDraw::DrawBatches(bool xform, bool wireframe)
{
	for (int i = r::Material::S_Solid; i < r::Material::NumSorts; ++i)
		DrawBatches((r::Material::Sort)i, xform, wireframe);
}

void WorldDraw::DrawBatches(r::Material::Sort sort, bool xform, bool wireframe)
{
	for (details::MBatchIdMap::const_iterator it = m_batches.begin(); it != m_batches.end(); ++it)
	{
		if (it->second->mat->sort != sort)
			continue;
		DrawBatch(*it->second, xform, wireframe);
	}
}

void WorldDraw::DrawBatch(const details::MBatch &batch, bool xform, bool wireframe)
{
	{
		int numDraws=0;
		for (MBatchDraw::PtrList::const_iterator it = batch.draws.begin(); it != batch.draws.end(); ++it)
		{
			MBatchDraw *draw = *it;
			if (!draw->VisMarked(m_frame))
				continue;
			if (draw->xform != xform)
				continue;
			++numDraws;
		}

		if (numDraws == 0)
			return;
	}

	Vec3 pos;
	Vec3 angles;
	r::Material *mat = (wireframe) ? m_wireframeMat : batch.mat;
	bool first = true;

	mat->BindStates();
	if (!wireframe)
		mat->BindTextures(batch.loader);
	mat->shader->Begin(r::Shader::P_Default, *mat);

	for (MBatchDraw::PtrList::const_iterator it = batch.draws.begin(); it != batch.draws.end(); ++it)
	{
		MBatchDraw *draw = *it;
		
		if (!draw->VisMarked(m_frame))
			continue;
		if (xform != draw->xform)
			continue;

		bool tx = draw->GetTransform(pos, angles);
		if (tx)
			m_rb->PushMatrix(pos, draw->scale, angles);

		if (first && !wireframe)
		{
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

void WorldDraw::DrawOverlay(ScreenOverlay &overlay)
{
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

void WorldDraw::AddScreenOverlay(ScreenOverlay &overlay)
{
	m_overlays.push_back(&overlay);
	overlay.m_it = --(m_overlays.end());
}

void WorldDraw::RemoveScreenOverlay(ScreenOverlay &overlay)
{
	m_overlays.erase(overlay.m_it);
}

void WorldDraw::PostProcess()
{
	int num = 0;

	for (PostProcessEffect::Map::const_iterator it = m_postFX.begin(); it != m_postFX.end(); ++it)
	{
		const PostProcessEffect::Ref &fx = it->second;
		if (!fx->enabled)
			continue;
		++num;
	}

	for (PostProcessEffect::Map::const_iterator it = m_postFX.begin(); it != m_postFX.end(); ++it)
	{
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

} // world
