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
#undef near
#undef far

using namespace r;

namespace world {
	
enum {
	OverlayDiv = 26
};

RB_WorldDraw::Ref RB_WorldDraw::New(World *world) {
	return RB_WorldDraw::Ref(new (ZWorld) GLWorldDraw(world));
}

GLWorldDraw::GLWorldDraw(World *world) : RB_WorldDraw(world), 
m_activeRT(-1), m_bank(0), m_rtFB(false), m_shadowRTFB(false), m_activeShadowRT(-1) {
	m_overlaySize[0] = m_overlaySize[1] = 0;
	m_rtSize[0] = m_rtSize[1] = 0;
	m_shadowRTSize[0] = m_shadowRTSize[1] = 0;
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
	gls.Set(kDepthWriteMask_Enable|kScissorTest_Disable, -1, true); // for glClear()
	glClear(GL_DEPTH_BUFFER_BIT);
}

void GLWorldDraw::ClearBackBuffer() {
	gls.Set(kDepthWriteMask_Enable|kScissorTest_Disable, -1, true); // for glClear()

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
				TX_FilterBilinear,
				false
			));

			// for the remaining RTs, attach a shared depth buffer.
			for (int k = 1; k < kNumRTs; ++k) {
				m_rts[i][k].reset(new GLRenderTarget(
					GL_TEXTURE_2D,
					GL_RGBA,
					GL_UNSIGNED_BYTE,
					m_rtSize[0],
					m_rtSize[1],
					0,
					m_rtSize[0]*m_rtSize[1]*4,
					TX_FilterBilinear,
					false
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
	BindRTFB(m_activeRT, true);
	m_rtFB = true;
}

void GLWorldDraw::BindRTFB(int num, bool discardHint) {
	GLuint fb = m_rts[m_bank][num]->id[0];
	gls.BindBuffer(GL_FRAMEBUFFER_EXT, fb);
	CHECK_GL_ERRORS();
#if defined(RAD_OPT_IOS)
	if (discardHint) {
		// won't be able to do depth here for each FB bind once we support MTS_FrameBuffer
		GLenum attachments[] = { GL_COLOR_ATTACHMENT0_EXT, GL_DEPTH_ATTACHMENT_EXT };
		glDiscardFramebufferEXT(GL_FRAMEBUFFER_EXT, 2, attachments);
	}
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

void GLWorldDraw::BindUnifiedShadowRenderTarget(r::Material &shadowMaterial) {
	int vpx, vpy, vpw, vph;
	world->game->Viewport(vpx, vpy, vpw, vph);

	vpw >>= 1;
	vph >>= 1;

	if (vpw != m_shadowRTSize[0] || vph != m_shadowRTSize[1]) {
		m_shadowRTSize[0] = vpw;
		m_shadowRTSize[1] = vph;
		
		for (int i = 0; i < kNumShadowTextures; ++i) {
			m_unifiedShadowRTs[i].reset(new GLRenderTarget(
				GL_TEXTURE_2D,
				GL_RGBA,
				GL_UNSIGNED_BYTE,
				m_shadowRTSize[0],
				m_shadowRTSize[1],
	#if defined(RAD_OPT_IOS)
				GL_DEPTH_COMPONENT24_ARB,
				m_rtSize[0]*m_rtSize[1]*7,
	#else
				GL_DEPTH_COMPONENT32_ARB,
				m_rtSize[0]*m_rtSize[1]*8,
	#endif
				TX_Mipmap|TX_FilterTrilinear,
				false
			));
		}
	}

	BindShadowRTFB(++m_shadowRTFB);
	shadowMaterial.BindStates(kScissorTest_Enable);
}

void GLWorldDraw::UnbindUnifiedShadowRenderTarget() {
	m_shadowRTFB = false;
	if (m_rtFB) {
		BindRTFB(m_activeRT, false);
	} else {
		BindDefaultFB(false);
	}
}

void GLWorldDraw::BindUnifiedShadowTexture(
	r::Material &projectedTexture
) {
	BindShadowRTTX(m_shadowRTFB);
	projectedTexture.BindStates(0, kBlendModeSource_SrcAlpha|kBlendModeDest_InvSrcAlpha);
}

void GLWorldDraw::BindShadowRTFB(int num) {
	GLuint fb = m_unifiedShadowRTs[num]->id[0];
	gls.BindBuffer(GL_FRAMEBUFFER_EXT, fb);
	CHECK_GL_ERRORS();
#if defined(RAD_OPT_IOS)
    // won't be able to do depth here for each FB bind once we support MTS_FrameBuffer
    GLenum attachments[] = { GL_COLOR_ATTACHMENT0_EXT, GL_DEPTH_ATTACHMENT_EXT };
    glDiscardFramebufferEXT(GL_FRAMEBUFFER_EXT, 2, attachments);
#endif
	glViewport(0, 0, m_shadowRTSize[0], m_shadowRTSize[1]);
	gls.Set(kDepthWriteMask_Enable|kScissorTest_Disable, -1, true); // for glClear()
	glClearColor(0.f, 0.f, 0.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	gls.Scissor(
		2,
		2,
		m_shadowRTSize[0]-4,
		m_shadowRTSize[1]-4
	);
	CHECK_GL_ERRORS();
	m_shadowRTFB = true;
}

void GLWorldDraw::BindShadowRTTX(int num) {
	for (int i = 0; i < kMaterialTextureSource_MaxIndices; ++i) {
		if (!gls.MaterialTextureSource(kMaterialTextureSource_Texture, i))
			gls.SetMTSource(kMaterialTextureSource_Texture, i, m_unifiedShadowRTs[num]->tex);
	}

	UnbindUnifiedShadowRenderTarget();

	GLTexture::GenerateMipmaps(m_unifiedShadowRTs[num]->tex);
}

void GLWorldDraw::BindDefaultFB(bool discardHint) {
	// restore frame buffer
	int vpx, vpy, vpw, vph;
	world->game->Viewport(vpx, vpy, vpw, vph);
	App::Get()->engine->sys->r->BindFramebuffer();
#if defined(RAD_OPT_IOS)
	if (discardHint)
		ClearBackBuffer(); // discard hint
#endif
	glViewport(0, 0, vpw, vph);
	m_rtFB = false;
	m_shadowRTFB = false;
}

Mat4 GLWorldDraw::MakePerspectiveMatrix(
	float left, 
	float right, 
	float top, 
	float bottom, 
	float near, 
	float far,
	const Mat4 *bias
) {
	Mat4 m = Mat4::PerspectiveOffCenterRH(left, right, bottom, top, near, far);
	if (bias)
		m = m * (*bias);
	return m;
}

void GLWorldDraw::SetPerspectiveMatrix(
	const Camera &camera,
	const int viewport[4]
) {
	gl.MatrixMode(GL_PROJECTION);
	gl.LoadIdentity();

	float yaspect = ((float)viewport[3]/(float)viewport[2]);
	float xaspect = 1.f / yaspect;
	float yfov = camera.fov.get() * yaspect;

	gl.Perspective(yfov, xaspect, 4.0, camera.farClip.get());

	if (m_rtFB) { 
		// render target tc's are flipped about Y, so correct for this in the perspective transform.
		gl.Scalef(1.f, -1.f, 1.f);
		gls.invertCullFace = true;
	}

	gl.MatrixMode(GL_MODELVIEW);
}

void GLWorldDraw::SetPerspectiveMatrix(
	const Mat4 &m
) {
	gl.MatrixMode(GL_PROJECTION);
	gl.LoadIdentity();
	gl.MultMatrix(m);

	if (m_rtFB) { 
		// render target tc's are flipped about Y, so correct for this in the perspective transform.
		gl.Scalef(1.f, -1.f, 1.f);
		gls.invertCullFace = true;
	}

	gl.MatrixMode(GL_MODELVIEW);
}

void GLWorldDraw::SetScreenLocalMatrix() {
	gl.MatrixMode(GL_PROJECTION);
	gl.LoadIdentity();

	int vpx, vpy, vpw, vph;
	world->game->Viewport(vpx, vpy, vpw, vph);
	gl.Ortho((double)vpx, (double)(vpx+vpw), (double)(vpy+vph), (double)vpy, -1.0, 1.0);

	gls.invertCullFace = false;

	gl.MatrixMode(GL_MODELVIEW);
	gl.LoadIdentity();
}

void GLWorldDraw::SetOrthoMatrix(
	float left,
	float right,
	float top,
	float bottom,
	float near, 
	float far
) {
	gl.MatrixMode(GL_PROJECTION);
	gl.LoadIdentity();
	gl.Ortho(
		(double)left, 
		(double)right,
		(double)bottom,
		(double)top,
		(double)near,
		(double)far
	);

	if (m_rtFB) { 
		// render target tc's are flipped about Y, so correct for this in the transform.
		gl.Scalef(1.f, -1.f, 1.f);
		gls.invertCullFace = true;
	}

	gl.MatrixMode(GL_MODELVIEW);
}

void GLWorldDraw::RotateForCamera(const Camera &camera) {
	RotateForCameraBasis();

	if (camera.quatMode) {
		Mat4 rot = Mat4::Rotation(camera.rot.get());
		rot.Transpose();
		gl.MultMatrix(rot);
	} else {
		const Vec3 &angles = camera.angles;
		gl.Rotatef(-angles.X(), 1, 0, 0);
		gl.Rotatef(-angles.Y(), 0, 1, 0);
		gl.Rotatef(-angles.Z(), 0, 0, 1);
	}

	const Vec3 &pos = camera.pos;
	gl.Translatef(-pos.X(), -pos.Y(), -pos.Z());
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

void GLWorldDraw::BindLitMaterialStates(
	r::Material &mat,
	const Vec4 *rect
) {
	int flags = 0;
	
	if (mat.depthWrite)
		flags |= kDepthTest_Equal|kDepthWriteMask_Disable;

	if (rect) {
		boost::array<int, 4> viewport;
		world->game->Viewport(viewport[0], viewport[1], viewport[2], viewport[3]);

		flags |= kScissorTest_Enable;
		gls.Scissor(
			FloatToInt((*rect)[0]),
			viewport[3]-FloatToInt((*rect)[3]),
			FloatToInt((*rect)[2])-FloatToInt((*rect)[0]),
			FloatToInt((*rect)[3]-(*rect)[1])
		);
	}

	int bm = 0;
	if (mat.blendMode == Material::kBlendMode_None)
		bm = kBlendModeSource_One|kBlendModeDest_One; // additive

	mat.BindStates(flags, bm);
}

void GLWorldDraw::SetWorldStates() {
}

void GLWorldDraw::BindPostFXTargets(bool chain) {
	if (chain) {
		int d = (m_activeRT+1)%kNumRTs;
		BindRTFB(d, true);
		BindRTTX(m_activeRT);
		m_activeRT = d;
	} else { 
		BindDefaultFB(true);
		BindRTTX(m_activeRT);
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
	
	SetPerspectiveMatrix(*world->camera.get(), viewport);
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
	
	SetPerspectiveMatrix(*world->camera.get(), viewport);
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

Mat4 GLWorldDraw::GetModelViewMatrix() {
	return gl.GetModelViewMatrix();
}

Mat4 GLWorldDraw::GetModelViewProjectionMatrix() {
	return gl.GetModelViewProjectionMatrix();
}

} // world
