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
	virtual void RotateForCamera(const Camera &camera);
	virtual void RotateForCameraBasis();
	virtual void PushMatrix(const Vec3 &pos, const Vec3 &scale, const Vec3 &angles);
	virtual void PopMatrix();
	virtual void ReleaseArrayStates();

#if !defined(RAD_OPT_SHIP)
	virtual void DebugUploadVerts(
		const Vec3 *verts, 
		int numVerts
	);

	virtual int DebugUploadAutoTessTriIndices(int numVerts);

	virtual void DebugDrawLineLoop(int numVerts);
	virtual void DebugDrawIndexedTris(int numIndices);
#endif

	virtual void BindPostFXTargets(bool chain);
	virtual void BindPostFXQuad();
	virtual void DrawPostFXQuad();

	virtual void BindOverlay();
	
	virtual void DrawOverlay();
	virtual void CommitStates();
	virtual void Finish();

	virtual bool Project(const Vec3 &p, Vec3 &out);
	virtual Vec3 Unproject(const Vec3 &p);

protected:

	virtual RAD_DECLARE_GET(wireframe, bool) { 
		return r::gl.wireframe; 
	}

	virtual RAD_DECLARE_SET(wireframe, bool) { 
		r::gl.wireframe = value; 
	}

	virtual RAD_DECLARE_GET(numTris, int) { 
		return r::gl.numTris; 
	}

	virtual RAD_DECLARE_SET(numTris, int) { 
		r::gl.numTris = value; 
	}

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

#if !defined(RAD_OPT_SHIP)
	enum {
		kNumDebugIndices = Kilo,
		kDebugVertSize = sizeof(Vec3) * Kilo
	};
	void AllocateDebugVerts();
	r::GLVertexBuffer::Ref m_debugVerts;
	r::GLVertexBuffer::Ref m_debugIndices;
#endif

	int m_overlaySize[2];
	r::GLVertexBuffer::Ref m_overlayVB[2];
	r::GLVertexBuffer::Ref m_overlayIB[2];
};

} // world
