// WorldDraw.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Renderer/Mesh.h"
#include "../Renderer/Material.h"
#include "../Assets/MaterialParser.h"
#include "../Camera.h"
#include "BSPFile.h"
#include "WorldDef.h"
#include "WorldDrawDef.h"
#include "MBatchDraw.h"
#include "DrawModel.h"
#include "ScreenOverlay.h"
#include "PostProcess.h"
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/Base/ObjectPool.h>
#include <bitset>
#include <Runtime/PushPack.h>


namespace world {

///////////////////////////////////////////////////////////////////////////////

class ViewDef {
public:
	enum {
		kNumFrustumPlanes = 6
	};
	
	ViewDef() : mirror(false) {
	}

	Camera camera;
	PlaneVec frustum;

	dBSPLeaf *bspLeaf;
	dBSPAreaLeaf *areaLeaf;
	bool mirror;

	EntityBits marked;
	details::MBatchIdMap batches;
};

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS RB_WorldDraw
{
public:
	typedef boost::shared_ptr<RB_WorldDraw> Ref;

	static Ref New(World *w);

	RB_WorldDraw(World *w) : m_world(w) {}
	virtual ~RB_WorldDraw() {}

	RAD_DECLARE_READONLY_PROPERTY(RB_WorldDraw, world, World*);
	RAD_DECLARE_PROPERTY(RB_WorldDraw, wireframe, bool, bool);
	RAD_DECLARE_PROPERTY(RB_WorldDraw, numTris, int, int);
	RAD_DECLARE_PROPERTY(RB_WorldDraw, clearColorBuffer, bool, bool);

	virtual void BeginFrame() = 0;
	virtual void EndFrame() = 0;
	virtual int LoadMaterials() = 0;
	virtual int Precache() = 0;
	virtual void BindRenderTarget() = 0;
    virtual void ClearBackBuffer() = 0;
	virtual void ClearDepthBuffer() = 0;
	virtual void SetWorldStates() = 0;
	virtual void SetPerspectiveMatrix() = 0;
	virtual void SetScreenLocalMatrix() = 0;
	virtual void RotateForView(const ViewDef &view) = 0;
	virtual void RotateForViewBasis() = 0;
	virtual void PushMatrix(const Vec3 &pos, const Vec3 &scale, const Vec3 &angles) = 0;
	virtual void PopMatrix() = 0;
	virtual void ReleaseArrayStates() = 0;

	// Post Process FX
	virtual void BindPostFXTargets(bool chain) = 0;
	virtual void BindPostFXQuad() = 0;
	virtual void DrawPostFXQuad() = 0;

	virtual void BindOverlay() = 0;
	
	virtual void DrawOverlay() = 0;
	virtual void CommitStates() = 0;
	virtual void Finish() = 0;

	virtual bool Project(const Vec3 &p, Vec3 &out) = 0;
	virtual Vec3 Unproject(const Vec3 &p) = 0;

protected:

	virtual RAD_DECLARE_GET(wireframe, bool) = 0;
	virtual RAD_DECLARE_SET(wireframe, bool) = 0;
	virtual RAD_DECLARE_GET(numTris, int) = 0; 
	virtual RAD_DECLARE_SET(numTris, int) = 0;
	virtual RAD_DECLARE_GET(clearColorBuffer, bool) = 0;
	virtual RAD_DECLARE_SET(clearColorBuffer, bool) = 0;

private:

	RAD_DECLARE_GET(world, World*) { return m_world; }

	World *m_world;
};

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS WorldDraw
{
public:
	typedef boost::shared_ptr<WorldDraw> Ref;
	WorldDraw(World *w);
	~WorldDraw();

	struct Counters {
		Counters() {
			Clear();
		}

		void Clear();

		int drawnLeafs;
		int testedLeafs;
		int drawnNodes;
		int testedNodes;
		int numModels;
		int numTris;
		int numMaterials;
	};

	int LoadMaterials();
	void Init(const bsp_file::BSPFile::Ref &bsp);
	int Precache();
	void AddEffect(int id, const PostProcessEffect::Ref &fx);
	bool AddMaterial(int matId);
	void RemoveMaterial(int matId);
	
	ScreenOverlay::Ref CreateScreenOverlay(int matId);
	
	void Tick(float dt);
	void Draw(Counters *counters = 0);
	
	bool Project(const Vec3 &p, Vec3 &out) { 
		return m_rb->Project(p, out); 
	}

	Vec3 Unproject(const Vec3 &p) { 
		return m_rb->Unproject(p); 
	}

	const PostProcessEffect::Ref &PostFX(int idx) {
		return m_postFX[idx];
	}

	RAD_DECLARE_READONLY_PROPERTY(WorldDraw, rb, const RB_WorldDraw::Ref&);
	RAD_DECLARE_PROPERTY(WorldDraw, wireframe, bool, bool);
	
private:

	friend class ScreenOverlay;
	friend class World;

	class MStaticWorldMeshBatch : public MBatchDraw
	{
	public:
		typedef boost::shared_ptr<MStaticWorldMeshBatch> Ref;
		typedef zone_vector<Ref, ZWorldT>::type RefVec;

		MStaticWorldMeshBatch(const r::Mesh::Ref &m, int matId);

	protected:
		virtual void Bind(r::Shader *shader);
		virtual void CompileArrayStates(r::Shader &shader);
		virtual void FlushArrayStates(r::Shader *shader);
		virtual void Draw();

		virtual RAD_DECLARE_GET(visible, bool) { 
			return true; 
		}

		virtual RAD_DECLARE_GET(rgba, const Vec4&) { 
			return s_rgba; 
		}

		virtual RAD_DECLARE_GET(scale, const Vec3&) { 
			return s_scale; 
		}

		virtual RAD_DECLARE_GET(xform, bool) { 
			return true; 
		}

	private:
		r::Mesh::Ref m_m;

		static Vec4 s_rgba;
		static Vec3 s_scale;
	};

	RAD_DECLARE_GET(rb, const RB_WorldDraw::Ref&) { return m_rb; }
	RAD_DECLARE_GET(wireframe, bool) { return m_wireframe; }
	RAD_DECLARE_SET(wireframe, bool) { m_wireframe = value; }

	static void DeleteBatch(details::MBatch *batch);

	void AddStaticWorldMesh(const r::Mesh::Ref &m, int matId);

	details::MBatchRef AllocateBatch();
	details::MBatchRef AddMaterialRef(int id);
	details::MBatchRef AddViewBatch(ViewDef &view, int id);

	void FindViewArea(ViewDef &view);
	void SetupFrustumPlanes(ViewDef &view);
	void VisMarkArea(ViewDef &view, int nodeNum);
	void VisMarkAreaFlood(ViewDef &view, const PlaneVec &frustum, int areaNum);
	
	bool ClipBounds(const ViewDef &view, const BBox &bounds);
	void DrawView();
	void DrawView(ViewDef &view);
	void DrawUI();
	void DrawOverlays();
	void DrawBatches(ViewDef &view, bool wireframe);
	void PostProcess();
	void DrawBatches(ViewDef &view, r::Material::Sort sort, bool wireframe);
	void DrawBatch(const details::MBatch &batch, bool wireframe);
	void DrawOverlay(ScreenOverlay &overlay);
	void AddScreenOverlay(ScreenOverlay &overlay);
	void RemoveScreenOverlay(ScreenOverlay &overlay);
	void LinkEntity(Entity *entity, const BBox &bounds);
	void UnlinkEntity(Entity *entity);
	void LinkEntity(Entity *entity, const BBox &bounds, int nodeNum);
	dBSPAreaLeaf *LeafForPoint(const Vec3 &pos, int areaNum);
	dBSPAreaLeaf *LeafForPoint_r(const Vec3 &pos, int nodeNum);

	int m_frame;
	bool m_wireframe;
	Counters m_counters;
	PostProcessEffect::Map m_postFX;
	MStaticWorldMeshBatch::RefVec m_worldModels;
	pkg::Asset::Ref m_wireframeAsset;
	r::Material *m_wireframeMat;
	dBSPAreaNode::Vec m_nodes;
	dBSPAreaLeaf::Vec m_leafs;
	ScreenOverlay::List m_overlays;
	RB_WorldDraw::Ref m_rb;
	details::MBatchIdMap m_refMats;
	ObjectPool<details::MBatch> m_batchPool;
	World *m_world;
};

} // world

#include <Runtime/PopPack.h>
