// DrawModel.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Renderer/Mesh.h"
#include "../Renderer/SkMesh.h"
#include "MBatchDraw.h"
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/PushPack.h>

namespace world {

class Entity;
class WorldDraw;

class RADENG_CLASS DrawModel {
public:
	typedef boost::shared_ptr<DrawModel> Ref;
	typedef boost::weak_ptr<DrawModel> WRef;
	typedef zone_map<DrawModel*, Ref, ZWorldT>::type Map;

	DrawModel(Entity *entity);
	virtual ~DrawModel();

	void Tick(float time, float dt);
	void Fade(const Vec4 &rgba, float time);

	RAD_DECLARE_READONLY_PROPERTY(DrawModel, entity, Entity*);
	RAD_DECLARE_PROPERTY(DrawModel, pos, const Vec3&, const Vec3&);
	RAD_DECLARE_PROPERTY(DrawModel, angles, const Vec3&, const Vec3&);
	RAD_DECLARE_PROPERTY(DrawModel, scale, const Vec3&, const Vec3&);
	RAD_DECLARE_PROPERTY(DrawModel, visible, bool, bool);
	RAD_DECLARE_PROPERTY(DrawModel, bounds, const BBox&, const BBox&);
	RAD_DECLARE_READONLY_PROPERTY(DrawModel, rgba, const Vec4&);

protected:

	void RefBatch(const MBatchDraw::Ref &batch);
	bool GetTransform(Vec3 &pos, Vec3 &angles) const;

	class DrawBatch : public MBatchDraw {
	public:
		DrawBatch(DrawModel &model, int matId);
	protected:
		virtual bool GetTransform(Vec3 &pos, Vec3 &angles) const;
		virtual RAD_DECLARE_GET(visible, bool) { return m_model->visible; }
		virtual RAD_DECLARE_GET(rgba, const Vec4&) { return m_model->rgba; }
		virtual RAD_DECLARE_GET(scale, const Vec3&) { return m_model->scale; }
		virtual RAD_DECLARE_GET(bounds, const BBox&) { return m_model->bounds; }
	private:
		DrawModel *m_model;
	};

	virtual void OnTick(float time, float dt) {}

private:

	friend class WorldDraw;

	RAD_DECLARE_GET(entity, Entity*) { 
		return m_entity; 
	}

	RAD_DECLARE_GET(pos, const Vec3&) { 
		return m_p; 
	}

	RAD_DECLARE_SET(pos, const Vec3&) { 
		m_p = value; 
	}

	RAD_DECLARE_GET(angles, const Vec3&) { 
		return m_r; 
	}

	RAD_DECLARE_SET(angles, const Vec3&) { 
		m_r = value; 
	}

	RAD_DECLARE_GET(scale, const Vec3&) { 
		return m_scale; 
	}

	RAD_DECLARE_SET(scale, const Vec3&) { 
		m_scale = value; 
	}

	RAD_DECLARE_GET(visible, bool) { 
		return m_visible && m_rgba[0][3] > 0.f; 
	}

	RAD_DECLARE_SET(visible, bool) { 
		m_visible = value; 
	}

	RAD_DECLARE_GET(rgba, const Vec4&) { 
		return m_rgba[0]; 
	}
	
	RAD_DECLARE_GET(bounds, const BBox&) {
		return m_bounds;
	}

	RAD_DECLARE_SET(bounds, const BBox&) {
		m_bounds = value;
	}
	
	Vec3 m_r;
	Vec3 m_p;
	Vec3 m_scale;
	BBox m_bounds;
	Entity *m_entity;
	MBatchDraw::RefVec m_batches;
	Vec4 m_rgba[3];
	float m_fadeTime[2];
	int m_markFrame;
	bool m_fade;
	bool m_visible;
};

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS MeshDrawModel : public DrawModel {
public:
	typedef boost::shared_ptr<MeshDrawModel> Ref;
	typedef boost::weak_ptr<MeshDrawModel> WRef;

	static Ref New(WorldDraw &draw, Entity *entity, const r::Mesh::Ref &m, int matId);
	virtual ~MeshDrawModel();

	RAD_DECLARE_READONLY_PROPERTY(MeshDrawModel, mesh, const r::Mesh::Ref&);

private:

	RAD_DECLARE_GET(mesh, const r::Mesh::Ref&) { 
		return m_mesh; 
	}

	class Batch : public DrawModel::DrawBatch {
	public:
		typedef boost::shared_ptr<Batch> Ref;
		Batch(DrawModel &model, const r::Mesh::Ref &m, int matId);

	protected:
		virtual void Bind(r::Shader *shader);
		virtual void CompileArrayStates(r::Shader &shader);
		virtual void FlushArrayStates(r::Shader *shader);
		virtual void Draw();

	private:
		friend class MeshDrawModel;
		r::Mesh::Ref m_m;
	};

	MeshDrawModel(Entity *entity);

	r::Mesh::Ref m_mesh;
};

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS MeshBundleDrawModel : public DrawModel {
public:
	typedef boost::shared_ptr<MeshBundleDrawModel> Ref;
	typedef boost::weak_ptr<MeshBundleDrawModel> WRef;

	static Ref New(WorldDraw &draw, Entity *entity, const r::MeshBundle::Ref &bundle);
	virtual ~MeshBundleDrawModel();

	RAD_DECLARE_READONLY_PROPERTY(MeshBundleDrawModel, bundle, const r::MeshBundle::Ref&);

private:

	RAD_DECLARE_GET(bundle, const r::MeshBundle::Ref&) { 
		return m_bundle; 
	}

	class Batch : public DrawModel::DrawBatch {
	public:
		typedef boost::shared_ptr<Batch> Ref;
		Batch(DrawModel &model, const r::Mesh::Ref &m, int matId);

	protected:
		virtual void Bind(r::Shader *shader);
		virtual void CompileArrayStates(r::Shader &shader);
		virtual void FlushArrayStates(r::Shader *shader);
		virtual void Draw();

	private:
		friend class MeshBundleDrawModel;
		r::Mesh::Ref m_m;
	};

	MeshBundleDrawModel(Entity *entity);

	r::MeshBundle::Ref m_bundle;
};

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS SkMeshDrawModel : public DrawModel {
public:
	typedef boost::shared_ptr<SkMeshDrawModel> Ref;
	typedef boost::weak_ptr<SkMeshDrawModel> WRef;
	
	static Ref New(WorldDraw &draw, Entity *entity, const r::SkMesh::Ref &m);

	virtual ~SkMeshDrawModel();
	
	Vec3 BonePos(int idx) const;

	RAD_DECLARE_PROPERTY(SkMeshDrawModel, motionScale, float, float);
	RAD_DECLARE_PROPERTY(SkMeshDrawModel, timeScale, float, float);
	RAD_DECLARE_READONLY_PROPERTY(SkMeshDrawModel, mesh, const r::SkMesh::Ref&);
	RAD_DECLARE_READONLY_PROPERTY(SkMeshDrawModel, deltaMotion, const ska::BoneTM*);
	RAD_DECLARE_READONLY_PROPERTY(SkMeshDrawModel, absMotion, const ska::BoneTM*);

protected:

	virtual void OnTick(float time, float dt);

private:

	RAD_DECLARE_GET(motionScale, float) { 
		return m_motionScale;
	}

	RAD_DECLARE_SET(motionScale, float) {
		m_motionScale = value;
	}

	RAD_DECLARE_GET(timeScale, float) { 
		return m_timeScale;
	}

	RAD_DECLARE_SET(timeScale, float) {
		m_timeScale = value;
	}

	RAD_DECLARE_GET(mesh, const r::SkMesh::Ref&) { 
		return m_mesh; 
	}

	RAD_DECLARE_GET(deltaMotion, const ska::BoneTM*) { 
		return m_mesh->ska->deltaMotion; 
	}

	RAD_DECLARE_GET(absMotion, const ska::BoneTM*) { 
		return m_mesh->ska->absMotion;
	}

	class Batch : public DrawModel::DrawBatch {
	public:
		typedef boost::shared_ptr<Batch> Ref;
		Batch(DrawModel &model, const r::SkMesh::Ref &m, int idx, int matId);

	protected:
		virtual void Bind(r::Shader *shader);
		virtual void CompileArrayStates(r::Shader &shader);
		virtual void FlushArrayStates(r::Shader *shader);
		virtual void Draw();

	private:
		friend class MeshDrawModel;
		int m_idx;
		r::SkMesh::Ref m_m;
	};

	SkMeshDrawModel(Entity *entity, const r::SkMesh::Ref &m);

	r::SkMesh::Ref m_mesh;
	float m_motionScale;
	float m_timeScale;
};


} // world

#include <Runtime/PopPack.h>
