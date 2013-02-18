// DrawModel.cpp
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "../App.h"
#include "../Engine.h"
#include "../Packages/Packages.h"
#include "../MathUtils.h"
#include "../Assets/MeshMaterialLoader.h"
#include "../Assets/MeshVBLoader.h"
#include "DrawModel.h"
#include "Entity.h"
#include "World.h"
#include "Lua/D_Material.h"
#include "../Assets/SkMaterialLoader.h"

namespace world {

DrawModel::DrawModel(Entity *entity) : 
m_entity(entity), 
m_r(Vec3::Zero), 
m_p(Vec3::Zero),
m_bounds(Vec3::Zero, Vec3::Zero),
m_visible(true),
m_markFrame(-1),
m_visibleFrame(-1) {
	m_rgba[0] = m_rgba[1] = m_rgba[2] = Vec4(1, 1, 1, 1);
	m_fadeTime[0] = m_fadeTime[1] = 0.f;
	m_scale[0] = m_scale[1] = m_scale[2] = Vec3(1, 1, 1);
	m_scaleTime[0] = m_scaleTime[1] = 0.f;
}

DrawModel::~DrawModel() {
}

void DrawModel::RefBatch(const MBatchDraw::Ref &batch) {
	m_batches.push_back(batch);
}

bool DrawModel::GetTransform(Vec3 &pos, Vec3 &angles) const {
	if (!m_entity)
		return false;

	const Vec3 &entPos = m_entity->ps->worldPos;
	const Vec3 &entAngles = m_entity->ps->worldAngles;

	pos = entPos;

	if (m_p != Vec3::Zero) {
		if (entAngles != Vec3::Zero) {
			Quat q = QuatFromAngles(entAngles);
			Mat4 m = Mat4::Rotation(q);
			pos += (m.Transform3X3(m_p));
		} else {
			pos += m_p;
		}
	}
	
	angles = m_r + entAngles;

	return true;
}

void DrawModel::Tick(float time, float dt) {
	if (m_fadeTime[1] > 0.f) {
		m_fadeTime[0] += dt;
		if (m_fadeTime[0] >= m_fadeTime[1]) {
			m_fadeTime[1] = 0.f;
			m_rgba[0] = m_rgba[2];
		} else {
			m_rgba[0] = math::Lerp(m_rgba[1], m_rgba[2], m_fadeTime[0]/m_fadeTime[1]);
		}
	}

	if (m_scaleTime[1] > 0.f) {
		m_scaleTime[0] += dt;
		if (m_scaleTime[0] >= m_scaleTime[1]) {
			m_scaleTime[1] = 0.f;
			m_scale[0] = m_scale[2];
		} else {
			m_scale[0] = math::Lerp(m_scale[1], m_scale[2], m_scaleTime[0]/m_scaleTime[1]);
		}
	}

	OnTick(time, dt);
}

void DrawModel::BlendTo(const Vec4 &rgba, float time) {
	if (time <= 0.f) {
		m_fadeTime[1] = 0.f;
		m_rgba[0] = rgba;
	} else {
		m_rgba[1] = m_rgba[0];
		m_rgba[2] = rgba;
		m_fadeTime[0] = 0.f;
		m_fadeTime[1] = time;
	}
}

void DrawModel::ScaleTo(const Vec3 &scale, float time) {
	if (time <= 0.f) {
		m_scaleTime[1] = 0.f;
		m_scale[0] = scale;
	} else {
		m_scale[1] = m_scale[0];
		m_scale[2] = scale;
		m_scaleTime[0] = 0.f;
		m_scaleTime[1] = time;
	}
}

void DrawModel::ReplaceMaterial(int src, int dst) {
	for (MBatchDraw::RefVec::iterator it = m_batches.begin(); it != m_batches.end(); ++it) {
		const MBatchDraw::Ref &r = *it;
		if (r->matId == src)
			r->matId = dst;
	}
}

void DrawModel::ReplaceMaterials(int dst) {
	for (MBatchDraw::RefVec::iterator it = m_batches.begin(); it != m_batches.end(); ++it) {
		const MBatchDraw::Ref &r = *it;
		r->matId = dst;
	}
}

void DrawModel::PushElements(lua_State *L) {
	lua_pushcfunction(L, lua_BlendTo);
	lua_setfield(L, -2, "BlendTo");
	lua_pushcfunction(L, lua_ReplaceMaterial);
	lua_setfield(L, -2, "ReplaceMaterial");
	lua_pushcfunction(L, lua_ReplaceMaterials);
	lua_setfield(L, -2, "ReplaceMaterials");
	lua_pushcfunction(L, lua_MaterialList);
	lua_setfield(L, -2, "MaterialList");
	lua_pushcfunction(L, lua_ScaleTo);
	lua_setfield(L, -2, "ScaleTo");
	
	LUART_REGISTER_GETSET(L, Pos);
	LUART_REGISTER_GETSET(L, Angles);
	LUART_REGISTER_GET(L, Scale);
	LUART_REGISTER_GETSET(L, Visible);
	LUART_REGISTER_GETSET(L, Bounds);
}

int DrawModel::lua_BlendTo(lua_State *L) {
	Ref r = lua::SharedPtr::Get<DrawModel>(L, "DrawModel", 1, true);
	r->BlendTo(lua::Marshal<Vec4>::Get(L, 2, true), (float)luaL_checknumber(L, 3));
	return 0;
}

int DrawModel::lua_ReplaceMaterial(lua_State *L) {
	Ref r = lua::SharedPtr::Get<DrawModel>(L, "DrawModel", 1, true);
	D_Material::Ref a = lua::SharedPtr::Get<D_Material>(L, "Material", 2, true);
	D_Material::Ref b = lua::SharedPtr::Get<D_Material>(L, "Material", 3, true);
	r->ReplaceMaterial(a->assetId, b->assetId);
	return 0;
}

int DrawModel::lua_ReplaceMaterials(lua_State *L) {
	Ref r = lua::SharedPtr::Get<DrawModel>(L, "DrawModel", 1, true);
	D_Material::Ref a = lua::SharedPtr::Get<D_Material>(L, "Material", 2, true);
	r->ReplaceMaterials(a->assetId);
	return 0;
}

int DrawModel::lua_MaterialList(lua_State *L) {
	Ref r = lua::SharedPtr::Get<DrawModel>(L, "DrawModel", 1, true);
	return r->lua_PushMaterialList(L);
}

int DrawModel::lua_ScaleTo(lua_State *L) {
	Ref r = lua::SharedPtr::Get<DrawModel>(L, "DrawModel", 1, true);
	r->ScaleTo(
		lua::Marshal<Vec3>::Get(L, 2, true),
		(float)luaL_checknumber(L, 3)
	);
	return 0;
}

#define SELF Ref self = lua::SharedPtr::Get<DrawModel>(L, "DrawModel", 1, true)
LUART_GETSET(DrawModel, Pos, Vec3, m_p, SELF);
LUART_GETSET(DrawModel, Angles, Vec3, m_r, SELF);
LUART_GET(DrawModel, Scale, Vec3, m_scale[0], SELF);
LUART_GETSET(DrawModel, Visible, bool, m_visible, SELF);

int DrawModel::LUART_GETFN(Bounds)(lua_State *L) {
	Ref r = lua::SharedPtr::Get<DrawModel>(L, "DrawModel", 1, true);
	lua::Marshal<Vec3>::Push(L, r->m_bounds.Mins());
	lua::Marshal<Vec3>::Push(L, r->m_bounds.Maxs());
	return 2;
}

int DrawModel::LUART_SETFN(Bounds)(lua_State *L) {
	Ref r = lua::SharedPtr::Get<DrawModel>(L, "DrawModel", 1, true);
	Vec3 mins = lua::Marshal<Vec3>::Get(L, 2, true);
	Vec3 maxs = lua::Marshal<Vec3>::Get(L, 3, true);
	r->m_bounds.Initialize(mins, maxs);
	return 0;
}

LUART_GET(DrawModel, RGBA, Vec4, m_rgba[0], SELF);
#undef SELF

DrawModel::DrawBatch::DrawBatch(DrawModel &model, int matId) : MBatchDraw(matId), m_model(&model) {
}

bool DrawModel::DrawBatch::GetTransform(Vec3 &pos, Vec3 &angles) const {
	return m_model ? m_model->GetTransform(pos, angles) : false;
}

///////////////////////////////////////////////////////////////////////////////

MeshDrawModel::Ref MeshDrawModel::New(
	Entity *entity, 
	const r::Mesh::Ref &m, 
	int matId
) {
	Ref r(new (ZWorld) MeshDrawModel(entity));
	r->m_mesh = m;
	r->m_matId = matId;

	Batch::Ref b(new (ZWorld) Batch(*r, m, matId));
	entity->world->draw->AddMaterial(matId);

	r->RefBatch(b);
	return r;
}

MeshDrawModel::MeshDrawModel(Entity *entity) : DrawModel(entity) {
}

MeshDrawModel::~MeshDrawModel() {
}

void MeshDrawModel::PushElements(lua_State *L) {
	DrawModel::PushElements(L);
	lua_pushcfunction(L, lua_CreateInstance);
	lua_setfield(L, -2, "CreateInstance");
}

int MeshDrawModel::lua_CreateInstance(lua_State *L) {
	Ref r = lua::SharedPtr::Get<MeshDrawModel>(L, "MeshDrawModel", 1, true);
	r->CreateInstance()->Push(L);
	return 1;
}

int MeshDrawModel::lua_PushMaterialList(lua_State *L) {
	pkg::Asset::Ref asset = App::Get()->engine->sys->packages->Asset(m_matId, pkg::Z_Engine);
	if (asset) {
		D_Material::Ref m = D_Material::New(asset);
		if (m) {
			lua_createtable(L, 1, 0);
			lua_pushinteger(L, 1);
			m->Push(L);
			lua_settable(L, -3);
			return 1;
		}
	}
	return 0;
}

MeshDrawModel::Ref MeshDrawModel::CreateInstance() {
	return New(entity, m_mesh, m_matId);
}

MeshDrawModel::Batch::Batch(DrawModel &model, const r::Mesh::Ref &m, int matId) : 
DrawModel::DrawBatch(model, matId), m_m(m) {
}

void MeshDrawModel::Batch::Bind(r::Shader *shader) {
	m_m->BindAll(shader);
}

void MeshDrawModel::Batch::CompileArrayStates(r::Shader &shader) {
	m_m->CompileArrayStates(shader);
}

void MeshDrawModel::Batch::FlushArrayStates(r::Shader *shader) {
	m_m->FlushArrayStates(shader);
}

void MeshDrawModel::Batch::Draw() {
	m_m->Draw();
}

///////////////////////////////////////////////////////////////////////////////

MeshBundleDrawModel::Ref MeshBundleDrawModel::New(
	Entity *entity, 
	const pkg::AssetRef &asset
) {
	if (asset->type != asset::AT_Mesh)
		return Ref();

	Ref r(new (ZWorld) MeshBundleDrawModel(entity));
	r->m_asset = asset;

	WorldDraw *draw = entity->world->draw;

	asset::MeshVBLoader *bundle = asset::MeshVBLoader::Cast(asset);
	asset::MeshMaterialLoader *loader = asset::MeshMaterialLoader::Cast(asset);

	for (int i = 0; i < bundle->numMeshes; ++i) {
		Batch::Ref b(new (ZWorld) Batch(*r, bundle->Mesh(i), loader->MaterialAsset(i)->id));
		draw->AddMaterial(loader->MaterialAsset(i)->id);

		r->RefBatch(b);
	}

	return r;
}

MeshBundleDrawModel::MeshBundleDrawModel(Entity *entity) : DrawModel(entity) {
}

MeshBundleDrawModel::~MeshBundleDrawModel() {
}

MeshBundleDrawModel::Ref MeshBundleDrawModel::CreateInstance() {
	return New(entity, m_asset);
}

void MeshBundleDrawModel::PushElements(lua_State *L) {
	DrawModel::PushElements(L);
	lua_pushcfunction(L, lua_CreateInstance);
	lua_setfield(L, -2, "CreateInstance");
}

int MeshBundleDrawModel::lua_PushMaterialList(lua_State *L) {

	asset::MeshMaterialLoader *loader = asset::MeshMaterialLoader::Cast(m_asset);
	int numUniqueMaterials = loader->numUniqueMaterials;

	if (numUniqueMaterials < 1)
		return 0;

	lua_createtable(L, numUniqueMaterials, 0);

	for (int i = 0; i < numUniqueMaterials; ++i) {
		const pkg::Asset::Ref &asset = loader->UniqueMaterialAsset(i);
		if (asset) {
			D_Material::Ref m = D_Material::New(asset);
			if (m) {
				lua_pushinteger(L, i+1);
				m->Push(L);
				lua_settable(L, -3);
			}
		}
	}

	return 1;
}

int MeshBundleDrawModel::lua_CreateInstance(lua_State *L) {
	Ref r = lua::SharedPtr::Get<MeshBundleDrawModel>(L, "MeshBundleDrawModel", 1, true);
	r->CreateInstance()->Push(L);
	return 1;
}

MeshBundleDrawModel::Batch::Batch(DrawModel &model, const r::Mesh::Ref &m, int matId) : 
DrawModel::DrawBatch(model, matId), m_m(m) {
}

void MeshBundleDrawModel::Batch::Bind(r::Shader *shader) {
	m_m->BindAll(shader);
}

void MeshBundleDrawModel::Batch::CompileArrayStates(r::Shader &shader) {
	m_m->CompileArrayStates(shader);
}

void MeshBundleDrawModel::Batch::FlushArrayStates(r::Shader *shader) {
	m_m->FlushArrayStates(shader);
}

void MeshBundleDrawModel::Batch::Draw() {
	m_m->Draw();
}

///////////////////////////////////////////////////////////////////////////////

SkMeshDrawModel::Ref SkMeshDrawModel::New(
	Entity *entity, 
	const r::SkMesh::Ref &m
) {
	Ref r(new (ZWorld) SkMeshDrawModel(entity, m));

	asset::SkMaterialLoader *loader = asset::SkMaterialLoader::Cast(m->asset);
	if (!loader)
		return Ref();

	WorldDraw *draw = entity->world->draw;

	for (int i = 0 ; i < m->numMeshes; ++i) {
		pkg::Asset::Ref material = loader->MaterialAsset(i);
		if (!material)
			continue;

		Batch::Ref b(new (ZWorld) Batch(*r, m, i, material->id));
		draw->AddMaterial(material->id);

		r->RefBatch(b);
	}

	return r;
}

SkMeshDrawModel::SkMeshDrawModel(Entity *entity, const r::SkMesh::Ref &m) : 
DrawModel(entity),
m_mesh(m),
m_motionScale(1.f),
m_timeScale(1.f),
m_instanced(false) {
}

SkMeshDrawModel::~SkMeshDrawModel() {
}

SkMeshDrawModel::Ref SkMeshDrawModel::CreateInstance() {
	Ref r = New(entity, m_mesh);
	r->m_instanced = true;
	r->m_motionScale = m_motionScale;
	r->m_timeScale = m_timeScale;
	return r;
}

void SkMeshDrawModel::OnTick(float time, float dt) {
	if (visible && !m_instanced) {
		m_mesh->ska->Tick(
			dt * m_timeScale, 
			entity->ps->distanceMoved * m_motionScale,
			true, 
			true, 
			Mat4::Identity
		);
	}
}

void SkMeshDrawModel::PushElements(lua_State *L) {
	DrawModel::PushElements(L);
	LUART_REGISTER_GETSET(L, TimeScale);
	LUART_REGISTER_GETSET(L, MotionScale);
	lua_pushcfunction(L, lua_CreateInstance);
	lua_setfield(L, -2, "CreateInstance");
	lua_pushcfunction(L, lua_FindBone);
	lua_setfield(L, -2, "FindBone");
	lua_pushcfunction(L, lua_BonePos);
	lua_setfield(L, -2, "BonePos");
	lua_pushcfunction(L, lua_WorldBonePos);
	lua_setfield(L, -2, "WorldBonePos");
}

int SkMeshDrawModel::lua_PushMaterialList(lua_State *L) {

	asset::SkMaterialLoader *loader = asset::SkMaterialLoader::Cast(m_mesh->asset);
	int numUniqueMaterials = loader->numUniqueMaterials;

	if (numUniqueMaterials < 1)
		return 0;

	lua_createtable(L, numUniqueMaterials, 0);

	for (int i = 0; i < numUniqueMaterials; ++i) {
		const pkg::Asset::Ref &asset = loader->UniqueMaterialAsset(i);
		if (asset) {
			D_Material::Ref m = D_Material::New(asset);
			if (m) {
				lua_pushinteger(L, i+1);
				m->Push(L);
				lua_settable(L, -3);
			}
		}
	}

	return 1;
}

int SkMeshDrawModel::lua_CreateInstance(lua_State *L) {
	Ref r = lua::SharedPtr::Get<SkMeshDrawModel>(L, "SkMeshDrawModel", 1, true);
	r->CreateInstance()->Push(L);
	return 1;
}

int SkMeshDrawModel::lua_FindBone(lua_State *L) {
	Ref r = lua::SharedPtr::Get<SkMeshDrawModel>(L, "SkMeshDrawModel", 1, true);
	lua_pushinteger(L, r->mesh->ska->FindBone(luaL_checkstring(L, 2)));
	return 1;
}

int SkMeshDrawModel::lua_BonePos(lua_State *L) {
	Ref r = lua::SharedPtr::Get<SkMeshDrawModel>(L, "SkMeshDrawModel", 1, true);
	Vec3 pos = r->BonePos((int)luaL_checkinteger(L, 2));
	lua::Marshal<Vec3>::Push(L, pos);
	return 1;
}

int SkMeshDrawModel::lua_WorldBonePos(lua_State *L) {
	Ref r = lua::SharedPtr::Get<SkMeshDrawModel>(L, "SkMeshDrawModel", 1, true);
	Vec3 pos = r->WorldBonePos((int)luaL_checkinteger(L, 2));
	lua::Marshal<Vec3>::Push(L, pos);
	return 1;
}

#define SELF Ref self = lua::SharedPtr::Get<SkMeshDrawModel>(L, "SkMeshDrawModel", 1, true)
LUART_GETSET(SkMeshDrawModel, TimeScale, float, m_timeScale, SELF);
LUART_GETSET(SkMeshDrawModel, MotionScale, float, m_motionScale, SELF);
#undef SELF

SkMeshDrawModel::Batch::Batch(DrawModel &model, const r::SkMesh::Ref &m, int idx, int matId) :
DrawModel::DrawBatch(model, matId), m_idx(idx), m_m(m) {
}

Vec3 SkMeshDrawModel::WorldBonePos(int idx) const {
	if (idx >= 0 && idx < m_mesh->ska->numBones) {
		Vec3 bone = m_mesh->ska->BoneWorldMat(idx)[3];
		Vec3 pos, rot;
		
		if (GetTransform(pos, rot)) {
			bone =  (Mat4::Scaling(Scale3(scale)) * Mat4::Rotation(QuatFromAngles(rot)) * Mat4::Translation(pos)) * bone;
		}

		return bone;
	}
	return Vec3::Zero;
}

Vec3 SkMeshDrawModel::BonePos(int idx) const {
	if (idx >= 0 && idx < m_mesh->ska->numBones) {
		Vec3 bone = m_mesh->ska->BoneWorldMat(idx)[3];
		bone = (Mat4::Scaling(Scale3(scale)) * Mat4::Translation(pos)) * bone;
		return bone;
	}
	return Vec3::Zero;
}
	
void SkMeshDrawModel::Batch::Bind(r::Shader *shader) {
	m_m->Skin(m_idx);
	r::Mesh &m = m_m->Mesh(m_idx);
	m.BindAll(shader);
}

void SkMeshDrawModel::Batch::CompileArrayStates(r::Shader &shader) {
	m_m->Mesh(m_idx).CompileArrayStates(shader);
}

void SkMeshDrawModel::Batch::FlushArrayStates(r::Shader *shader) {
	m_m->Mesh(m_idx).FlushArrayStates(shader);
}

void SkMeshDrawModel::Batch::Draw() {
	m_m->Mesh(m_idx).Draw();
}

} // world
