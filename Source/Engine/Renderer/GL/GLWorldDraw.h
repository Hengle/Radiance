// GLWorldDraw.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../../World/World.h"
#include "GLVertexBuffer.h"
#include "GLRenderTarget.h"
#include "../Material.h"
#include "../../Packages/PackagesDef.h"
#include "../../Assets/MaterialParser.h"

namespace world {

class RADENG_CLASS GLWorldDraw : public RB_WorldDraw {
public:
	GLWorldDraw(World *world);
	virtual ~GLWorldDraw();

	virtual void BeginFrame();
	virtual void EndFrame();
	virtual int LoadMaterials();
	virtual int Precache();
	virtual void BindRenderTarget();
    virtual void ClearBackBuffer();
	virtual void ClearDepthBuffer();
	virtual void SetWorldStates();
	virtual void SetPerspectiveMatrix();
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

	virtual bool CalcBoundsScissor(
		const BBox &bounds,
		Vec4 &rect
	);

	virtual void BindPostFXTargets(bool chain);
	virtual void BindPostFXQuad();
	virtual void DrawPostFXQuad();

	virtual void BindOverlay();
	
	virtual void DrawOverlay();
	virtual void CommitStates();
	virtual void Finish();

	virtual bool Project(const Vec3 &p, Vec3 &out);
	virtual Vec3 Unproject(const Vec3 &p);


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
		kNumBanks = 1
	};

	void CreateScreenOverlay();

	int m_bank;
	int m_activeRT;
	int m_rtSize[2];
	bool m_rtFB;
	boost::array<boost::array<r::GLRenderTarget::Ref, kNumRTs>, kNumBanks> m_rts;
	void BindRTFB(int num);
	void BindRTTX(int num);

	struct OverlayVert {
		float xy[2];
		float st[2];
	};

	void CreateOverlay(
		int vpw, 
		int vph,
		r::GLVertexBuffer::Ref &vb,
		r::GLVertexBuffer::Ref &ib,
		bool invY
	);

	int m_overlaySize[2];
	r::GLVertexBuffer::Ref m_overlayVB[2];
	r::GLVertexBuffer::Ref m_overlayIB[2];

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
