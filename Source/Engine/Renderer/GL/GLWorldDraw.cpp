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

GLWorldDraw::GLWorldDraw(World *world) : RB_WorldDraw(world) {
	m_overlaySize[0] = m_overlaySize[1] = 0;
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

void GLWorldDraw::ClearBackBuffer() {
	gls.Set(kDepthWriteMask_Enable|kScissorTest_Disable, -1, true); // for glClear()
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}

int GLWorldDraw::LoadMaterials() {
	int r = LoadMaterial("Sys/Copy_M", m_copy_M);
	return r;
}

int GLWorldDraw::Precache() {
	CreateScreenOverlay();
	return pkg::SR_Success;
}

void GLWorldDraw::BindRenderTarget() {
	int vpx, vpy, vpw, vph;
	world->game->Viewport(vpx, vpy, vpw, vph);

	if (!m_framebufferRT) {
#if defined(RAD_OPT_OGLES)
		const GLenum depth = GL_DEPTH_COMPONENT24_ARB;
		const int depthBytesPP = 3;
#else
		const GLenum depth = GL_DEPTH_COMPONENT32_ARB;
		const int depthBytesPP = 4;
#endif

		m_framebufferRT.reset(new (ZRender) GLRenderTarget(
			GL_TEXTURE_2D,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			vpw,
			vph,
			depth,
			(depthBytesPP+4)*vpw*vph,
			TX_FilterBilinear,
			false
		));
	}

	m_activeRT = m_framebufferRT;
	m_activeRT->BindFramebuffer(GLRenderTarget::kDiscard_All);
}

Vec2 GLWorldDraw::BindPostFXTargets(bool chain, const r::Material &mat, const Vec2 &srcScale, const Vec2 &dstScale) {
	
	if (!m_rtCache) {
#if defined(RAD_OPT_OGLES)
		const GLenum depth = GL_DEPTH_COMPONENT24_ARB;
		const int depthBytesPP = 3;
#else
		const GLenum depth = GL_DEPTH_COMPONENT32_ARB;
		const int depthBytesPP = 4;
#endif

		m_rtCache.reset(
			new (ZRender) GLRenderTargetMultiCache(
			kNumRTs,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			depth,
			GLRenderTargetCache::kDepthInstanceMode_Shared,
			TX_FilterBilinear,
			4,
			depthBytesPP
		));
	}

	int vpx, vpy, vpw, vph;
	world->game->Viewport(vpx, vpy, vpw, vph);

	int srcW = vpw * srcScale[0];
	int srcH = vph * srcScale[1];
	
	GLRenderTarget::Ref curRT(m_activeRT);
				
	if ((curRT->tex->width != srcW) || (curRT->tex->height != srcH)) {
		m_activeRT = m_rtCache->NextRenderTarget(srcW, srcH);
		Copy(curRT, m_activeRT);
		curRT = m_activeRT;
	}
		
	curRT->BindTexture(0);
	m_framebufferRT->BindTexture(1);

	if (chain) {
		// select output size
		int dstW = vpw * dstScale[0];
		int dstH = vph * dstScale[1];

		m_activeRT = m_rtCache->NextRenderTarget(dstW, dstH);
		m_activeRT->BindFramebuffer(GLRenderTarget::kDiscard_All);
	} else {
		GLRenderTarget::DiscardFramebuffer(GLRenderTarget::kDiscard_Depth); // don't need depth anymore.
		BindDefaultFB(true);
		m_activeRT.reset();
	}

	return Vec2(1.f/srcW, 1.f/srcH);
}

void GLWorldDraw::Copy(
	const r::GLRenderTarget::Ref &src,
	const r::GLRenderTarget::Ref &dst
) {
	ReleaseArrayStates();

	m_copy_M.material->BindTextures(m_copy_M.loader);
	src->BindTexture(0);
	dst->BindFramebuffer(GLRenderTarget::kDiscard_All);

	m_copy_M.material->shader->Begin(r::Shader::kPass_Default, *m_copy_M.material);

	gl.MatrixMode(GL_PROJECTION);
	gl.PushMatrix();
	gl.LoadIdentity();

	gl.Ortho(0.0, 1.0, 1.0, 0.0, -1.0, 1.0);

	gls.invertCullFace = false;

	gl.MatrixMode(GL_MODELVIEW);
	gl.PushMatrix();
	gl.LoadIdentity();

	gls.DisableAllMGSources();
	gls.SetMGSource(
		kMaterialGeometrySource_Vertices,
		0,
		m_rectVB,
		2,
		GL_FLOAT,
		GL_FALSE,
		sizeof(OverlayVert),
		0
	);

	gls.SetMGSource(
		kMaterialGeometrySource_TexCoords,
		0,
		m_rectVB,
		2,
		GL_FLOAT,
		GL_FALSE,
		sizeof(OverlayVert),
		sizeof(float)*2
	);

	gls.BindBuffer(
		GL_ELEMENT_ARRAY_BUFFER_ARB,
		m_rectIB
	);

	m_copy_M.material->BindStates();
	m_copy_M.material->shader->BindStates();
	gls.Commit();

	gl.DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
	CHECK_GL_ERRORS();
	m_copy_M.material->shader->End();

	gl.MatrixMode(GL_PROJECTION);
	gl.PopMatrix();

	gl.MatrixMode(GL_MODELVIEW);
	gl.PopMatrix();
}

void GLWorldDraw::BindUnifiedShadowRenderTarget(r::Material &shadowMaterial) {

	if (!m_unifiedShadowRTCache) {
#if defined(RAD_OPT_OGLES)
		const GLenum depth = GL_DEPTH_COMPONENT24_ARB;
		const int depthBytesPP = 3;
#else
		const GLenum depth = GL_DEPTH_COMPONENT32_ARB;
		const int depthBytesPP = 4;
#endif

		int vpx, vpy, vpw, vph;
		world->game->Viewport(vpx, vpy, vpw, vph);

		vpw = PowerOf2(vpw>>1)>>1;
		vph = PowerOf2(vph>>1)>>1;

		m_unifiedShadowRTCache.reset(new (ZRender) GLRenderTargetCache(
			vpw,
			vph,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			depth,
			GLRenderTargetCache::kDepthInstanceMode_Shared,
			TX_Mipmap|TX_FilterTrilinear,
			4,
			depthBytesPP
		));

		m_unifiedShadowRTCache->CreateRenderTargets(kNumShadowTextures);
	}

	if (m_shadowRT)
		GLRenderTarget::DiscardFramebuffer(GLRenderTarget::kDiscard_All);

	m_shadowRT = m_unifiedShadowRTCache->NextRenderTarget();
	m_shadowRT->BindFramebuffer(GLRenderTarget::kDiscard_All);

	gls.Scissor(
		8,
		8,
		m_shadowRT->tex->size-16,
		m_shadowRT->tex->size-16
	);
	CHECK_GL_ERRORS();

	shadowMaterial.BindStates(kScissorTest_Enable);
}

void GLWorldDraw::UnbindUnifiedShadowRenderTarget() {

	RAD_ASSERT(m_shadowRT);
	GLRenderTarget::DiscardFramebuffer(GLRenderTarget::kDiscard_All);
	m_shadowRT.reset();

	if (m_activeRT) {
		m_activeRT->BindFramebuffer(GLRenderTarget::kDiscard_None);
	} else {
		BindDefaultFB(false);
	}
}

void GLWorldDraw::BindUnifiedShadowTexture(
	r::Material &projectedTexture
) {
	GLRenderTarget::DiscardFramebuffer(GLRenderTarget::kDiscard_Depth);
	m_shadowRT->BindTexture(0);
	GLTexture::GenerateMipmaps(m_shadowRT->tex);
	m_shadowRT.reset();

	if (m_activeRT) {
		m_activeRT->BindFramebuffer(GLRenderTarget::kDiscard_None);
	} else {
		BindDefaultFB(false);
	}

	projectedTexture.BindStates(0, kBlendModeSource_SrcAlpha|kBlendModeDest_InvSrcAlpha);
}

void GLWorldDraw::BindDefaultFB(bool discardHint) {
	int vpx, vpy, vpw, vph;
	world->game->Viewport(vpx, vpy, vpw, vph);
	App::Get()->engine->sys->r->BindFramebuffer();
//#if defined(RAD_OPT_OGLES)
	if (discardHint)
		ClearBackBuffer(); // discard hint
//#endif
	glViewport(0, 0, vpw, vph);
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

	if (m_activeRT) { 
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

	if (m_activeRT) { 
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

	CreateRect(m_rectVB, m_rectIB);
}

void GLWorldDraw::BindPostFXQuad() {
	CreateScreenOverlay();
	RAD_ASSERT(m_overlayVB);
	RAD_ASSERT(m_overlayIB);

	gls.DisableAllMGSources();
	gls.SetMGSource(
		kMaterialGeometrySource_Vertices,
		0,
		m_activeRT ? m_overlayVB[1] : m_overlayVB[0],
		2,
		GL_FLOAT,
		GL_FALSE,
		sizeof(OverlayVert),
		0
	);

	gls.SetMGSource(
		kMaterialGeometrySource_TexCoords,
		0,
		m_activeRT ? m_overlayVB[1] : m_overlayVB[0],
		2,
		GL_FLOAT,
		GL_FALSE,
		sizeof(OverlayVert),
		sizeof(float)*2
	);

	gls.BindBuffer(
		GL_ELEMENT_ARRAY_BUFFER_ARB,
		m_activeRT ? m_overlayIB[1] : m_overlayIB[0]
	);
}

void GLWorldDraw::DrawPostFXQuad() {
	gl.DrawElements(GL_TRIANGLES, (OverlayDiv-1)*(OverlayDiv-1)*6, GL_UNSIGNED_SHORT, 0);
	CHECK_GL_ERRORS();
}

void GLWorldDraw::BindOverlay() {
	BOOST_STATIC_ASSERT(sizeof(GLWorldDraw::OverlayVert)==16);
	
	CreateScreenOverlay();

	GLVertexBuffer::Ref vb = m_activeRT ? m_overlayVB[1] : m_overlayVB[0];
	GLVertexBuffer::Ref ib = m_activeRT ? m_overlayIB[1] : m_overlayIB[0];
	
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

void GLWorldDraw::CreateRect(r::GLVertexBuffer::Ref &_vb, r::GLVertexBuffer::Ref &_ib) {
	_vb.reset(
		new GLVertexBuffer(
			GL_ARRAY_BUFFER_ARB, 
			GL_STATIC_DRAW_ARB, 
			sizeof(OverlayVert)*4
		)
	);

	RAD_ASSERT(_vb);

	GLVertexBuffer::Ptr::Ref vb = _vb->Map();
	RAD_ASSERT(vb);
	OverlayVert *verts = (OverlayVert*)vb->ptr.get();

	// note flipped TC's
	verts[0].st[0] = 0.f;
	verts[0].st[1] = 1.f;
	verts[0].xy[0] = 0.f;
	verts[0].xy[1] = 0.f;
	
	verts[1].st[0] = 1.f;
	verts[1].st[1] = 1.f;
	verts[1].xy[0] = 1.f;
	verts[1].xy[1] = 0.f;

	verts[2].st[0] = 1.f;
	verts[2].st[1] = 0.f;
	verts[2].xy[0] = 1.f;
	verts[2].xy[1] = 1.f;

	verts[3].st[0] = 0.f;
	verts[3].st[1] = 0.f;
	verts[3].xy[0] = 0.f;
	verts[3].xy[1] = 1.f;

	vb.reset(); // unmap

	// setup triangle indices

	_ib.reset(
		new GLVertexBuffer(
			GL_ELEMENT_ARRAY_BUFFER_ARB,
			GL_STATIC_DRAW_ARB,
			sizeof(U16)*6
		)
	);

	vb = _ib->Map();
	RAD_ASSERT(vb);
	U16 *indices = (U16*)vb->ptr.get();

	indices[0] = 0;
	indices[1] = 3;
	indices[2] = 1;
	indices[3] = 1;
	indices[4] = 3;
	indices[5] = 2;

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

	GLRenderTarget::Ref tempRT;
	m_activeRT.swap(tempRT);
	
	SetPerspectiveMatrix(*world->camera.get(), viewport);
	RotateForCamera(*world->camera.get());

	m_activeRT.swap(tempRT);
	
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

	GLRenderTarget::Ref tempRT;
	m_activeRT.swap(tempRT);
	
	SetPerspectiveMatrix(*world->camera.get(), viewport);
	RotateForCamera(*world->camera.get());
	
	m_activeRT.swap(tempRT);

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
