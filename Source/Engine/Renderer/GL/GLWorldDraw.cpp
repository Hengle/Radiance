// GLWorldDraw.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "GLWorldDraw.h"
#include "../../Game/Game.h"
#include "../../App.h"
#include "../../Engine.h"
#include "GLState.h"
#include "GLTexture.h"
#include "../Shader.h"
#include "../../MathUtils.h"
#include <algorithm>
#undef min
#undef max

using namespace r;

namespace world {
	
enum {
	OverlayDiv = 26
};

RB_WorldDraw::Ref RB_WorldDraw::New(World *world) {
	return RB_WorldDraw::Ref(new (ZWorld) GLWorldDraw(world));
}

GLWorldDraw::GLWorldDraw(World *world) : RB_WorldDraw(world), 
m_activeRT(-1), m_bank(0), m_rtFB(false) {
	m_overlaySize[0] = m_overlaySize[1] = 0;
	m_rtSize[0] = m_rtSize[1] = 0;
#if defined(WORLD_DEBUG_DRAW)
	m_numDebugVerts = 0;
	m_numDebugIndices = 0;
#endif
}

GLWorldDraw::~GLWorldDraw() {
}

void GLWorldDraw::BeginFrame() {
}

void GLWorldDraw::EndFrame() {
	gls.invertCullFace = false;
}

void GLWorldDraw::ClearDepthBuffer() {
	gls.Set(kDepthWriteMask_Enable, -1, true); // for glClear()
	glClear(GL_DEPTH_BUFFER_BIT);
}

void GLWorldDraw::ClearBackBuffer() {
	gls.Set(kDepthWriteMask_Enable, -1, true); // for glClear()

#if defined(RAD_OPT_IOS)
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
#else
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
#endif
}

int GLWorldDraw::LoadMaterials() {
	return pkg::SR_Success;
}

int GLWorldDraw::Precache() {
	CreateScreenOverlay();
	BindRenderTarget();
	BindPostFXTargets(false);
	return pkg::SR_Success;
}

void GLWorldDraw::BindRenderTarget() {
	int vpx, vpy, vpw, vph;
	world->game->Viewport(vpx, vpy, vpw, vph);

	m_bank = (m_bank+1)%kNumBanks;

	if (vpw != m_rtSize[0] || vph != m_rtSize[1]) {
		m_rtSize[0] = vpw;
		m_rtSize[1] = vph;
		
		for (int i = 0; i < kNumBanks; ++i) {
			m_rts[i][0].reset(new GLRenderTarget(
				GL_TEXTURE_2D,
				GL_RGBA,
				GL_UNSIGNED_BYTE,
				m_rtSize[0],
				m_rtSize[1],
	#if defined(RAD_OPT_IOS)
				GL_DEPTH_COMPONENT24_ARB,
				m_rtSize[0]*m_rtSize[1]*7,
	#else
				GL_DEPTH_COMPONENT32_ARB,
				m_rtSize[0]*m_rtSize[1]*8,
	#endif
				TX_Filter
			));

			for (int k = 1; k < kNumRTs; ++k) {
				m_rts[i][k].reset(new GLRenderTarget(
					GL_TEXTURE_2D,
					GL_RGBA,
					GL_UNSIGNED_BYTE,
					m_rtSize[0],
					m_rtSize[1],
					0,
					m_rtSize[0]*m_rtSize[1]*4,
					TX_Filter
				));

				gls.BindBuffer(GL_FRAMEBUFFER_EXT, m_rts[i][k]->id[0]);
				CHECK_GL_ERRORS();
				gls.BindBuffer(GL_RENDERBUFFER_EXT, m_rts[i][0]->id[1]); // is this necessary?
				CHECK_GL_ERRORS();
				gl.FramebufferRenderbufferEXT(
					GL_FRAMEBUFFER_EXT,
					GL_DEPTH_ATTACHMENT_EXT,
					GL_RENDERBUFFER_EXT,
					m_rts[i][0]->id[1]
				);
				CHECK_GL_ERRORS();
				RAD_ASSERT(gl.CheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) == GL_FRAMEBUFFER_COMPLETE_EXT);
			}
		}
	}

	m_activeRT = 0;
	BindRTFB(m_activeRT);
	m_rtFB = true;
}

void GLWorldDraw::BindRTFB(int num) {
	GLuint fb = m_rts[m_bank][num]->id[0];
	gls.BindBuffer(GL_FRAMEBUFFER_EXT, fb);
	CHECK_GL_ERRORS();
#if defined(RAD_OPT_IOS)
    // won't be able to do depth here for each FB bind once we support MTS_FrameBuffer
    GLenum attachments[] = { GL_COLOR_ATTACHMENT0_EXT, GL_DEPTH_ATTACHMENT_EXT };
    glDiscardFramebufferEXT(GL_FRAMEBUFFER_EXT, 2, attachments);
#endif
	glViewport(0, 0, m_rtSize[0], m_rtSize[1]);
	CHECK_GL_ERRORS();
}

void GLWorldDraw::BindRTTX(int num) {
	for (int i = 0; i < kMaterialTextureSource_MaxIndices; ++i) {
		if (!gls.MaterialTextureSource(kMaterialTextureSource_Texture, i))
			gls.SetMTSource(kMaterialTextureSource_Texture, i, m_rts[m_bank][num]->tex);
	}
}

void GLWorldDraw::SetScreenLocalMatrix() {
	gl.MatrixMode(GL_PROJECTION);
	gl.LoadIdentity();

	int vpx, vpy, vpw, vph;
	world->game->Viewport(vpx, vpy, vpw, vph);
	gl.Ortho((double)vpx, (double)vpw, (double)(vpy+vph), (double)vpy, -1.0, 1.0);
	gls.invertCullFace = false;

	gl.MatrixMode(GL_MODELVIEW);
	gl.LoadIdentity();
}

void GLWorldDraw::SetPerspectiveMatrix() {
	gl.MatrixMode(GL_PROJECTION);
	gl.LoadIdentity();

	int vpx, vpy, vpw, vph;
	world->game->Viewport(vpx, vpy, vpw, vph);
	float yaspect = ((float)vph/(float)vpw);
	float xaspect = 1.f / yaspect;
	float yfov = world->camera->fov.get() * yaspect;

	gl.Perspective(yfov, xaspect, 4.0, world->camera->farClip.get());

	if (m_rtFB) { 
		// render target tc's are flipped about Y, so correct for this in the perspective transform.
		gl.Scalef(1.f, -1.f, 1.f);
		gls.invertCullFace = true;
	}
}

void RotateForDebugCamera() {
	gl.MatrixMode(GL_MODELVIEW);
	gl.LoadIdentity();
	gl.Rotatef(-90, 1, 0, 0);	    // put Z going up
    gl.Rotatef( 90, 0, 0, 1);	    // put Z going up

	gl.Rotatef(0, 1, 0, 0);
	gl.Rotatef(-90, 0, 1, 0);
	gl.Rotatef(0, 0, 0, 1);
	
	Vec3 pos(0.f, 0.f, 8000.f);
	gl.Translatef(-pos.X(), -pos.Y(), -pos.Z());
}

void GLWorldDraw::RotateForCamera(const Camera &camera) {
	RotateForCameraBasis();

	if (camera.quatMode) {
		Mat4 rot = Mat4::Rotation(camera.rot.get());
		gl.MultMatrix(rot);
	} else {
		const Vec3 &angles = camera.angles;
		gl.Rotatef(-angles.X(), 1, 0, 0);
		gl.Rotatef(-angles.Y(), 0, 1, 0);
		gl.Rotatef(-angles.Z(), 0, 0, 1);
	}

	const Vec3 &pos = camera.pos;
	gl.Translatef(-pos.X(), -pos.Y(), -pos.Z());
	gl.SetEye(&pos[0]);
}

void GLWorldDraw::RotateForCameraBasis() {
	gl.MatrixMode(GL_MODELVIEW);
	gl.LoadIdentity();
	gl.Rotatef(-90, 1, 0, 0);	    // put Z going up
    gl.Rotatef( 90, 0, 0, 1);	    // put Z going up
}

void GLWorldDraw::PushMatrix(const Vec3 &pos, const Vec3 &scale, const Vec3 &angles) {
	gl.PushMatrix();
	gl.Translatef(pos.X(), pos.Y(), pos.Z());
	gl.Rotatef(angles.Z(), 0, 0, 1);
	gl.Rotatef(angles.X(), 1, 0, 0);
	gl.Rotatef(angles.Y(), 0, 1, 0);
	gl.Scalef(scale.X(), scale.Y(), scale.Z());
}

void GLWorldDraw::PopMatrix() {
	gl.PopMatrix();
}

void GLWorldDraw::ReleaseArrayStates() {
	if (gl.vbos&&gl.vaos)
		gls.BindVertexArray(GLVertexArrayRef());
}

void GLWorldDraw::SetWorldStates() {
}

void GLWorldDraw::BindPostFXTargets(bool chain) {
	if (chain) {
		int d = (m_activeRT+1)%kNumRTs;
		BindRTFB(d);
		BindRTTX(m_activeRT);
		m_activeRT = d;
	} else { 
		// write to frame buffer
		int vpx, vpy, vpw, vph;
		world->game->Viewport(vpx, vpy, vpw, vph);

		App::Get()->engine->sys->r->BindFramebuffer();
#if defined(RAD_OPT_IOS)
		ClearBackBuffer(); // discard hint
#endif
		glViewport(0, 0, vpw, vph);
		BindRTTX(m_activeRT);
		m_rtFB = false;
	}
}

void GLWorldDraw::CreateScreenOverlay() {
	int vpx, vpy, vpw, vph;
	world->game->Viewport(vpx, vpy, vpw, vph);

	if (vpw != m_overlaySize[0] ||
		vph != m_overlaySize[1]) {
		RAD_ASSERT(vpw&&vph);
		CreateOverlay(vpw, vph, m_overlayVB[0], m_overlayIB[0], false);
		CreateOverlay(vpw, vph, m_overlayVB[1], m_overlayIB[1], true);
		m_overlaySize[0] = vpw;
		m_overlaySize[1] = vph;
	}
}

void GLWorldDraw::BindPostFXQuad() {
	CreateScreenOverlay();
	RAD_ASSERT(m_overlayVB);
	RAD_ASSERT(m_overlayIB);

	gls.DisableAllMGSources();
	gls.SetMGSource(
		kMaterialGeometrySource_Vertices,
		0,
		m_rtFB ? m_overlayVB[1] : m_overlayVB[0],
		2,
		GL_FLOAT,
		GL_FALSE,
		sizeof(OverlayVert),
		0
	);

	gls.SetMGSource(
		kMaterialGeometrySource_TexCoords,
		0,
		m_rtFB ? m_overlayVB[1] : m_overlayVB[0],
		2,
		GL_FLOAT,
		GL_FALSE,
		sizeof(OverlayVert),
		sizeof(float)*2
	);

	gls.BindBuffer(
		GL_ELEMENT_ARRAY_BUFFER_ARB,
		m_rtFB ? m_overlayIB[1] : m_overlayIB[0]
	);
}

void GLWorldDraw::DrawPostFXQuad() {
	gl.DrawElements(GL_TRIANGLES, (OverlayDiv-1)*(OverlayDiv-1)*6, GL_UNSIGNED_SHORT, 0);
	CHECK_GL_ERRORS();
}

void GLWorldDraw::BindOverlay() {
	BOOST_STATIC_ASSERT(sizeof(GLWorldDraw::OverlayVert)==16);
	
	CreateScreenOverlay();

	GLVertexBuffer::Ref vb = m_rtFB ? m_overlayVB[1] : m_overlayVB[0];
	GLVertexBuffer::Ref ib = m_rtFB ? m_overlayIB[1] : m_overlayIB[0];
	
	RAD_ASSERT(vb);
	RAD_ASSERT(ib);

	gls.DisableAllMGSources();
	gls.SetMGSource(
		kMaterialGeometrySource_Vertices,
		0,
		vb,
		2,
		GL_FLOAT,
		GL_FALSE,
		sizeof(OverlayVert),
		0
	);

	gls.SetMGSource(
		kMaterialGeometrySource_TexCoords,
		0,
		vb,
		2,
		GL_FLOAT,
		GL_FALSE,
		sizeof(OverlayVert),
		sizeof(float)*2
	);

	gls.BindBuffer(
		GL_ELEMENT_ARRAY_BUFFER_ARB,
		ib
	);
}

void GLWorldDraw::DrawOverlay() {
	gl.DrawElements(GL_TRIANGLES, (OverlayDiv-1)*(OverlayDiv-1)*6, GL_UNSIGNED_SHORT, 0);
	CHECK_GL_ERRORS();
}

void GLWorldDraw::CreateOverlay(
	int vpw, 
	int vph,
	GLVertexBuffer::Ref &_vb,
	GLVertexBuffer::Ref &_ib,
	bool invY
) {
	_vb.reset(
		new GLVertexBuffer(
			GL_ARRAY_BUFFER_ARB, 
			GL_STATIC_DRAW_ARB, 
			sizeof(OverlayVert)*OverlayDiv*OverlayDiv
		)
	);

	RAD_ASSERT(_vb);

	GLVertexBuffer::Ptr::Ref vb = _vb->Map();
	RAD_ASSERT(vb);
	OverlayVert *verts = (OverlayVert*)vb->ptr.get();

	float xInc = vpw / ((float)OverlayDiv-1);
	float yInc = vph / ((float)OverlayDiv-1);

	int x, y;
	float xf, yf;

	for (y = 0, yf = 0.f; y < OverlayDiv; ++y, yf += yInc) {
		for (x = 0, xf = 0.f; x < OverlayDiv; ++x, xf += xInc) {
			OverlayVert &v = verts[y*OverlayDiv+x];
			v.xy[0] = xf;
			v.xy[1] = yf;
	
			v.st[0] = xf / vpw;
			if (invY)
				v.st[1] = 1.f - (yf/vph);
			else
				v.st[1] = yf / vph;
		}
	}

	vb.reset(); // unmap

	// setup triangle indices

	_ib.reset(
		new GLVertexBuffer(
			GL_ELEMENT_ARRAY_BUFFER_ARB,
			GL_STATIC_DRAW_ARB,
			sizeof(U16)*(OverlayDiv-1)*(OverlayDiv-1)*6
		)
	);

	vb = _ib->Map();
	RAD_ASSERT(vb);
	U16 *indices = (U16*)vb->ptr.get();

	for (y = 0; y < OverlayDiv-1; ++y) {
		for (x = 0; x < OverlayDiv-1; ++x) {
			U16 *idx = &indices[y*(OverlayDiv-1)*6+x*6];

			idx[0] = (U16)(y*OverlayDiv+x);
			idx[1] = (U16)((y+1)*OverlayDiv+x);
			idx[2] = (U16)((y+1)*OverlayDiv+x+1);
			idx[3] = (U16)(y*OverlayDiv+x);
			idx[4] = (U16)((y+1)*OverlayDiv+x+1);
			idx[5] = (U16)(y*OverlayDiv+x+1);
		}
	}

	vb.reset(); // unmap
}

void GLWorldDraw::CommitStates() {
	App::Get()->engine->sys->r->CommitStates();
}

void GLWorldDraw::Finish() {
	gls.Set(kDepthWriteMask_Enable, -1, true); // for glClear()
}

bool GLWorldDraw::Project(const Vec3 &p, Vec3 &out) {
	int viewport[4];
	world->game->Viewport(viewport[0], viewport[1], viewport[2], viewport[3]);

	gl.MatrixMode(GL_PROJECTION);
	gl.PushMatrix();
	gl.MatrixMode(GL_MODELVIEW);
	gl.PushMatrix();

	bool tempRTFB = m_rtFB;
	m_rtFB = false;
	
	SetPerspectiveMatrix();
	RotateForCamera(*world->camera.get());

	m_rtFB = tempRTFB;
	
	bool r = ::Project(
		gl.GetModelViewProjectionMatrix(),
		viewport,
		p,
		out
	);

	gl.MatrixMode(GL_PROJECTION);
	gl.PopMatrix();
	gl.MatrixMode(GL_MODELVIEW);
	gl.PopMatrix();

	return r;
}

Vec3 GLWorldDraw::Unproject(const Vec3 &p) {
	int viewport[4];
	world->game->Viewport(viewport[0], viewport[1], viewport[2], viewport[3]);

	gl.MatrixMode(GL_PROJECTION);
	gl.PushMatrix();
	gl.MatrixMode(GL_MODELVIEW);
	gl.PushMatrix();

	bool tempRTFB = m_rtFB;
	m_rtFB = false;
	
	SetPerspectiveMatrix();
	RotateForCamera(*world->camera.get());
	
	m_rtFB = tempRTFB;

	Vec3 z = ::Unproject(
		gl.GetModelViewProjectionMatrix(),
		viewport,
		p
	);

	gl.MatrixMode(GL_PROJECTION);
	gl.PopMatrix();
	gl.MatrixMode(GL_MODELVIEW);
	gl.PopMatrix();

	return z;
}

} // world
