/*! \file DrawModel.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#pragma once

#include "../Packages/PackagesDef.h"
#include "../Renderer/Mesh.h"
#include "../Renderer/SkMesh.h"
#include "../Renderer/VtMesh.h"
#include "../Renderer/Sprites.h"
#include "../Lua/LuaRuntime.h"
#include "MBatchDraw.h"
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/PushPack.h>

namespace world {

class Entity;
class WorldDraw;

class RADENG_CLASS DrawModel : public lua::SharedPtr {
public:
	typedef boost::shared_ptr<DrawModel> Ref;
	typedef boost::weak_ptr<DrawModel> WRef;
	typedef zone_map<DrawModel*, Ref, ZWorldT>::type Map;
	typedef zone_map<int, int, ZWorldT>::type IntMap;

	DrawModel(Entity *entity);
	virtual ~DrawModel();

	void Tick(float time, float dt);
	void BlendTo(const Vec4 &rgba, float time);
	void ScaleTo(const Vec3 &scale, float time);
	void ReplaceMaterial(int src, int dst);
	void ReplaceMaterials(int dst);

	BBox TransformedBounds() const;

	RAD_DECLARE_READONLY_PROPERTY(DrawModel, entity, Entity*);
	RAD_DECLARE_PROPERTY(DrawModel, pos, const Vec3&, const Vec3&);
	RAD_DECLARE_PROPERTY(DrawModel, angles, const Vec3&, const Vec3&);
	RAD_DECLARE_PROPERTY(DrawModel, visible, bool, bool);
	RAD_DECLARE_READONLY_PROPERTY(DrawModel, bounds, const BBox&);
	RAD_DECLARE_READONLY_PROPERTY(DrawModel, scale, const Vec3&);
	RAD_DECLARE_READONLY_PROPERTY(DrawModel, rgba, const Vec4&);
	RAD_DECLARE_READONLY_PROPERTY(DrawModel, batches, const MBatchDraw::RefVec*);

protected:

	virtual void PushElements(lua_State *L);

	void RefBatch(const MBatchDraw::Ref &batch);
	virtual bool GetTransform(Vec3 &pos, Vec3 &angles) const;

	class DrawBatch : public MBatchDraw {
	public:
		DrawBatch(DrawModel &model, int matId);

		virtual BBox TransformedBounds() const {
			return m_model->TransformedBounds();
		}

	protected:
		virtual bool GetTransform(Vec3 &pos, Vec3 &angles) const;

		virtual RAD_DECLARE_GET(visible, bool) { return 
			m_model->visible; 
		}

		virtual RAD_DECLARE_GET(rgba, const Vec4&) { return 
			m_model->rgba; 
		}

		virtual RAD_DECLARE_GET(scale, const Vec3&) { return 
			m_model->scale; 
		}

	private:
		DrawModel *m_model;
	};

	virtual void OnTick(float time, float dt) {}

	virtual int lua_PushMaterialList(lua_State *L) = 0;

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
		return m_scale[0]; 
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

	RAD_DECLARE_GET(batches, const MBatchDraw::RefVec*) {
		return &m_batches;
	}

	static int lua_BlendTo(lua_State *L);
	static int lua_ReplaceMaterial(lua_State *L);
	static int lua_ReplaceMaterials(lua_State *L);
	static int lua_MaterialList(lua_State *L);
	static int lua_ScaleTo(lua_State *L);
	
	LUART_DECL_GETSET(Pos);
	LUART_DECL_GETSET(Angles);
	LUART_DECL_GET(Scale);
	LUART_DECL_GETSET(Visible);
	LUART_DECL_GETSET(Bounds);
	LUART_DECL_GET(RGBA);
	
	Vec3 m_r;
	Vec3 m_p;
	Vec3 m_scale[3];
	float m_scaleTime[2];
	BBox m_bounds;
	Entity *m_entity;
	MBatchDraw::RefVec m_batches;
	Vec4 m_rgba[3];
	float m_fadeTime[2];
	int m_markFrame;
	int m_visibleFrame;
	bool m_visible;
};

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS MeshDrawModel : public DrawModel {
public:
	typedef boost::shared_ptr<MeshDrawModel> Ref;
	typedef boost::weak_ptr<MeshDrawModel> WRef;

	static Ref New(Entity *entity, const r::Mesh::Ref &m, int matId);
	virtual ~MeshDrawModel();

	Ref CreateInstance();

	RAD_DECLARE_READONLY_PROPERTY(MeshDrawModel, mesh, const r::Mesh::Ref&);

protected:

	virtual void PushElements(lua_State *L);
	virtual int lua_PushMaterialList(lua_State *L);

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
		r::Mesh::Ref m_m;
	};

	static int lua_CreateInstance(lua_State *L);

	MeshDrawModel(Entity *entity);

	int m_matId;
	r::Mesh::Ref m_mesh;
};

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS MeshBundleDrawModel : public DrawModel {
public:
	typedef boost::shared_ptr<MeshBundleDrawModel> Ref;
	typedef boost::weak_ptr<MeshBundleDrawModel> WRef;

	static Ref New(Entity *entity, const pkg::AssetRef &meshBundle);
	virtual ~MeshBundleDrawModel();

	Ref CreateInstance();

	RAD_DECLARE_READONLY_PROPERTY(MeshBundleDrawModel, bundle, const pkg::AssetRef&);

protected:

	virtual void PushElements(lua_State *L);
	virtual int lua_PushMaterialList(lua_State *L);

private:

	RAD_DECLARE_GET(bundle, const pkg::AssetRef&) { 
		return m_asset; 
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
		r::Mesh::Ref m_m;
	};

	static int lua_CreateInstance(lua_State *L);

	MeshBundleDrawModel(Entity *entity);

	pkg::AssetRef m_asset;
};

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS SkMeshDrawModel : public DrawModel {
public:
	typedef boost::shared_ptr<SkMeshDrawModel> Ref;
	typedef boost::weak_ptr<SkMeshDrawModel> WRef;
	
	static Ref New(Entity *entity, const r::SkMesh::Ref &m);

	virtual ~SkMeshDrawModel();

	Ref CreateInstance();
	
	Vec3 BonePos(int idx) const;
	Vec3 WorldBonePos(int idx) const;

	RAD_DECLARE_PROPERTY(SkMeshDrawModel, motionScale, float, float);
	RAD_DECLARE_PROPERTY(SkMeshDrawModel, timeScale, float, float);
	RAD_DECLARE_READONLY_PROPERTY(SkMeshDrawModel, mesh, const r::SkMesh::Ref&);
	RAD_DECLARE_READONLY_PROPERTY(SkMeshDrawModel, deltaMotion, const ska::BoneTM*);
	RAD_DECLARE_READONLY_PROPERTY(SkMeshDrawModel, absMotion, const ska::BoneTM*);

protected:

	virtual void PushElements(lua_State *L);
	virtual int lua_PushMaterialList(lua_State *L);

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
		int m_idx;
		r::SkMesh::Ref m_m;
	};

	SkMeshDrawModel(Entity *entity, const r::SkMesh::Ref &m);

	static int lua_CreateInstance(lua_State *L);
	static int lua_FindBone(lua_State *L);
	static int lua_WorldBonePos(lua_State *L);
	static int lua_BonePos(lua_State *L);

	LUART_DECL_GETSET(TimeScale);
	LUART_DECL_GETSET(MotionScale);

	r::SkMesh::Ref m_mesh;
	float m_motionScale;
	float m_timeScale;
	bool m_instanced;
};

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS VtMeshDrawModel : public DrawModel {
public:
	typedef boost::shared_ptr<VtMeshDrawModel> Ref;
	typedef boost::weak_ptr<VtMeshDrawModel> WRef;
	
	static Ref New(Entity *entity, const r::VtMesh::Ref &m);

	virtual ~VtMeshDrawModel();

	Ref CreateInstance();
	

	RAD_DECLARE_PROPERTY(VtMeshDrawModel, timeScale, float, float);
	RAD_DECLARE_READONLY_PROPERTY(VtMeshDrawModel, mesh, const r::VtMesh::Ref&);

protected:

	virtual void PushElements(lua_State *L);
	virtual int lua_PushMaterialList(lua_State *L);

	virtual void OnTick(float time, float dt);

private:

	RAD_DECLARE_GET(timeScale, float) { 
		return m_timeScale;
	}

	RAD_DECLARE_SET(timeScale, float) {
		m_timeScale = value;
	}

	RAD_DECLARE_GET(mesh, const r::VtMesh::Ref&) { 
		return m_mesh; 
	}

	class Batch : public DrawModel::DrawBatch {
	public:
		typedef boost::shared_ptr<Batch> Ref;
		Batch(DrawModel &model, const r::VtMesh::Ref &m, int idx, int matId);

	protected:
		virtual void Bind(r::Shader *shader);
		virtual void CompileArrayStates(r::Shader &shader);
		virtual void FlushArrayStates(r::Shader *shader);
		virtual void Draw();

	private:
		int m_idx;
		r::VtMesh::Ref m_m;
	};

	VtMeshDrawModel(Entity *entity, const r::VtMesh::Ref &m);

	static int lua_CreateInstance(lua_State *L);

	LUART_DECL_GETSET(TimeScale);

	r::VtMesh::Ref m_mesh;
	float m_timeScale;
	bool m_instanced;
};

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS SpriteBatchDrawModel : public DrawModel {
public:

	typedef boost::shared_ptr<SpriteBatchDrawModel> Ref;
	typedef boost::weak_ptr<SpriteBatchDrawModel> WRef;
	
	static Ref New(
		Entity *entity, 
		const r::SpriteBatch::Ref &sprites,
		int matId
	);

	virtual ~SpriteBatchDrawModel();

	RAD_DECLARE_READONLY_PROPERTY(SpriteBatchDrawModel, spriteBatch, r::SpriteBatch*);

protected:

	SpriteBatchDrawModel(Entity *entity, const r::SpriteBatch::Ref &sprites, int matId);

	virtual void OnTick(float time, float dt);
	virtual void PushElements(lua_State *L);
	virtual int lua_PushMaterialList(lua_State *L);
	static int lua_AllocateSprite(lua_State *L);
	static int lua_FreeSprite(lua_State *L);
	static int lua_SetSpriteData(lua_State *L);

private:

	class Batch : public DrawModel::DrawBatch {
	public:
		typedef boost::shared_ptr<Batch> Ref;
		Batch(DrawModel &model, const r::SpriteBatch::Ref &m, int matId);

	protected:
		virtual void Bind(r::Shader *shader);
		virtual void CompileArrayStates(r::Shader &shader);
		virtual void FlushArrayStates(r::Shader *shader);
		virtual void Draw();

	private:
		r::SpriteBatch::Ref m_spriteBatch;
	};

	RAD_DECLARE_GET(spriteBatch, r::SpriteBatch*) {
		return m_spriteBatch.get();
	}

	r::SpriteBatch::Ref m_spriteBatch;
	int m_matId;
};


} // world

#include <Runtime/PopPack.h>
