// ViewModel.h
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

class RADENG_CLASS ViewModel
{
public:
	typedef boost::shared_ptr<ViewModel> Ref;
	typedef boost::weak_ptr<ViewModel> WRef;
	typedef zone_map<ViewModel*, Ref, ZWorldT>::type Map;

	ViewModel(Entity *entity);
	virtual ~ViewModel();

	void Tick(float time, float dt);
	void Fade(const Vec4 &rgba, float time);

	RAD_DECLARE_READONLY_PROPERTY(ViewModel, entity, Entity*);
	RAD_DECLARE_PROPERTY(ViewModel, pos, const Vec3&, const Vec3&);
	RAD_DECLARE_PROPERTY(ViewModel, angles, const Vec3&, const Vec3&);
	RAD_DECLARE_PROPERTY(ViewModel, scale, const Vec3&, const Vec3&);
	RAD_DECLARE_PROPERTY(ViewModel, visible, bool, bool);
	RAD_DECLARE_READONLY_PROPERTY(ViewModel, rgba, const Vec4&);
	RAD_DECLARE_READONLY_PROPERTY(ViewModel, xform, bool);

protected:

	void RefBatch(const MBatchDraw::Ref &batch);
	bool GetTransform(Vec3 &pos, Vec3 &angles) const;

	class ViewBatch : public MBatchDraw
	{
	public:
		ViewBatch(ViewModel &model, int matId);
	protected:
		virtual bool GetTransform(Vec3 &pos, Vec3 &angles) const;
		virtual RAD_DECLARE_GET(visible, bool) { return m_model->visible; }
		virtual RAD_DECLARE_GET(rgba, const Vec4&) { return m_model->rgba; }
		virtual RAD_DECLARE_GET(scale, const Vec3&) { return m_model->scale; }
		virtual RAD_DECLARE_GET(xform, bool) { return m_model->xform; }
	private:
		ViewModel *m_model;
	};

	virtual void OnTick(float time, float dt) {}

private:

	RAD_DECLARE_GET(entity, Entity*) { return m_entity; }
	RAD_DECLARE_GET(pos, const Vec3&) { return m_p; }
	RAD_DECLARE_SET(pos, const Vec3&) { m_p = value; }
	RAD_DECLARE_GET(angles, const Vec3&) { return m_r; }
	RAD_DECLARE_SET(angles, const Vec3&) { m_r = value; }
	RAD_DECLARE_GET(scale, const Vec3&) { return m_scale; }
	RAD_DECLARE_SET(scale, const Vec3&) { m_scale = value; }
	RAD_DECLARE_GET(visible, bool) { return m_visible && m_rgba[0][3] > 0.f; }
	RAD_DECLARE_SET(visible, bool) { m_visible = value; }
	RAD_DECLARE_GET(rgba, const Vec4&) { return m_rgba[0]; }
	RAD_DECLARE_GET(xform, bool) { return m_entity != 0; }

	Vec3 m_r;
	Vec3 m_p;
	Vec3 m_scale;
	Entity *m_entity;
	MBatchDraw::RefVec m_batches;
	Vec4 m_rgba[3];
	float m_fadeTime[2];
	bool m_fade;
	bool m_visible;
};

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS MeshViewModel : public ViewModel
{
public:
	typedef boost::shared_ptr<MeshViewModel> Ref;
	typedef boost::weak_ptr<MeshViewModel> WRef;

	static Ref New(WorldDraw &draw, Entity *entity, const r::Mesh::Ref &m, int matId);
	virtual ~MeshViewModel();

	RAD_DECLARE_READONLY_PROPERTY(MeshViewModel, mesh, const r::Mesh::Ref&);

private:

	RAD_DECLARE_GET(mesh, const r::Mesh::Ref&) { return m_mesh; }

	class Batch : public ViewModel::ViewBatch
	{
	public:
		typedef boost::shared_ptr<Batch> Ref;
		Batch(ViewModel &model, const r::Mesh::Ref &m, int matId);

	protected:
		virtual void Bind(r::Shader *shader);
		virtual void CompileArrayStates(r::Shader &shader);
		virtual void FlushArrayStates(r::Shader *shader);
		virtual void Draw();

	private:
		friend class MeshViewModel;
		r::Mesh::Ref m_m;
	};

	MeshViewModel(Entity *entity);

	r::Mesh::Ref m_mesh;
};

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS MeshBundleViewModel : public ViewModel
{
public:
	typedef boost::shared_ptr<MeshBundleViewModel> Ref;
	typedef boost::weak_ptr<MeshBundleViewModel> WRef;

	static Ref New(WorldDraw &draw, Entity *entity, const r::MeshBundle::Ref &bundle);
	virtual ~MeshBundleViewModel();

	RAD_DECLARE_READONLY_PROPERTY(MeshBundleViewModel, bundle, const r::MeshBundle::Ref&);

private:

	RAD_DECLARE_GET(bundle, const r::MeshBundle::Ref&) { return m_bundle; }

	class Batch : public ViewModel::ViewBatch
	{
	public:
		typedef boost::shared_ptr<Batch> Ref;
		Batch(ViewModel &model, const r::Mesh::Ref &m, int matId);

	protected:
		virtual void Bind(r::Shader *shader);
		virtual void CompileArrayStates(r::Shader &shader);
		virtual void FlushArrayStates(r::Shader *shader);
		virtual void Draw();

	private:
		friend class MeshBundleViewModel;
		r::Mesh::Ref m_m;
	};

	MeshBundleViewModel(Entity *entity);

	r::MeshBundle::Ref m_bundle;
};

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS SkMeshViewModel : public ViewModel
{
public:
	typedef boost::shared_ptr<SkMeshViewModel> Ref;
	typedef boost::weak_ptr<SkMeshViewModel> WRef;
	
	static Ref New(WorldDraw &draw, Entity *entity, const r::SkMesh::Ref &m);

	virtual ~SkMeshViewModel();
	
	Vec3 BonePos(int idx) const;

	RAD_DECLARE_READONLY_PROPERTY(SkMeshViewModel, mesh, const r::SkMesh::Ref&);
	RAD_DECLARE_PROPERTY(SkMeshViewModel, motionType, ska::Ska::MotionType, ska::Ska::MotionType);
	RAD_DECLARE_READONLY_PROPERTY(SkMeshViewModel, motion, const ska::BoneTM&);

protected:

	virtual void OnTick(float time, float dt);

private:

	RAD_DECLARE_GET(mesh, const r::SkMesh::Ref&) { return m_mesh; }
	RAD_DECLARE_GET(motionType, ska::Ska::MotionType) { return m_motionType; }
	RAD_DECLARE_SET(motionType, ska::Ska::MotionType) { m_motionType = value; }
	RAD_DECLARE_GET(motion, const ska::BoneTM&) { return m_motion; }

	class Batch : public ViewModel::ViewBatch
	{
	public:
		typedef boost::shared_ptr<Batch> Ref;
		Batch(ViewModel &model, const r::SkMesh::Ref &m, int idx, int matId);

	protected:
		virtual void Bind(r::Shader *shader);
		virtual void CompileArrayStates(r::Shader &shader);
		virtual void FlushArrayStates(r::Shader *shader);
		virtual void Draw();

	private:
		friend class MeshViewModel;
		int m_idx;
		r::SkMesh::Ref m_m;
	};

	SkMeshViewModel(Entity *entity, const r::SkMesh::Ref &m);

	ska::BoneTM m_motion;
	r::SkMesh::Ref m_mesh;
	ska::Ska::MotionType m_motionType;
};


} // world

#include <Runtime/PopPack.h>
