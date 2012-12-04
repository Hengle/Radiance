// DrawModel.cpp
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "../MathUtils.h"
#include "DrawModel.h"
#include "Entity.h"
#include "World.h"
#include "../Assets/SkMaterialLoader.h"

namespace world {

DrawModel::DrawModel(Entity *entity) : 
m_entity(entity), 
m_r(Vec3::Zero), 
m_p(Vec3::Zero),
m_bounds(Vec3::Zero, Vec3::Zero),
m_scale(Vec3(1, 1, 1)),
m_visible(true),
m_fade(false),
m_markFrame(-1) {
	m_rgba[0] = m_rgba[1] = m_rgba[2] = Vec4(1, 1, 1, 1);
	m_fadeTime[0] = m_fadeTime[1] = 0.f;
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
	if (m_fade) {
		m_fadeTime[0] += dt;
		if (m_fadeTime[0] >= m_fadeTime[1]) {
			m_fade = false;
			m_fadeTime[0] = m_fadeTime[1];
			m_rgba[0] = m_rgba[2];
		} else {
			m_rgba[0] = math::Lerp(m_rgba[1], m_rgba[2], m_fadeTime[0]/m_fadeTime[1]);
		}
	}

	OnTick(time, dt);
}

void DrawModel::Fade(const Vec4 &rgba, float time) {
	if (time <= 0.f) {
		m_fade = false;
		m_rgba[0] = rgba;
	} else {
		m_rgba[1] = m_rgba[0];
		m_rgba[2] = rgba;
		m_fadeTime[0] = 0.f;
		m_fadeTime[1] = time;
		m_fade = true;
	}
}

DrawModel::DrawBatch::DrawBatch(DrawModel &model, int matId) : MBatchDraw(matId), m_model(&model) {
}

bool DrawModel::DrawBatch::GetTransform(Vec3 &pos, Vec3 &angles) const {
	return m_model ? m_model->GetTransform(pos, angles) : false;
}

///////////////////////////////////////////////////////////////////////////////

MeshDrawModel::Ref MeshDrawModel::New(
	WorldDraw &draw, 
	Entity *entity, 
	const r::Mesh::Ref &m, 
	int matId
) {
	Ref r(new (ZWorld) MeshDrawModel(entity));
	r->m_mesh = m;

	Batch::Ref b(new (ZWorld) Batch(*r, m, matId));
	draw.AddMaterial(matId);

	r->RefBatch(b);
	return r;
}

MeshDrawModel::MeshDrawModel(Entity *entity) : DrawModel(entity) {
}

MeshDrawModel::~MeshDrawModel() {
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
	WorldDraw &draw, 
	Entity *entity, 
	const r::MeshBundle::Ref &bundle
) {
	Ref r(new (ZWorld) MeshBundleDrawModel(entity));
	r->m_bundle = bundle;

	for (int i = 0; i < bundle->numMeshes; ++i) {
		Batch::Ref b(new (ZWorld) Batch(*r, bundle->Mesh(i), bundle->MaterialAsset(i)->id));
		draw.AddMaterial(bundle->MaterialAsset(i)->id);

		r->RefBatch(b);
	}

	return r;
}

MeshBundleDrawModel::MeshBundleDrawModel(Entity *entity) : DrawModel(entity) {
}

MeshBundleDrawModel::~MeshBundleDrawModel() {
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
	WorldDraw &draw, 
	Entity *entity, 
	const r::SkMesh::Ref &m
) {
	Ref r(new (ZWorld) SkMeshDrawModel(entity, m));

	asset::SkMaterialLoader::Ref loader = asset::SkMaterialLoader::Cast(m->asset);
	if (!loader)
		return Ref();

	for (int i = 0 ; i < m->numMeshes; ++i) {
		pkg::Asset::Ref material = loader->MaterialAsset(i);
		if (!material)
			continue;

		Batch::Ref b(new (ZWorld) Batch(*r, m, i, material->id));
		draw.AddMaterial(material->id);

		r->RefBatch(b);
	}

	return r;
}

SkMeshDrawModel::SkMeshDrawModel(Entity *entity, const r::SkMesh::Ref &m) : 
DrawModel(entity),
m_mesh(m),
m_motionType(ska::Ska::MT_None) {
}

SkMeshDrawModel::~SkMeshDrawModel() {
}

void SkMeshDrawModel::OnTick(float time, float dt)
{
	if (visible) {
		m_mesh->ska->Tick(
			dt, 
			true, 
			true, 
			Mat4::Identity,
			m_motionType,
			m_motion
		);
	}
}

SkMeshDrawModel::Batch::Batch(DrawModel &model, const r::SkMesh::Ref &m, int idx, int matId) :
DrawModel::DrawBatch(model, matId), m_idx(idx), m_m(m) {
}

Vec3 SkMeshDrawModel::BonePos(int idx) const {
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
