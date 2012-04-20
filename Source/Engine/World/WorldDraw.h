// WorldDraw.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Renderer/Mesh.h"
#include "../Renderer/Material.h"
#include "../Assets/MaterialParser.h"
#include "BSPFile.h"
#include "WorldDrawDef.h"
#include "MBatchDraw.h"
#include "ViewModel.h"
#include "ScreenOverlay.h"
#include "PostProcess.h"
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/PushPack.h>

namespace world {

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
	virtual void RotateForCamera() = 0;
	virtual void RotateForCameraBasis() = 0;
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

	struct Counters
	{
		Counters()
		{
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
	bool AddBatch(const MBatchDraw::Ref &draw, bool worldRef=false);
	bool AddMaterial(int matId);
	void RemoveMaterial(int matId);
	void SwapMaterial(int src, int dst);
	void AttachViewModel(const ViewModel::Ref &ref);
	void RemoveViewModel(const ViewModel::Ref &ref);

	ScreenOverlay::Ref CreateScreenOverlay(int matId);
	
	void Tick(float dt);
	void Draw(Counters *counters = 0);
	
	bool Project(const Vec3 &p, Vec3 &out) { return m_rb->Project(p, out); }
	Vec3 Unproject(const Vec3 &p) { return m_rb->Unproject(p); }

	const PostProcessEffect::Ref &PostFX(int idx)
	{
		return m_postFX[idx];
	}

	RAD_DECLARE_READONLY_PROPERTY(WorldDraw, rb, const RB_WorldDraw::Ref&);
	RAD_DECLARE_PROPERTY(WorldDraw, wireframe, bool, bool);
	RAD_DECLARE_READONLY_PROPERTY(WorldDraw, viewModels, const ViewModel::Map&);

private:

	friend class ScreenOverlay;
	friend class World;

	bool AddStaticWorldMesh(const r::Mesh::Ref &m, int matId);

	RAD_DECLARE_GET(rb, const RB_WorldDraw::Ref&) { return m_rb; }
	RAD_DECLARE_GET(wireframe, bool) { return m_wireframe; }
	RAD_DECLARE_SET(wireframe, bool) { m_wireframe = value; }
	RAD_DECLARE_GET(viewModels, const ViewModel::Map&) { return m_viewModels; }

	class MStaticWorldMeshBatch : public MBatchDraw
	{
	public:
		typedef boost::shared_ptr<MStaticWorldMeshBatch> Ref;
		typedef zone_vector<Ref, ZWorldT>::type RefVec;

		MStaticWorldMeshBatch(const r::Mesh::Ref &m, int matId);

		void Mark(int frame) { m_frame = frame; }

	protected:
		virtual void Bind(r::Shader *shader);
		virtual void CompileArrayStates(r::Shader &shader);
		virtual void FlushArrayStates(r::Shader *shader);
		virtual void Draw();
		virtual bool VisMarked(int frame) const { return this->visible && (m_frame == frame); }
		virtual RAD_DECLARE_GET(visible, bool) { return true; }
		virtual RAD_DECLARE_GET(rgba, const Vec4&) { return s_rgba; }
		virtual RAD_DECLARE_GET(scale, const Vec3&) { return s_scale; }
		virtual RAD_DECLARE_GET(xform, bool) { return true; }
	private:
		r::Mesh::Ref m_m;

		int m_frame;
		static Vec4 s_rgba;
		static Vec3 s_scale;
	};

	struct Node
	{
		typedef zone_vector<Node, ZWorldT>::type Vec;
		BBox bounds;
		int parent;
		int planenum;
		int children[2];
	};

	struct Leaf
	{
		typedef zone_vector<Leaf, ZWorldT>::type Vec;
		BBox bounds;
		int parent;
		int firstModel;
		int numModels;
	};

	typedef zone_vector<Plane, ZWorldT>::type PlaneVec;

	details::MBatchRef AddMaterialBatch(int id);

	enum
	{
		NumFrustumPlanes = 6
	};

	void SetupFrustumPlanes();
	void VisMark(int node);
	bool ClipBounds(const BBox &bounds);
	int BoundsOnPlaneSide(const BBox &bounds, const Plane &p);
	void DrawView();
	void DrawViewModels();
	void DrawUI();
	void DrawOverlays();
	void DrawBatches(bool xform, bool wireframe);
	void PostProcess();
	void DrawBatches(r::Material::Sort sort, bool xform, bool wireframe);
	void DrawBatch(const details::MBatch &batch, bool xform, bool wireframe);
	void DrawOverlay(ScreenOverlay &overlay);
	void AddScreenOverlay(ScreenOverlay &overlay);
	void RemoveScreenOverlay(ScreenOverlay &overlay);

	int m_frame;
	bool m_wireframe;
	Counters m_counters;
	Plane m_frustum[NumFrustumPlanes];
	PostProcessEffect::Map m_postFX;
	MBatchDraw::RefVec m_drawRefs;
	details::MBatchIdMap m_batches;
	MStaticWorldMeshBatch::RefVec m_worldModels;
	pkg::Asset::Ref m_wireframeAsset;
	r::Material *m_wireframeMat;
	PlaneVec m_planes;
	Node::Vec m_nodes;
	Leaf::Vec m_leafs;
	ScreenOverlay::List m_overlays;
	RB_WorldDraw::Ref m_rb;
	Vec3 m_viewPos;
	World *m_world;
	ViewModel::Map m_viewModels;
};

} // world

#include <Runtime/PopPack.h>
