// GLWorldDraw.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../../World/World.h"
#include "GLVertexBuffer.h"
#include "GLRenderTarget.h"
#include "../Mesh.h"
#include "../Material.h"
#include "../../Packages/PackagesDef.h"
#include "../../Assets/MaterialParser.h"
#include <Runtime/PushPack.h>

namespace world {

class RADENG_CLASS GLWorldDraw : public RB_WorldDraw {
public:
	GLWorldDraw(World *world);
	virtual ~GLWorldDraw();

	virtual void BeginFrame();
	virtual void EndFrame();
	virtual int LoadMaterials();
	virtual int Precache();
	virtual void BindFramebuffer(bool discardHint, bool copy);
	virtual void BindRenderTarget();
    virtual void ClearBackBuffer();
	virtual void SetWorldStates();
	virtual void FlipMatrixHack(bool enable);

	virtual Mat4 MakePerspectiveMatrix(
		float left, 
		float right, 
		float top, 
		float bottom, 
		float near, 
		float far,
		bool txAddressBias
	);

	virtual void SetPerspectiveMatrix(
		const Camera &camera,
		const int viewport[4]
	);

	virtual void SetPerspectiveMatrix(const Mat4 &m);

	virtual void SetScreenLocalMatrix();

	virtual void SetOrthoMatrix(
		float left, 
		float right, 
		float top, 
		float bottom, 
		float near, 
		float far
	);

	virtual void RotateForCamera(const Camera &camera);
	virtual void RotateForCameraBasis();
	virtual void PushMatrix(const Vec3 &pos, const Vec3 &scale, const Vec3 &angles);
	virtual void PopMatrix();
	virtual void ReleaseArrayStates();

	virtual void BindLitMaterialStates(
		r::Material &mat,
		const Vec4 *scissorBounds
	);

	virtual void BeginFog();
	virtual void BeginFogDepthWrite(r::Material &fog, bool front);
	virtual void BeginFogDraw(r::Material &fog);
	virtual void EndFog();

	virtual void BeginUnifiedShadows();
	virtual void EndUnifiedShadows();

	virtual bool BindUnifiedShadowRenderTarget(
		r::Material &shadowMaterial
	);
	
	virtual bool BindUnifiedShadowTexture(
		r::Material &projectedMaterial
	);
	
	virtual Vec2 BindPostFXTargets(
		bool chain, 
		const r::Material &mat,
		const Vec2 &srcScale,
		const Vec2 &dstScale
	);

	virtual void BindPostFXQuad(const r::Material &mat);
	virtual void DrawPostFXQuad();

	virtual void BindOverlay(const r::Material &mat);
	
	virtual void DrawOverlay();
	virtual void CommitStates();
	virtual void Finish();

	virtual bool Project(const Vec3 &p, Vec3 &out);
	virtual Vec3 Unproject(const Vec3 &p);

	virtual Mat4 GetModelViewMatrix();
	virtual Mat4 GetModelViewProjectionMatrix(bool txAddressBias);

#if defined(WORLD_DEBUG_DRAW)
	virtual void DebugUploadVerts(
		const Vec3 *verts, 
		int numVerts
	);

	virtual void DebugUploadIndices(
		const U16 *indices,
		int numIndices
	);

	virtual int DebugTesselateVerts(int numVerts);
	virtual void DebugDrawLineLoop(int numVerts);
	virtual void DebugDrawLineStrip(int numVerts);
	virtual void DebugDrawIndexedTris(int numIndices);
	virtual void DebugDrawIndexedLineLoop(int numIndices);
	virtual void DebugDrawIndexedLineStrip(int numIndices);
	virtual void DebugDrawTris(int num);
	virtual void DebugDrawPoly(int num);
#endif

protected:

	virtual RAD_DECLARE_GET(numTris, int) { 
		return r::gl.numTris; 
	}

	virtual RAD_DECLARE_SET(numTris, int) { 
		r::gl.numTris = value; 
	}

#if defined(WORLD_DEBUG_DRAW)
	virtual RAD_DECLARE_GET(wireframe, bool) { 
		return r::gl.wireframe; 
	}

	virtual RAD_DECLARE_SET(wireframe, bool) { 
		r::gl.wireframe = value; 
	}
#endif

private:

	enum {
		kNumRTs = 2,
#if defined(PRERENDER_SHADOWS)
		kNumShadowTextures = 8
#else
		kNumShadowTextures = 2
#endif
	};

	void CalcBBoxScreenBounds(
		const BBox &bounds,
		Vec4 &rect,
		int viewport[4],
		bool modelViewOnly
	);

	void CreateScreenOverlay();
	
	void Copy(
		const r::GLRenderTarget::Ref &src,
		const r::GLRenderTarget::Ref &dst,
		r::GLRenderTarget::DiscardFlags discard
	);
	
	r::GLRenderTargetMultiCache::Ref m_rtCache;
	r::GLRenderTargetCache::Ref m_unifiedShadowRTCache;
	r::GLRenderTarget::Ref m_activeRT;
	r::GLRenderTarget::Ref m_shadowRT;
	r::GLRenderTarget::Ref m_framebufferRT;
	r::GLRenderTarget::Ref m_fogRT;
	
	struct OverlayVert {
		float xy[2];
		float st[2];
	};

	void CreateOverlay(
		int vpw, 
		int vph,
		r::Mesh::Ref &mesh,
		bool invY
	);

	void CreateRect();

	asset::MaterialBundle m_copy_M;
	r::Mesh::Ref m_rectMesh;
	r::Mesh::Ref m_overlay;

	int m_overlaySize[2];
	bool m_flipMatrix;

#if defined(WORLD_DEBUG_DRAW)
	enum {
		kDebugVertSize = sizeof(Vec3)
	};
	int m_numDebugVerts;
	int m_numDebugIndices;
	void AllocateDebugVerts(int num);
	void AllocateDebugIndices(int num);
	r::GLVertexBuffer::Ref m_debugVerts;
	r::GLVertexBuffer::Ref m_debugIndices;
#endif
};

} // world

#include <Runtime/PopPack.h>
