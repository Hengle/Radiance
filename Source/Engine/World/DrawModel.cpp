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
#include "../Assets/VtMaterialLoader.h"

namespace world {

DrawModel::DrawModel(Entity *entity) : 
m_entity(entity), 
m_r(Vec3::Zero), 
m_p(Vec3::Zero),
m_bounds(Vec3::Zero, Vec3::Zero),
m_visible(true),
m_markFrame(-1),
m_visibleFrame(-1),
m_inView(false) {
	m_draw = entity->world->draw;
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
	for (MBatchDraw::Vec::iterator it = m_batches.begin(); it != m_batches.end(); ++it) {
		const MBatchDraw::Ref &r = *it;
		if (r->matId == src)
			r->matId = dst;
	}
}

void DrawModel::ReplaceMaterials(int dst) {
	for (MBatchDraw::Vec::iterator it = m_batches.begin(); it != m_batches.end(); ++it) {
		const MBatchDraw::Ref &r = *it;
		r->matId = dst;
	}
}

BBox DrawModel::TransformedBounds() const {
	BBox bounds = m_bounds;
	Vec3 pos, angles;
	if (GetTransform(pos, angles))
		bounds.Translate(pos);
	return bounds;
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
	r->ReplaceMaterial(a->asset->entry->id, b->asset->entry->id);
	return 0;
}

int DrawModel::lua_ReplaceMaterials(lua_State *L) {
	Ref r = lua::SharedPtr::Get<DrawModel>(L, "DrawModel", 1, true);
	D_Material::Ref a = lua::SharedPtr::Get<D_Material>(L, "Material", 2, true);
	r->ReplaceMaterials(a->asset->entry->id);
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

DrawModel::DrawBatch::DrawBatch(DrawModel &model, int matId) : 
MBatchDraw(*model.entity->world->draw, matId, model.m_entity), m_model(&model), m_draw(model.worldDraw) {
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

///////////////////////////////////////////////////////////////////////////////

VtMeshDrawModel::Ref VtMeshDrawModel::New(
	Entity *entity, 
	const r::VtMesh::Ref &m
) {
	Ref r(new (ZWorld) VtMeshDrawModel(entity, m));

	asset::VtMaterialLoader *loader = asset::VtMaterialLoader::Cast(m->asset);
	if (!loader)
		return Ref();

	WorldDraw *draw = entity->world->draw;

	for (int i = 0 ; i < m->numMeshes; ++i) {
		pkg::Asset::Ref material = loader->MaterialAsset(i);
		if (!material)
			continue;

		Batch::Ref b(new (ZWorld) Batch(*r, m, i, material->id));
		r->RefBatch(b);
	}

	return r;
}

VtMeshDrawModel::VtMeshDrawModel(Entity *entity, const r::VtMesh::Ref &m) : 
DrawModel(entity),
m_mesh(m),
m_timeScale(1.f),
m_instanced(false) {
}

VtMeshDrawModel::~VtMeshDrawModel() {
}

VtMeshDrawModel::Ref VtMeshDrawModel::CreateInstance() {
	Ref r = New(entity, m_mesh);
	r->m_instanced = true;
	r->m_timeScale = m_timeScale;
	return r;
}

void VtMeshDrawModel::OnTick(float time, float dt) {
	if (visible && !m_instanced) {
		m_mesh->vtm->Tick(
			dt * m_timeScale, 
			true, 
			true
		);
	}
}

void VtMeshDrawModel::PushElements(lua_State *L) {
	DrawModel::PushElements(L);
	LUART_REGISTER_GETSET(L, TimeScale);
	lua_pushcfunction(L, lua_CreateInstance);
	lua_setfield(L, -2, "CreateInstance");
}

int VtMeshDrawModel::lua_PushMaterialList(lua_State *L) {

	asset::VtMaterialLoader *loader = asset::VtMaterialLoader::Cast(m_mesh->asset);
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

int VtMeshDrawModel::lua_CreateInstance(lua_State *L) {
	Ref r = lua::SharedPtr::Get<VtMeshDrawModel>(L, "VtMeshDrawModel", 1, true);
	r->CreateInstance()->Push(L);
	return 1;
}

#define SELF Ref self = lua::SharedPtr::Get<VtMeshDrawModel>(L, "VtMeshDrawModel", 1, true)
LUART_GETSET(VtMeshDrawModel, TimeScale, float, m_timeScale, SELF);
#undef SELF

VtMeshDrawModel::Batch::Batch(DrawModel &model, const r::VtMesh::Ref &m, int idx, int matId) :
DrawModel::DrawBatch(model, matId), m_idx(idx), m_m(m) {
}
	
void VtMeshDrawModel::Batch::Bind(r::Shader *shader) {
	m_m->Skin(m_idx);
	r::Mesh &m = m_m->Mesh(m_idx);
	m.BindAll(shader);
}

void VtMeshDrawModel::Batch::CompileArrayStates(r::Shader &shader) {
	m_m->Mesh(m_idx).CompileArrayStates(shader);
}

void VtMeshDrawModel::Batch::FlushArrayStates(r::Shader *shader) {
	m_m->Mesh(m_idx).FlushArrayStates(shader);
}

void VtMeshDrawModel::Batch::Draw() {
	m_m->Mesh(m_idx).Draw();
}

///////////////////////////////////////////////////////////////////////////////

SpriteBatchDrawModel::Ref SpriteBatchDrawModel::New(
	Entity *entity, 
	const r::SpriteBatch::Ref &sprites,
	int matId
) {
	SpriteBatchDrawModel::Ref r(new SpriteBatchDrawModel(entity, sprites, matId));

	WorldDraw *draw = entity->world->draw;

	Batch::Ref b(new (ZWorld) Batch(*r, sprites, matId));
	r->RefBatch(b);

	return r;
}

SpriteBatchDrawModel::SpriteBatchDrawModel(
	Entity *entity,
	const r::SpriteBatch::Ref &sprites,
	int matId
) : DrawModel(entity), m_spriteBatch(sprites), m_matId(matId) {

}

SpriteBatchDrawModel::~SpriteBatchDrawModel() {
}

void SpriteBatchDrawModel::OnTick(float time, float dt) {
	m_spriteBatch->Skin();
}

void SpriteBatchDrawModel::PushElements(lua_State *L) {
	DrawModel::PushElements(L);
	lua_pushcfunction(L, lua_AllocateSprite);
	lua_setfield(L, -2, "AllocateSprite");
	lua_pushcfunction(L, lua_FreeSprite);
	lua_setfield(L, -2, "FreeSprite");
	lua_pushcfunction(L, lua_SetSpriteData);
	lua_setfield(L, -2, "SetSpriteData");
}

int SpriteBatchDrawModel::lua_PushMaterialList(lua_State *L) {

	pkg::Asset::Ref asset = App::Get()->engine->sys->packages->Asset(m_matId, pkg::Z_Engine);

	if (asset) {
		lua_createtable(L, 1, 0);
		D_Material::Ref m = D_Material::New(asset);
		if (m) {
			lua_pushinteger(L, 1);
			m->Push(L);
			lua_settable(L, -3);
		}

		return 1;
	}
	
	return 0;
}

int SpriteBatchDrawModel::lua_AllocateSprite(lua_State *L) {
	SpriteBatchDrawModel::Ref r = lua::SharedPtr::Get<SpriteBatchDrawModel>(L, "SpriteBatchDrawModel", 1, true);
	r::Sprite *sprite = r->m_spriteBatch->AllocateSprite();
	if (sprite) {
		lua_pushlightuserdata(L, sprite);
		return 1;
	}
	return 0;
}

int SpriteBatchDrawModel::lua_FreeSprite(lua_State *L) {
	SpriteBatchDrawModel::Ref r = lua::SharedPtr::Get<SpriteBatchDrawModel>(L, "SpriteBatchDrawModel", 1, true);
	r::Sprite *sprite = (r::Sprite*)lua_touserdata(L, 2);
	if (sprite)
		r->m_spriteBatch->FreeSprite(sprite);
	return 0;
}

int SpriteBatchDrawModel::lua_SetSpriteData(lua_State *L) {
	SpriteBatchDrawModel::Ref r = lua::SharedPtr::Get<SpriteBatchDrawModel>(L, "SpriteBatchDrawModel", 1, true);
	r::Sprite *sprite = (r::Sprite*)lua_touserdata(L, 2);
	if (sprite) {
		luaL_checktype(L, 3, LUA_TTABLE);
		lua_getfield(L, 3, "pos");
		sprite->pos = lua::Marshal<Vec3>::Get(L, -1, true);
		lua_getfield(L, 3, "rgba");
		sprite->rgba = lua::Marshal<Vec4>::Get(L, -1, true);
		lua_getfield(L, 3, "size");
		sprite->size = lua::Marshal<Vec2>::Get(L, -1, true);
		lua_getfield(L, 3, "rot");
		sprite->rot = (float)luaL_checknumber(L, -1);
		lua_pop(L, 4);
	}
	return 0;
}

SpriteBatchDrawModel::Batch::Batch(DrawModel &model, const r::SpriteBatch::Ref &m, int matId) 
: DrawModel::DrawBatch(model, matId), m_spriteBatch(m) {

}

void SpriteBatchDrawModel::Batch::Bind(r::Shader *shader) {
	m_spriteBatch->mesh->BindAll(shader);
}

void SpriteBatchDrawModel::Batch::CompileArrayStates(r::Shader &shader) {
	m_spriteBatch->mesh->CompileArrayStates(shader);
}

void SpriteBatchDrawModel::Batch::FlushArrayStates(r::Shader *shader) {
	m_spriteBatch->mesh->FlushArrayStates(shader);
}

void SpriteBatchDrawModel::Batch::Draw() {
	m_spriteBatch->Draw();
}

///////////////////////////////////////////////////////////////////////////////

ParticleEmitterDrawModel::Ref ParticleEmitterDrawModel::New(
	Entity *entity, 
	const r::ParticleEmitter::Ref &emitter,
	const pkg::Asset::Ref &asset,
	int matId
) {
	ParticleEmitterDrawModel::Ref r(new ParticleEmitterDrawModel(entity, emitter, asset, matId));

	WorldDraw *draw = entity->world->draw;

	for (int i = 0; i < emitter->numBatches; ++i) {
		Batch::Ref b(new (ZWorld) Batch(*r, emitter, matId, i));
		r->RefBatch(b);
	}

	return r;
}

ParticleEmitterDrawModel::ParticleEmitterDrawModel(
	Entity *entity,
	const r::ParticleEmitter::Ref &emitter, 
	const pkg::Asset::Ref &asset,
	int matId
) : DrawModel(entity), m_emitter(emitter), m_asset(asset), m_matId(matId) {

}

ParticleEmitterDrawModel::~ParticleEmitterDrawModel() {
}

void ParticleEmitterDrawModel::OnTick(float time, float dt) {
	if (inView) { // expensive don't do this if we aren't in view
		m_emitter->Tick(dt);
		worldDraw->counters->simulatedParticles += m_emitter->numParticles;
	}
}

int ParticleEmitterDrawModel::lua_PushMaterialList(lua_State *L) {

	pkg::Asset::Ref asset = App::Get()->engine->sys->packages->Asset(m_matId, pkg::Z_Engine);

	if (asset) {
		lua_createtable(L, 1, 0);
		D_Material::Ref m = D_Material::New(asset);
		if (m) {
			lua_pushinteger(L, 1);
			m->Push(L);
			lua_settable(L, -3);
		}

		return 1;
	}
	
	return 0;
}

ParticleEmitterDrawModel::Batch::Batch(
	DrawModel &model, 
	const r::ParticleEmitter::Ref &m, 
	int matId,
	int batchIdx
) : DrawModel::DrawBatch(model, matId), m_emitter(m), m_batchIdx(batchIdx) {

}

void ParticleEmitterDrawModel::Batch::Bind(r::Shader *shader) {
	m_emitter->Skin();
	r::SpriteBatch &batch = m_emitter->Batch(m_batchIdx);
	RAD_ASSERT(batch.numSprites > 0);
	batch.mesh->BindAll(shader);
}

void ParticleEmitterDrawModel::Batch::CompileArrayStates(r::Shader &shader) {
	r::SpriteBatch &batch = m_emitter->Batch(m_batchIdx);
	RAD_ASSERT(batch.numSprites > 0);
	batch.mesh->CompileArrayStates(shader);
}

void ParticleEmitterDrawModel::Batch::FlushArrayStates(r::Shader *shader) {
	r::SpriteBatch &batch = m_emitter->Batch(m_batchIdx);
	RAD_ASSERT(batch.numSprites > 0);
	batch.mesh->FlushArrayStates(shader);
}

void ParticleEmitterDrawModel::Batch::Draw() {
	r::SpriteBatch &batch = m_emitter->Batch(m_batchIdx);
	RAD_ASSERT(batch.numSprites > 0);
	batch.Draw();
	worldDraw->counters->drawnParticles += batch.numSprites;
}

bool ParticleEmitterDrawModel::Batch::RAD_IMPLEMENT_GET(visible) {
	const r::SpriteBatch &batch = m_emitter->Batch(m_batchIdx);
	return batch.numSprites > 0;
}

} // world
