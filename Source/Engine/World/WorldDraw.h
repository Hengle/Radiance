/*! \file WorldDraw.h
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#pragma once

#include "../Renderer/Mesh.h"
#include "../Renderer/Material.h"
#include "../Renderer/Shader.h"
#include "../Assets/MaterialParser.h"
#include "../Camera.h"
#include "BSPFile.h"
#include "WorldDef.h"
#include "MBatchDraw.h"
#include "DrawModel.h"
#include "Light.h"
#include "ScreenOverlay.h"
#include "PostProcess.h"
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/Base/ObjectPool.h>
#include <bitset>
#include <Runtime/PushPack.h>

namespace world {

class FloorMove;

///////////////////////////////////////////////////////////////////////////////

class ViewDef {
public:
	enum {
		kNumFrustumPlanes = 6
	};
	
	ViewDef() : sky(false) {
	}

	Camera camera;
	Mat4 mvp;
	Mat4 mv;

	PlaneVec frustum;
	
	AreaBits areas;
	LightVec visLights;
	EntityPtrVec shadowEntities;
	MBatchOccupantPtrVec shadowOccupants;
	details::MBatchIdMap batches;

	int viewport[4];
	int area;
	bool sky;
};

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS RB_WorldDraw {
public:
	typedef boost::shared_ptr<RB_WorldDraw> Ref;

	static Ref New(World *w);

	RB_WorldDraw(World *w) : m_world(w) {}
	virtual ~RB_WorldDraw() {}

	RAD_DECLARE_READONLY_PROPERTY(RB_WorldDraw, world, World*);
	RAD_DECLARE_PROPERTY(RB_WorldDraw, numTris, int, int);

	virtual void BeginFrame() = 0;
	virtual void EndFrame() = 0;
	virtual int LoadMaterials() = 0;
	virtual int Precache() = 0;
	virtual void ClearBackBuffer() = 0;
	virtual void SetWorldStates() = 0;
	
	virtual Mat4 MakePerspectiveMatrix(
		float left, 
		float right, 
		float top, 
		float bottom, 
		float near, 
		float far,
		const Mat4 *bias
	) = 0;

	virtual void SetPerspectiveMatrix(
		const Camera &camera,
		const int viewport[4]
	) = 0;

	virtual void SetPerspectiveMatrix(const Mat4 &m) = 0;

	virtual void SetScreenLocalMatrix() = 0;

	virtual void SetOrthoMatrix(
		float left, 
		float right, 
		float top, 
		float bottom, 
		float near, 
		float far
	) = 0;

	virtual void RotateForCamera(const Camera &camera) = 0;
	virtual void RotateForCameraBasis() = 0;
	virtual void PushMatrix(const Vec3 &pos, const Vec3 &scale, const Vec3 &angles) = 0;
	virtual void PopMatrix() = 0;
	virtual void ReleaseArrayStates() = 0;

	virtual void BindLitMaterialStates(
		r::Material &mat,
		const Vec4 *scissorBounds
	) = 0;

	// Unified Shadows
	virtual void BindRenderTarget() = 0;

	virtual void BindUnifiedShadowRenderTarget(r::Material &shadowMaterial) = 0;
	virtual void BindUnifiedShadowTexture(r::Material &projectedMaterial) = 0;
	virtual void UnbindUnifiedShadowRenderTarget() = 0;
	
	// Post Process FX
	virtual Vec2 BindPostFXTargets(bool chain, const r::Material &mat, const Vec2 &srcScale, const Vec2 &dstScale) = 0;
	virtual void BindPostFXQuad() = 0;
	virtual void DrawPostFXQuad() = 0;

	virtual void BindOverlay() = 0;
	
	virtual void DrawOverlay() = 0;
	virtual void CommitStates() = 0;
	virtual void Finish() = 0;

	virtual bool Project(const Vec3 &p, Vec3 &out) = 0;
	virtual Vec3 Unproject(const Vec3 &p) = 0;

	virtual Mat4 GetModelViewMatrix() = 0;
	virtual Mat4 GetModelViewProjectionMatrix() = 0;

	static int LoadMaterial(const char *name, asset::MaterialBundle &mat);

#if defined(WORLD_DEBUG_DRAW)
	RAD_DECLARE_PROPERTY(RB_WorldDraw, wireframe, bool, bool);

	virtual void DebugUploadVerts(
		const Vec3 *verts, 
		int numVerts
	) = 0;

	virtual void DebugUploadIndices(
		const U16 *indices,
		int numIndices
	) = 0;

	virtual int DebugTesselateVerts(int numVerts) = 0;
	virtual void DebugDrawLineLoop(int numVerts) = 0;
	virtual void DebugDrawLineStrip(int numVerts) = 0;
	virtual void DebugDrawIndexedTris(int numIndices) = 0;
	virtual void DebugDrawIndexedLineLoop(int numIndices) = 0;
	virtual void DebugDrawIndexedLineStrip(int numIndices) = 0;
	virtual void DebugDrawTris(int num) = 0;
	virtual void DebugDrawPoly(int num) = 0;
#endif

protected:

	virtual RAD_DECLARE_GET(numTris, int) = 0; 
	virtual RAD_DECLARE_SET(numTris, int) = 0;

#if defined(WORLD_DEBUG_DRAW)
	virtual RAD_DECLARE_GET(wireframe, bool) = 0;
	virtual RAD_DECLARE_SET(wireframe, bool) = 0;
#endif

private:

	RAD_DECLARE_GET(world, World*) { return m_world; }

	World *m_world;
};

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS WorldDraw {
public:
	typedef boost::shared_ptr<WorldDraw> Ref;
	WorldDraw(World *w);
	~WorldDraw();

	struct Counters {
		Counters() {
			Clear();
		}

		void Clear();

		float fps;
		int area;
		int drawnAreas;
		int testedPortals;
		int drawnPortals;
		int testedWorldModels;
		int drawnWorldModels;
		int drawnEntities;
		int testedEntityModels;
		int drawnEntityModels;
		int drawnActors;
		int testedActorModels;
		int drawnActorModels;
		int testedLights;
		int visLights;
		int drawnLights;
		int numBatches;
		int numTris;
		int numMaterials;
	};

	int LoadMaterials();
	void Init(const bsp_file::BSPFile::Ref &bsp);
	int Precache();
	void AddEffect(int id, const PostProcessEffect::Ref &fx);
	bool AddMaterial(int matId);
		
	ScreenOverlay::Ref CreateScreenOverlay(int matId);
	Light::Ref CreateLight();
	
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

	int NumActivePostFX() const;

	RAD_DECLARE_READONLY_PROPERTY(WorldDraw, rb, const RB_WorldDraw::Ref&);
	RAD_DECLARE_PROPERTY(WorldDraw, uiOnly, bool, bool);
	
#if defined(WORLD_DEBUG_DRAW)
	void DebugAddEntityBBox(const BBox &bounds);
#endif

private:

	friend class ScreenOverlay;
	friend class World;
	friend class MBatchDraw;
	friend struct details::MBatch;
	
	class MStaticWorldMeshBatch : public MBatchDraw {
	public:
		typedef boost::shared_ptr<MStaticWorldMeshBatch> Ref;
		typedef zone_vector<Ref, ZWorldT>::type RefVec;

		MStaticWorldMeshBatch(
			WorldDraw &draw,
			const r::Mesh::Ref &m,
			const BBox &bounds,
			int matId
		);

		virtual BBox TransformedBounds() const {
			return m_bounds;
		}

	protected:
		virtual void Bind(r::Shader *shader);
		virtual void CompileArrayStates(r::Shader &shader);
		virtual void FlushArrayStates(r::Shader *shader);
		virtual void Draw();

		virtual RAD_DECLARE_GET(entity, Entity*) {
			return 0;
		}

		virtual RAD_DECLARE_GET(visible, bool) { 
			return true; 
		}

		virtual RAD_DECLARE_GET(rgba, const Vec4&) { 
			return s_rgba; 
		}

		virtual RAD_DECLARE_GET(scale, const Vec3&) { 
			return s_scale; 
		}

	private:
		r::Mesh::Ref m_m;
		BBox m_bounds;

		static Vec4 s_rgba;
		static Vec3 s_scale;
	};

	RAD_DECLARE_GET(rb, const RB_WorldDraw::Ref&) { 
		return m_rb; 
	}

	RAD_DECLARE_GET(uiOnly, bool) {
		return m_uiOnly;
	}

	RAD_DECLARE_SET(uiOnly, bool) {
		m_uiOnly = value;
	}
	
	static void DeleteBatch(details::MBatch *batch);

	void AddStaticWorldMesh(const r::Mesh::Ref &m, const BBox &bounds, int matId);

	details::MBatchRef AllocateBatch();
	details::MatRef *AddMaterialRef(int id);
	details::MBatchRef AddViewBatch(ViewDef &view, int id);

	void FindViewArea(ViewDef &view);
	
	void SetupPerspectiveFrustumPlanes(ViewDef &view);

	void SetupPerspectiveFrustumPlanes(
		ViewDef &view,
		const Vec4 &viewplanes,
		const Vec2 &zplanes
	);

	void SetupPerspectiveFrustumPlanes(
		PlaneVec &planes,
		const Camera &camera,
		const Vec4 &viewplanes,
		const Vec2 &zplanes
	);

	void SetupOrthoFrustumPlanes(
		ViewDef &view,
		const Vec4 &viewplanes,
		const Vec2 &zplanes
	);
	
	bool CalcScissorBounds(
		const ViewDef &view,
		const BBox &bounds,
		Vec4 &rect
	);

	void CalcViewplaneBounds(
		const ViewDef *view,
		const Mat4 &mv,
		const BBox &bounds,
		const Vec3 *radial,
		Vec4 &viewplanes,
		Vec2 &zplanes
	);

	void SetOrthoMatrix(
		const Vec4 &viewplanes,
		const Vec2 &zplanes
	);

	Mat4 MakePerspectiveMatrix(
		const Vec4 &viewplanes,
		const Vec2 &zplanes,
		bool txAddressBias
	);

	void VisMarkAreas(ViewDef &view);

	void VisMarkArea(
		ViewDef &view, 
		int area, 
		const StackWindingStackVec &volume, 
		const BBox &volumeBounds
	);
		
	void UpdateLightInteractions(ViewDef &view);
	void VisMarkShadowCasters(ViewDef &view);
	bool ClipShadowCasterBounds(ViewDef &view, const BBox &bounds, const Vec3 &lightPos);

	bool ClipBounds(
		const StackWindingStackVec &volume, 
		const BBox &volumeBounds, 
		const BBox &bounds
	);

	void DrawView();
	void DrawView(ViewDef &view);
	void DrawUI();
	void DrawOverlays();
	void PostProcess();
	
	void DrawViewBatches(ViewDef &view, bool wireframe);
	
	void DrawUnlitBatch(
		ViewDef &view,
		const details::MBatch &batch,
		bool wireframe
	);

	void DrawOverlay(ScreenOverlay &overlay);
	void AddScreenOverlay(ScreenOverlay &overlay);
	void RemoveScreenOverlay(ScreenOverlay &overlay);

	void LinkEntity(
		Entity &entity, 
		const BBox &bounds, 
		int nodeNum,
		dBSPLeaf &leaf,
		dBSPArea &area
	);

	void UnlinkEntity(Entity &entity);
	
	void LinkOccupant(
		MBatchOccupant &occupant, 
		const BBox &bounds, 
		int nodeNum,
		dBSPLeaf &leaf,
		dBSPArea &area
	);

	void UnlinkOccupant(MBatchOccupant &occupant);

	/*
	==============================================================================
	WorldDrawLight.cpp
	==============================================================================
	*/

	void InvalidateInteractions(Light &light);
	void InvalidateInteractions(Entity &entity);
	void InvalidateInteractions(MBatchOccupant &occupant);

	void LinkLight(Light &light, const BBox &bounds);

	void LinkLight(
		Light &light, 
		const BBox &bounds, 
		int nodeNum,
		dBSPLeaf &leaf,
		dBSPArea &area
	);

	void UnlinkLight(Light &light);

	details::LightInteraction *FindInteraction(
		Light &light,
		const MBatchDraw &batch
	);

	details::LightInteraction *FindInteraction(
		Light &light,
		const Entity &entity
	);

	details::LightInteraction *FindInteraction(
		Light &light,
		const MBatchOccupant &occupant
	);

	details::LightInteraction *CreateInteraction(
		Light &light,
		MBatchDraw &batch
	);

	details::LightInteraction *CreateInteraction(
		Light &light,
		Entity &entity,
		MBatchDraw &batch
	);

	details::LightInteraction *CreateInteraction(
		Light &light,
		MBatchOccupant &occupant,
		MBatchDraw &batch
	);

	details::LightInteraction *CreateInteraction(
		Light &light,
		Entity &entity
	);

	details::LightInteraction *CreateInteraction(
		Light &light,
		MBatchOccupant &occupant
	);

	void LinkInteraction(
		details::LightInteraction &interaction,
		details::LightInteraction *&headOnLight,
		details::LightInteraction *&headOnBatch
	);

	void UnlinkInteraction(
		details::LightInteraction &interaction,
		details::LightInteraction *&headOnLight,
		details::LightInteraction *&headOnBatch
	);

	void DrawUnshadowedLitBatch(
		ViewDef &view,
		const details::MBatch &batch
	);

	void DrawBatch(
		ViewDef &view,
		MBatchDraw &draw,
		r::Material &mat
	);

	void DrawUnshadowedLitBatchLights(
		ViewDef &view,
		MBatchDraw &draw,
		r::Material &mat
	);

	void GenLightDef(
		const Light &light,
		r::LightDef &lightDef,
		Mat4 *tx
	);

	void DrawViewUnifiedShadows(const ViewDef &view);
	bool DrawUnifiedEntityShadow(const ViewDef &view, const Entity &e);
	bool DrawUnifiedOccupantShadow(const ViewDef &view, const MBatchOccupant &o);

	bool DrawUnifiedShadowTexture(
		const ViewDef &view,
		const DrawModel::Map &models,
		const Vec3 &unifiedPos,
		float unifiedRadius
	);

	bool DrawUnifiedShadowTexture(
		const ViewDef &view,
		const MBatchDraw::RefVec &batches,
		const Vec3 &unifiedPos,
		float unifiedRadius
	);

	void DrawViewWithUnifiedShadow(
		const ViewDef &view,
		const Vec3 &unifiedPos,
		float unifiedRadius,
		const PlaneVec &shadowFrustum,
		const Mat4 &mv,
		const Vec4 &viewplanes,
		const Vec2 &zplanes,
		const void *occluder
	);

	void CalcUnifiedLightPosAndSize(
		const BBox &bounds,
		const details::LightInteraction *head,
		Vec3 &pos,
		float &radius
	);

	void UpdateLightInteractions(Light &light);
	void CleanupLights();
	void TickLights(float dt);
	static void DeleteLight(Light *light);

	ObjectPool<details::MBatch> m_batchPool;
	ObjectPool<details::MBatchDrawLink> m_linkPool;
	MemoryPool m_interactionPool;
	
	Counters m_counters;
	asset::MaterialBundle m_projected_M;
	asset::MaterialBundle m_shadow_M;
	PostProcessEffect::Map m_postFX;
	MStaticWorldMeshBatch::RefVec m_worldModels;
	ScreenOverlay::List m_overlays;
	RB_WorldDraw::Ref m_rb;
	details::MatRefMap m_refMats;
	World *m_world;
	Light *m_lights[2];
	int m_frame;
	int m_markFrame;
	bool m_uiOnly;
	bool m_init;
	
#if defined(WORLD_DEBUG_DRAW)

	typedef zone_vector<BBox, ZWorldT>::type BBoxVec;
	typedef zone_vector<Vec4, ZWorldT>::type Vec4Vec;
	typedef zone_vector<Vec3, ZWorldT>::type Vec3Vec;

	struct DebugVars {
		DebugVars() : lockVis(false) {}

		BBoxVec entityBBoxes;
		BBoxVec worldBBoxes;
		BBoxVec actorBBoxes;
		Vec3Vec unifiedLights;
		Vec3Vec projectedBoxPoints;
		Vec3Vec lights;
		Vec4Vec lightScissors;
		boost::array<Vec3, 5> unifiedLightAxis;
		Mat4 unifiedLightMatrix;
		StackWindingStackVec frustum;
		ClippedAreaVolumeStackVec frustumAreas;
		asset::MaterialBundle wireframe_M;
		boost::array<asset::MaterialBundle, 2> portal_M;
		asset::MaterialBundle worldBBox_M;
		asset::MaterialBundle entityBBox_M;
		asset::MaterialBundle actorBBox_M;
		asset::MaterialBundle waypoint_M;
		asset::MaterialBundle lightSphere_M;
		asset::MaterialBundle sphere_M;
		boost::array<asset::MaterialBundle, 6> lightPasses_M;
		r::Mesh::Ref lightMesh;
		Camera lockVisCamera;
		bool lockVis;
	};
	
	int  LoadDebugMaterials();
	void DebugDrawPortals(ViewDef &view);
	void DebugDrawAreaportals(int area);
	void DebugDrawLightScissors();
	void DebugDrawRects(const asset::MaterialBundle &material, const Vec4Vec &rects);
	void DebugDrawRect(const asset::MaterialBundle &material, const Vec4 &rect);
	void DebugDrawRectBatch(const asset::MaterialBundle &material, const Vec4 &rect);
	void DebugDrawBBoxes(const asset::MaterialBundle &material, const BBoxVec &bboxes, bool wireframe);
	void DebugDrawBBox(const asset::MaterialBundle &material, const BBox &bbox, bool wireframe);
	void DebugDrawBBoxBatch(const asset::MaterialBundle &material, const BBox &bbox, bool wireframe);
	void DebugDrawActiveWaypoints();
	void DebugDrawFloorMoves();
	void DebugDrawFloorMoveBatch(const asset::MaterialBundle &material, const FloorMove &move);
	void DebugDrawLightPasses(ViewDef &view);
	void DebugDrawLightPass(const details::MBatch &batch);
	void DebugDrawLightCounts(ViewDef &view);
	void DebugDrawLightCounts(const details::MBatch &batch);
	void DebugDrawFrustumVolumes(ViewDef &view);
	void DebugDrawLights();
	void DebugDrawUnifiedLights();
	bool DebugSetupUnifiedLightTextureMatrixView(ViewDef &view);
	bool DebugSetupUnifiedLightProjectionMatrixView(ViewDef &view);
	void DebugDrawProjectedBBoxPoints();
	void DebugDrawUnifiedLightAxis();

	DebugVars m_dbgVars;
#endif
	
};

} // world

#include <Runtime/PopPack.h>
