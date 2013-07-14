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

int LoadMaterial(const char *name, asset::MaterialBundle &mat) {
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

	mat.material = parser->material;
	mat.loader = asset::MaterialLoader::Cast(mat.asset);
	return pkg::SR_Success;
}

} // details

///////////////////////////////////////////////////////////////////////////////

int RB_WorldDraw::LoadMaterial(const char *name, asset::MaterialBundle &mat) {
	return details::LoadMaterial(name, mat);
}

///////////////////////////////////////////////////////////////////////////////

MBatchDraw::MBatchDraw(WorldDraw &draw, int matId, const void *uid) : 
m_matId(matId), 
m_markFrame(-1), 
m_visibleFrame(-1), 
m_interactions(0),
m_uid(uid) {
	m_matRef = draw.AddMaterialRef(matId);
}

///////////////////////////////////////////////////////////////////////////////

Vec4 WorldDraw::MStaticWorldMeshBatch::s_rgba(Vec4(1, 1, 1, 1));
Vec3 WorldDraw::MStaticWorldMeshBatch::s_scale(Vec3(1, 1, 1));

WorldDraw::MStaticWorldMeshBatch::MStaticWorldMeshBatch(WorldDraw &draw, const r::Mesh::Ref &m, const BBox &bounds, int matId) :
MBatchDraw(draw, matId, 0), m_m(m), m_bounds(bounds) {
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
	drawnActors = 0;
	testedActorModels = 0;
	drawnActorModels = 0;
	testedLights = 0;
	visLights = 0;
	drawnLights = 0;
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
	m_lights[0] = m_lights[1] = 0;
	m_rb = RB_WorldDraw::New(w);
}

WorldDraw::~WorldDraw() {

	CleanupLights();
	
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
	
	int r = details::LoadMaterial("Sys/ProjectedShadow_M", m_projected_M);
	if (r != pkg::SR_Success)
		return r;

	r = details::LoadMaterial("Sys/Shadow_M", m_shadow_M);
	if (r != pkg::SR_Success)
		return r;

#if defined(WORLD_DEBUG_DRAW)
	r = LoadDebugMaterials();
	if (r != pkg::SR_Success)
		return r;
#endif

	return m_rb->LoadMaterials();
}

void WorldDraw::Init(const bsp_file::BSPFile::Ref &bsp) {

	m_batchPool.Create(ZWorld, "world-draw-batch", 64);
	m_linkPool.Create(ZWorld, "world-draw-link", 64);
	m_interactionPool.Create(ZWorld, "world-light-interactions", sizeof(details::LightInteraction), 64);

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

	TickLights(dt);

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
	MStaticWorldMeshBatch::Ref r(new (ZWorld) MStaticWorldMeshBatch(*this, m, bounds, matId));
	m_worldModels.push_back(r);
}

void WorldDraw::Draw(Counters *counters) {
	m_counters.Clear();

	bool postFX = false;

	if (!m_uiOnly) {
		postFX = NumActivePostFX() > 0;
	}

	m_rb->BeginFrame();

	if (m_uiOnly) {
		m_rb->ClearBackBuffer();
	} else {
		if (postFX) {
			m_rb->BindRenderTarget();
		} else {
			m_rb->ClearBackBuffer();
		}

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

void WorldDraw::SetupPerspectiveFrustumPlanes(ViewDef &view) {

	// Things on back of frustum planes are outside the frustum
	view.frustum.clear();

	float yaspect = ((float)view.viewport[3]/(float)view.viewport[2]);
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

	normals[idx++] = fwd * s + left * -c;
	normals[idx++] = fwd * s + left * c;

	math::SinAndCos(&s, &c, yfov);
	
	normals[idx++] = fwd * s + up * -c;
	normals[idx++] = fwd * s + up * c;

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

void WorldDraw::SetupPerspectiveFrustumPlanes(
	PlaneVec &planes,
	const Camera &camera,
	const Vec4 &viewplanes,
	const Vec2 &zplanes
) {
	// Things on back of frustum planes are outside the frustum
	planes.clear();

	const Vec3 &fwd = camera.fwd;
	const Vec3 &left = camera.left;
	const Vec3 &up = camera.up;
	
	Plane p;

	if (ViewDef::kNumFrustumPlanes > 4) {
		Vec3 t = fwd * -zplanes[0];
		p.Initialize(fwd, camera.pos.get() + t, Plane::Unit);
		planes.push_back(p);
	}

	if (ViewDef::kNumFrustumPlanes > 5) {
		Vec3 t = fwd * -zplanes[1];
		p.Initialize(-fwd, camera.pos.get() + t, Plane::Unit);
		planes.push_back(p);
	}

	Vec3 xmin = -left * viewplanes[0];
	Vec3 xmax = -left * viewplanes[1];
	Vec3 ymin = up * viewplanes[2];
	Vec3 ymax = up * viewplanes[3];

	Vec3 viewOrg = camera.pos.get() + fwd * -zplanes[0];
	Vec3 topLeft = xmin + ymin + viewOrg;
	Vec3 topRight = xmax + ymin + viewOrg;
	Vec3 bottomLeft = xmin + ymax + viewOrg;
	Vec3 bottomRight = xmax + ymax + viewOrg;

	p.Initialize(camera.pos.get(), bottomLeft, topLeft);
	planes.push_back(p);

	p.Initialize(camera.pos.get(), topRight, bottomRight);
	planes.push_back(p);

	p.Initialize(camera.pos.get(), topLeft, topRight);
	planes.push_back(p);

	p.Initialize(camera.pos.get(), bottomRight, bottomLeft);
	planes.push_back(p);
}

void WorldDraw::SetupPerspectiveFrustumPlanes(
	ViewDef &view,
	const Vec4 &viewplanes,
	const Vec2 &zplanes
) {
	SetupPerspectiveFrustumPlanes(view.frustum, view.camera, viewplanes, zplanes);
}

void WorldDraw::SetupOrthoFrustumPlanes(
	ViewDef &view,
	const Vec4 &viewplanes,
	const Vec2 &zplanes
) {
	// Things on back of frustum planes are outside the frustum
	view.frustum.clear();

	const Vec3 &fwd = view.camera.fwd;
	const Vec3 &left = view.camera.left;
	const Vec3 &up = view.camera.up;
	
	//const float kWidth = (viewplanes[1] - viewplanes[0]) / 2.f;
	//const float kHeight = (viewplanes[2] - viewplanes[3]) / 2.f; // +Y up

	Plane p;

	if (ViewDef::kNumFrustumPlanes > 4) {
		Vec3 t = fwd * -zplanes[0];
		p.Initialize(fwd, view.camera.pos.get() + t, Plane::Unit);
		view.frustum.push_back(p);
	}

	if (ViewDef::kNumFrustumPlanes > 5) {
		Vec3 t = fwd * -zplanes[1];
		p.Initialize(-fwd, view.camera.pos.get() + t, Plane::Unit);
		view.frustum.push_back(p);
	}

	Vec3 t = -left * viewplanes[0];
	p.Initialize(-left, view.camera.pos.get() + t, Plane::Unit);
	view.frustum.push_back(p);

	t = -left * viewplanes[1];
	p.Initialize(left, view.camera.pos.get() + t, Plane::Unit);
	view.frustum.push_back(p);

	t = up * viewplanes[2];
	p.Initialize(-up, view.camera.pos.get() + t, Plane::Unit);
	view.frustum.push_back(p);

	t = up * viewplanes[3];
	p.Initialize(up, view.camera.pos.get() + t, Plane::Unit);
	view.frustum.push_back(p);
}

bool WorldDraw::CalcScissorBounds(
	const ViewDef &view,
	const BBox &bounds,
	Vec4 &rect
) {
	if (bounds.Contains(view.camera.pos))
		return false;

	rect[0] = std::numeric_limits<float>::max();
	rect[1] = std::numeric_limits<float>::max();
	rect[2] = std::numeric_limits<float>::min();
	rect[3] = std::numeric_limits<float>::min();

	for (int x = 0; x < 2; ++x) {
		float fx = x ? bounds.Maxs()[0] : bounds.Mins()[0];

		for (int y = 0; y < 2; ++y) {

			float fy = y ? bounds.Maxs()[1] : bounds.Mins()[1];

			for (int z = 0; z < 2; ++z) {

				float fz = z ? bounds.Maxs()[2] : bounds.Mins()[2];

				Vec3 p;
				::Project(
					view.mvp,
					&view.viewport[0],
					Vec3(fx, fy, fz),
					p
				);

				// bounds
				rect[0] = math::Min(rect[0], p[0]);
				rect[1] = math::Min(rect[1], p[1]);
				rect[2] = math::Max(rect[2], p[0]);
				rect[3] = math::Max(rect[3], p[1]);
			}
		}
	}

	// clamp
	rect[0] = math::Clamp(rect[0], (float)view.viewport[0], (float)view.viewport[0]+view.viewport[2]);
	rect[1] = math::Clamp(rect[1], (float)view.viewport[1], (float)view.viewport[1]+view.viewport[3]);
	rect[2] = math::Clamp(rect[2], (float)view.viewport[0], (float)view.viewport[0]+view.viewport[2]);
	rect[3] = math::Clamp(rect[3], (float)view.viewport[1], (float)view.viewport[1]+view.viewport[3]);

	if ((rect[0] == (float)view.viewport[0]) && (rect[1] == (float)view.viewport[1]) &&
		(rect[2] == ((float)view.viewport[0]+view.viewport[2])) && (rect[3] == ((float)view.viewport[1]+view.viewport[3]))) {
		return false;
	}

	return true;
}

void WorldDraw::CalcViewplaneBounds(
	const ViewDef *view,
	const Mat4 &mv,
	const BBox &bounds,
	const Vec3 *radial,
	Vec4 &viewplanes,
	Vec2 &zplanes
) {
	
	viewplanes[0] = std::numeric_limits<float>::max();
	viewplanes[1] = -std::numeric_limits<float>::max();
	viewplanes[2] = -std::numeric_limits<float>::max();
	viewplanes[3] = std::numeric_limits<float>::max();
	zplanes[0] = -std::numeric_limits<float>::max();
	zplanes[1] = std::numeric_limits<float>::max();

	for (int x = 0; x < 2; ++x) {
		float fx = x ? bounds.Maxs()[0] : bounds.Mins()[0];

		for (int y = 0; y < 2; ++y) {

			float fy = y ? bounds.Maxs()[1] : bounds.Mins()[1];

			for (int z = 0; z < 2; ++z) {

				float fz = z ? bounds.Maxs()[2] : bounds.Mins()[2];

				Vec3 p = mv.Transform(Vec3(fx, fy, fz));
				
#if defined(WORLD_DEBUG_DRAW)
				if (view && m_world->cvars->r_showunifiedlightbboxprojection.value) {
					Vec3 a = -p[2] * view->camera.fwd.get();
					Vec3 b = -p[0] * view->camera.left.get();
					Vec3 c =  p[1] * view->camera.up.get();
					m_dbgVars.projectedBoxPoints.push_back(view->camera.pos.get() + a+b+c);
				}
#endif

				// bounds
				viewplanes[0] = math::Min(viewplanes[0], p[0]);
				viewplanes[1] = math::Max(viewplanes[1], p[0]);
				viewplanes[2] = math::Max(viewplanes[2], p[1]); // +Y up
				viewplanes[3] = math::Min(viewplanes[3], p[1]);
				zplanes[0] = math::Max(zplanes[0], p[2]);
				zplanes[1] = math::Min(zplanes[1], p[2]);
			}
		}
	}

	if (radial) {
		Vec3 p = mv.Transform(*radial);
				
#if defined(WORLD_DEBUG_DRAW)
		if (view && m_world->cvars->r_showunifiedlightbboxprojection.value) {
			Vec3 a = -p[2] * view->camera.fwd.get();
			Vec3 b = -p[0] * view->camera.left.get();
			Vec3 c =  p[1] * view->camera.up.get();
			m_dbgVars.projectedBoxPoints.push_back(view->camera.pos.get() + a+b+c);
		}
#endif

		// bounds
		viewplanes[0] = math::Min(viewplanes[0], p[0]);
		viewplanes[1] = math::Max(viewplanes[1], p[0]);
		viewplanes[2] = math::Max(viewplanes[2], p[1]); // +Y up
		viewplanes[3] = math::Min(viewplanes[3], p[1]);
		zplanes[0] = math::Max(zplanes[0], p[2]);
		zplanes[1] = math::Min(zplanes[1], p[2]);
	}

	/*viewplanes[0] -= 8.f;
	viewplanes[1] += 8.f;
	viewplanes[2] += 8.f;
	viewplanes[3] -= 8.f;
	zplanes[0] += 8.f;
	zplanes[1] -= 8.f;*/
}

void WorldDraw::SetOrthoMatrix(
	const Vec4 &viewplanes,
	const Vec2 &zplanes
) {
	m_rb->SetOrthoMatrix(
		viewplanes[0],
		viewplanes[1],
		viewplanes[2],
		viewplanes[3],
		-zplanes[0],
		-zplanes[1]
	);
}

Mat4 WorldDraw::MakePerspectiveMatrix(
	const Vec4 &viewplanes,
	const Vec2 &zplanes,
	bool txAddressBias
) {
	Mat4 bias;

	if (txAddressBias) {
		bias = Mat4(
			Vec4(0.5f, 0.0f, 0.0f, 0.0f),
			Vec4(0.0f, 0.5f, 0.0f, 0.0f),
			Vec4(0.0f, 0.0f, 0.5f, 0.0f),
			Vec4(0.5f, 0.5f, 0.5f, 1.0f)
		);
	}

	return m_rb->MakePerspectiveMatrix(
		viewplanes[0],
		viewplanes[1],
		viewplanes[2],
		viewplanes[3],
		-zplanes[0],
		-zplanes[1],
		txAddressBias ? &bias : 0
	);
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

	if (view.sky) // add sky surfs
		VisMarkArea(view, kSkyArea, frustumVolume, frustumBounds);

#if defined(WORLD_DEBUG_DRAW)
	if (m_world->cvars->r_showfrustum.value) {
		m_dbgVars.frustum = frustumVolume;
		m_dbgVars.frustumAreas = frustumAreas;
	}
#endif
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

	view.sky = view.sky || area.sky;

	++m_counters.drawnAreas;

	// mark lights
	for (LightPtrSet::const_iterator it = area.lights.begin(); it != area.lights.end(); ++it) {
		Light &light = **it;

		if (light.m_markFrame != m_markFrame) {
			light.m_markFrame = m_markFrame;
			++m_counters.testedLights;
		}

		if (light.m_visFrame != m_markFrame) {
			
			BBox bounds(light.m_bounds);
			bounds.Translate(light.m_pos);
							
			if (!m_world->cvars->r_frustumcull.value || ClipBounds(volume, volumeBounds, bounds)) {
				++m_counters.visLights;
				light.m_visFrame = m_markFrame;
				view.visLights.push_back(&light);
#if defined(WORLD_DEBUG_DRAW)
				if (m_world->cvars->r_showlightscissor.value) {
					Vec4 scissorRect;
					if (CalcScissorBounds(view, bounds, scissorRect)) {
						m_dbgVars.lightScissors.push_back(scissorRect);
					}
				}
#endif
			}
		}
	}

	// mark world models.
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
				m_dbgVars.worldBBoxes.push_back(m->TransformedBounds());
#endif
			if (!m_world->cvars->r_frustumcull.value || ClipBounds(volume, volumeBounds, m->TransformedBounds())) {
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

		for (DrawModel::Map::const_iterator it = e->models->begin(); it != e->models->end(); ++it) {
			const DrawModel::Ref &m = it->second;
			if (!m->visible)
				continue;

			if (m->m_markFrame != m_markFrame) {
				m->m_markFrame = m_markFrame;
				++m_counters.testedEntityModels;
			}

			if (m->m_visibleFrame != m_markFrame) {

				if (!m_world->cvars->r_frustumcull.value || ClipBounds(volume, volumeBounds, m->TransformedBounds())) {
					m->m_visibleFrame = m_markFrame;
					++m_counters.drawnEntityModels;

					if (e->m_markFrame != m_markFrame) {
						e->m_markFrame = m_markFrame;
						++m_counters.drawnEntities;

#if defined(WORLD_DEBUG_DRAW)
						if (m_world->cvars->r_showentitybboxes.value) {
							BBox bounds(e->ps->bbox);
							bounds.Translate(e->ps->worldPos);
							m_dbgVars.entityBBoxes.push_back(bounds);
						}
#endif
					}			

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

		for (MBatchDraw::RefVec::const_iterator it = o->batches->begin(); it != o->batches->end(); ++it) {
			const MBatchDraw::Ref &m = *it;
			if (!m->visible)
				continue;

			if (m->m_markFrame != m_markFrame) {
				m->m_markFrame = m_markFrame;
				++m_counters.testedActorModels;
			}

			if (m->m_visibleFrame != m_markFrame) {
				
				if (!m_world->cvars->r_frustumcull.value || ClipBounds(volume, volumeBounds, m->TransformedBounds())) {
					m->m_visibleFrame = m_markFrame;
					++m_counters.drawnActorModels;

					if (o->m_markFrame != m_markFrame) {
						o->m_markFrame = m_markFrame;
						++m_counters.drawnActors;
			#if defined(WORLD_DEBUG_DRAW)
						if (m_world->cvars->r_showactorbboxes.value)
							m_dbgVars.actorBBoxes.push_back(o->bounds);
			#endif
					}

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
	m_world->game->Viewport(view.viewport[0], view.viewport[1], view.viewport[2], view.viewport[3]);

	m_rb->SetWorldStates();
	
#if defined(WORLD_DEBUG_DRAW)
	bool debugUniMatrix = true;
	if (m_world->cvars->r_lockvis.value) {
		if (!(m_world->cvars->r_viewunifiedlighttexturematrix.value && DebugSetupUnifiedLightTextureMatrixView(view)) &&
			!(m_world->cvars->r_viewunifiedlightprojectionmatrix.value && DebugSetupUnifiedLightProjectionMatrixView(view))) {
			if (!m_dbgVars.lockVis) {
				m_dbgVars.lockVisCamera = *m_world->camera.get();
				m_dbgVars.lockVis = true;
			}
			m_rb->SetPerspectiveMatrix(*m_world->camera.get(), view.viewport);
			m_rb->RotateForCamera(*m_world->camera.get());
			view.mvp = m_rb->GetModelViewProjectionMatrix();
			view.mv = m_rb->GetModelViewMatrix();
			view.camera = m_dbgVars.lockVisCamera;
			FindViewArea(view);
			SetupPerspectiveFrustumPlanes(view);
			debugUniMatrix = false;
		}
	} else if (!(m_world->cvars->r_viewunifiedlighttexturematrix.value && DebugSetupUnifiedLightTextureMatrixView(view)) &&
			!(m_world->cvars->r_viewunifiedlightprojectionmatrix.value && DebugSetupUnifiedLightProjectionMatrixView(view))) {
		debugUniMatrix = false;
		m_dbgVars.lockVis = false;
#endif
		view.camera = *m_world->camera.get();
		m_rb->SetPerspectiveMatrix(view.camera, view.viewport);
		m_rb->RotateForCamera(view.camera);
		view.mvp = m_rb->GetModelViewProjectionMatrix();
		view.mv = m_rb->GetModelViewMatrix();
		FindViewArea(view);
		SetupPerspectiveFrustumPlanes(view);
#if defined(WORLD_DEBUG_DRAW)
	}
#endif

	VisMarkAreas(view);
	m_counters.area = view.area;
	UpdateLightInteractions(view);
	VisMarkShadowCasters(view);
	
#if defined(WORLD_DEBUG_DRAW)
	if (m_dbgVars.lockVis || m_world->cvars->r_fly.value) { // restore world camera after vis has been calculated
		view.camera = *m_world->camera.get();
		if (debugUniMatrix && m_world->cvars->r_fly.value) {
			m_rb->SetPerspectiveMatrix(view.camera, view.viewport);
			m_rb->RotateForCamera(view.camera);
			view.mvp = m_rb->GetModelViewProjectionMatrix();
			view.mv = m_rb->GetModelViewMatrix();
		}
	}
#endif

	m_rb->numTris = 0;
	m_counters.numMaterials += (int)view.batches.size();

#if defined(WORLD_DEBUG_DRAW)
	if (m_world->cvars->r_showlightpasses.value) {
		DebugDrawLightPasses(view);
	} else if(m_world->cvars->r_showlightcounts.value) {
		DebugDrawLightCounts(view);
	} else {
#endif

	DrawViewBatches(view, false);

#if defined(WORLD_DEBUG_DRAW)
	}
#endif

	m_rb->ReleaseArrayStates(); // important! keeps pipeline changes from being recorded into VAO's
	m_counters.numTris += m_rb->numTris;

#if defined(WORLD_DEBUG_DRAW)

	if (m_world->cvars->r_showportals.value) {
		DebugDrawPortals(view);
	}

	if (m_world->cvars->r_showtris.value) {
		m_rb->wireframe = true;
		DrawViewBatches(view, true);
		m_rb->wireframe = false;
	}

	if (m_world->cvars->r_showfrustum.value) {
		DebugDrawFrustumVolumes(view);
		m_dbgVars.frustum->clear();
		m_dbgVars.frustumAreas->clear();
	}

	if (m_world->cvars->r_showentitybboxes.value) {
		DebugDrawBBoxes(m_dbgVars.entityBBox_M, m_dbgVars.entityBBoxes, true);
		m_dbgVars.entityBBoxes.clear();
	}
	
	if (m_world->cvars->r_showworldbboxes.value) {
		DebugDrawBBoxes(m_dbgVars.worldBBox_M, m_dbgVars.worldBBoxes, true);
		m_dbgVars.worldBBoxes.clear();
	}

	if (m_world->cvars->r_showactorbboxes.value) {
		DebugDrawBBoxes(m_dbgVars.actorBBox_M, m_dbgVars.actorBBoxes, true);
		m_dbgVars.actorBBoxes.clear();
	}

	if (m_world->cvars->r_showwaypoints.value) {
		DebugDrawActiveWaypoints();
	}

	if (m_world->cvars->r_showmovecmds.value) {
		DebugDrawFloorMoves();
	}

	if (m_world->cvars->r_showlights.value) {
		DebugDrawLights();
		m_dbgVars.lights.clear();
	}

	if (m_world->cvars->r_showunifiedlights.value) {
		DebugDrawUnifiedLights();
		m_dbgVars.unifiedLights.clear();
	}

	if (m_world->cvars->r_showunifiedlightbboxprojection.value) {
		DebugDrawProjectedBBoxPoints();
		m_dbgVars.projectedBoxPoints.clear();
	}

	if (debugUniMatrix) {
		DebugDrawUnifiedLightAxis();
	}

	// NOTE: scissor draw destroys perspective transform
	if (m_world->cvars->r_showlightscissor.value) {
		DebugDrawLightScissors();
		m_dbgVars.lightScissors.clear();
	}

#endif
}

void WorldDraw::UpdateLightInteractions(ViewDef &view) {
	for (LightVec::const_iterator it = view.visLights.begin(); it != view.visLights.end(); ++it) {
		UpdateLightInteractions(**it);
	}
}

void WorldDraw::VisMarkShadowCasters(ViewDef &view) {

	view.shadowEntities.reserve(64);
	view.shadowOccupants.reserve(64);

	for (LightVec::const_iterator it = view.visLights.begin(); it != view.visLights.end(); ++it) {
		const Light &light = **it;

		if (light.style.get()&Light::kStyle_CastShadows) {
			// add all visible objects as casters
			for (details::MatInteractionChain::const_iterator it = light.m_matInteractionChain.begin(); it != light.m_matInteractionChain.end(); ++it) {
				for (details::LightInteraction *i = it->second; i; i = i->nextOnLight) {
					if (i->entity && (i->entity->lightingFlags.get()&kLightingFlag_CastShadows)) {
						if (i->entity->m_shadowFrame != m_markFrame) {
							i->entity->m_shadowFrame = m_markFrame;
							view.shadowEntities.push_back(i->entity);
						}
					} else if (i->occupant  && (i->occupant->lightingFlags.get()&kLightingFlag_CastShadows)) {
						if (i->occupant->m_shadowFrame != m_markFrame) {
							i->occupant->m_shadowFrame = m_markFrame;
							view.shadowOccupants.push_back(i->occupant);
						}
					}
				}
			}
		}
	}
}

void WorldDraw::DrawOverlays() {
	for (ScreenOverlay::List::const_iterator it = m_overlays.begin(); it != m_overlays.end(); ++it)
		DrawOverlay(*(*it));
}

void WorldDraw::DrawUI() {
	m_world->uiRoot->Draw(0);
}

void WorldDraw::DrawViewBatches(ViewDef &view, bool wireframe) {

	if (wireframe) {
		
		for (details::MBatchIdMap::const_iterator it = view.batches.begin(); it != view.batches.end(); ++it) {
			DrawUnlitBatch(view, *it->second, wireframe);
		}

		return;
	}

	// draw unlit solid surfaces
	for (details::MBatchIdMap::const_iterator it = view.batches.begin(); it != view.batches.end(); ++it) {
		const details::MBatch &batch = *it->second;

		if (batch.matRef->mat->sort == r::Material::kSort_Solid) {
			if (batch.matRef->mat->maxLights == 0) {
				DrawUnlitBatch(view, *it->second, false);
			}
		}
	}

	// draw lit unshadowed solid surfaces
	
	if (m_world->cvars->r_enablelights.value) {
		for (details::MBatchIdMap::const_iterator it = view.batches.begin(); it != view.batches.end(); ++it) {
			const details::MBatch &batch = *it->second;

			if (batch.matRef->mat->sort == r::Material::kSort_Solid) {
				if (batch.matRef->mat->maxLights > 0) {
					DrawUnshadowedLitBatch(view, *it->second);
				}
			}
		}

		DrawViewUnifiedShadows(view);
	} else {
		for (details::MBatchIdMap::const_iterator it = view.batches.begin(); it != view.batches.end(); ++it) {
			const details::MBatch &batch = *it->second;

			if (batch.matRef->mat->sort == r::Material::kSort_Solid) {
				DrawUnlitBatch(view, *it->second, false);
			}
		}
	}

	// draw unlit translucent surfaces

	for (int i = r::Material::kSort_Translucent; i < r::Material::kNumSorts; ++i) {
		for (details::MBatchIdMap::const_iterator it = view.batches.begin(); it != view.batches.end(); ++it) {
			const details::MBatch &batch = *it->second;

			if (batch.matRef->mat->sort == (r::Material::Sort)i) {
				if (batch.matRef->mat->maxLights == 0) {
					DrawUnlitBatch(view, *it->second, false);
				}
			}
		}
	}

	// draw lit unshadowed translucent surfaces

	if (m_world->cvars->r_enablelights.value) {
		for (int i = r::Material::kSort_Translucent; i < r::Material::kNumSorts; ++i) {
			for (details::MBatchIdMap::const_iterator it = view.batches.begin(); it != view.batches.end(); ++it) {
				const details::MBatch &batch = *it->second;

				if (batch.matRef->mat->sort == (r::Material::Sort)i) {
					if (batch.matRef->mat->maxLights > 0) {
						DrawUnshadowedLitBatch(view, *it->second);
					}
				}
			}
		}
	} else {
		for (int i = r::Material::kSort_Translucent; i < r::Material::kNumSorts; ++i) {
			for (details::MBatchIdMap::const_iterator it = view.batches.begin(); it != view.batches.end(); ++it) {
				const details::MBatch &batch = *it->second;

				if (batch.matRef->mat->sort == (r::Material::Sort)i) {
					DrawUnlitBatch(view, *it->second, false);
				}
			}
		}
	}
}

void WorldDraw::DrawUnlitBatch(
	ViewDef &view,
	const details::MBatch &batch, 
	bool wireframe
) {

	if (!batch.head)
		return;

	Vec3 pos;
	Vec3 angles;
	Mat4 invTx;

#if defined(WORLD_DEBUG_DRAW)
	r::Material *mat = (wireframe) ? m_dbgVars.wireframe_M.material : batch.matRef->mat;
#else
	r::Material *mat = batch.matRef->mat;
#endif

	mat->BindStates();
	if (!wireframe)
		mat->BindTextures(batch.matRef->loader);
	mat->shader->Begin(r::Shader::kPass_Default, *mat);

	for (details::MBatchDrawLink *link = batch.head; link; link = link->next) {
		MBatchDraw *draw = link->draw;

		bool tx = draw->GetTransform(pos, angles);
		if (tx) {
			m_rb->PushMatrix(pos, draw->scale, angles);
			invTx = Mat4::Translation(-pos) * (Mat4::Rotation(QuatFromAngles(angles)).Transpose());
		}

		if (!wireframe)
			++m_counters.numBatches;

		draw->Bind(mat->shader.get().get());
		r::Shader::Uniforms u(draw->rgba.get());

		if (tx) {
			u.eyePos = invTx * view.camera.pos.get();
		} else {
			u.eyePos = view.camera.pos;
		}

		mat->shader->BindStates(u);
		m_rb->CommitStates();
		draw->CompileArrayStates(*mat->shader.get());
		draw->Draw();

		if (tx)
			m_rb->PopMatrix();
	}

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
	int num = NumActivePostFX();
	
	for (PostProcessEffect::Map::const_iterator it = m_postFX.begin(); it != m_postFX.end(); ++it) {
		const PostProcessEffect::Ref &fx = it->second;
		if (!fx->enabled)
			continue;

		Vec2 dstScale(1.f, 1.f);
		{
			PostProcessEffect::Map::const_iterator x = it;
			++x;
			for (; x != m_postFX.end(); ++x) {
				const PostProcessEffect::Ref &dfx = x->second;
				if (dfx->enabled) {
					dstScale = dfx->srcScale;
					break;
				}
			}
		}

		Shader::Uniforms u(fx->color.get());

		r::Material *m = fx->material;
		m->BindTextures(fx->loader);
		u.pfxVars = m_rb->BindPostFXTargets(--num > 0, *m, fx->srcScale, dstScale);
		m->BindStates();
		m->shader->Begin(r::Shader::kPass_Default, *m);
		m_rb->BindPostFXQuad();
		m->shader->BindStates(u);
		m_rb->CommitStates();
		m_rb->DrawPostFXQuad();
		m->shader->End();
	}
}

int WorldDraw::NumActivePostFX() const {
	int num = 0;

	for (PostProcessEffect::Map::const_iterator it = m_postFX.begin(); it != m_postFX.end(); ++it) {
		const PostProcessEffect::Ref &fx = it->second;
		if (fx->enabled)
			++num;
	}

	return num;
}

#if defined(WORLD_DEBUG_DRAW)
void WorldDraw::DebugAddEntityBBox(const BBox &bounds) {
	if (m_world->cvars->r_showentitybboxes.value) {
		m_dbgVars.entityBBoxes.push_back(bounds);
	}
}
#endif

} // world
