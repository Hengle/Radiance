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

GLWorldDraw::GLWorldDraw(World *world) : RB_WorldDraw(world), m_stencilRef(0), m_flipMatrix(false) {
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
	gls.StencilMask(0xff); // let us clear stencil
	gls.Set(kColorWriteMask_RGBA|kDepthWriteMask_Enable|kScissorTest_Disable, -1, true); // for glClear()
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
	gls.StencilMask(0);
}

int GLWorldDraw::LoadMaterials() {
	int r = LoadMaterial("Sys/Copy_M", m_copy_M);
	if (r != pkg::SR_Success)
		return r;
	r = LoadMaterial("Sys/ClearStencil_M", m_clearStencil_M);
	return r;
}

int GLWorldDraw::Precache() {
	CreateScreenOverlay();
	CreateRect();
	return pkg::SR_Success;
}

void GLWorldDraw::BeginPrecacheMaterials() {
	struct PrecacheVert {
		float xyz[3];
		float st[2];
		float normal[4];
		float tangent[4];
	};

	m_precacheMesh.reset(new (ZRender) Mesh());
	int stream = m_precacheMesh->AllocateStream(kStreamUsage_Static, sizeof(PrecacheVert), 4);

	m_precacheMesh->MapSource(
		stream,
		kMaterialGeometrySource_Vertices,
		0,
		sizeof(PrecacheVert),
		0,
		3
	);

	m_precacheMesh->MapSource(
		stream,
		kMaterialGeometrySource_TexCoords,
		0,
		sizeof(PrecacheVert),
		sizeof(float)*3,
		2
	);

	m_precacheMesh->MapSource(
		stream,
		kMaterialGeometrySource_Normals,
		0,
		sizeof(PrecacheVert),
		sizeof(float)*5,
		4
	);

	m_precacheMesh->MapSource(
		stream,
		kMaterialGeometrySource_Tangents,
		0,
		sizeof(PrecacheVert),
		sizeof(float)*9,
		4
	);

	GLVertexBuffer::Ptr::Ref vb = m_precacheMesh->Map(stream);
	RAD_ASSERT(vb);
	PrecacheVert *verts = (PrecacheVert*)vb->ptr.get();

	verts[0].xyz[0] = -1.0f;
	verts[0].xyz[1] = -1.0f;
	verts[0].xyz[2] = 0.0f;
	verts[0].normal[0] = 1.0f;
	verts[0].normal[1] = 0.0f;
	verts[0].normal[2] = 0.0f;
	verts[0].normal[3] = 1.0f;
	verts[0].st[0] = 0.0f;
	verts[0].st[1] = 0.0f;
	verts[0].tangent[0] = 1.0f;
	verts[0].tangent[1] = 0.0f;
	verts[0].tangent[2] = 0.0f;
	verts[0].tangent[3] = 1.0f;

	verts[1].xyz[0] = -1.0f;
	verts[1].xyz[1] = 1.0f;
	verts[1].xyz[2] = 0.0f;
	verts[1].normal[0] = 1.0f;
	verts[1].normal[1] = 0.0f;
	verts[1].normal[2] = 0.0f;
	verts[1].normal[3] = 1.0f;
	verts[1].st[0] = 0.0f;
	verts[1].st[1] = 0.0f;
	verts[1].tangent[0] = 1.0f;
	verts[1].tangent[1] = 0.0f;
	verts[1].tangent[2] = 0.0f;
	verts[1].tangent[3] = 1.0f;

	verts[2].xyz[0] = 1.0f;
	verts[2].xyz[1] = 1.0f;
	verts[2].xyz[2] = 0.0f;
	verts[2].normal[0] = 1.0f;
	verts[2].normal[1] = 0.0f;
	verts[2].normal[2] = 0.0f;
	verts[2].normal[3] = 1.0f;
	verts[2].st[0] = 0.0f;
	verts[2].st[1] = 0.0f;
	verts[2].tangent[0] = 1.0f;
	verts[2].tangent[1] = 0.0f;
	verts[2].tangent[2] = 0.0f;
	verts[2].tangent[3] = 1.0f;
	
	verts[3].xyz[0] = 1.0f;
	verts[3].xyz[1] = -1.0f;
	verts[3].xyz[2] = 0.0f;
	verts[3].normal[0] = 1.0f;
	verts[3].normal[1] = 0.0f;
	verts[3].normal[2] = 0.0f;
	verts[3].normal[3] = 1.0f;
	verts[3].st[0] = 0.0f;
	verts[3].st[1] = 0.0f;
	verts[3].tangent[0] = 1.0f;
	verts[3].tangent[1] = 0.0f;
	verts[3].tangent[2] = 0.0f;
	verts[3].tangent[3] = 1.0f;

	vb.reset(); // unmap

	// setup triangle indices

	vb = m_precacheMesh->MapIndices(kStreamUsage_Static, sizeof(U16), 6);
	RAD_ASSERT(vb);
	U16 *indices = (U16*)vb->ptr.get();

	indices[0] = 0;
	indices[1] = 1;
	indices[2] = 2;
	indices[3] = 0;
	indices[4] = 2;
	indices[5] = 3;

	vb.reset(); // unmap

	gl.MatrixMode(GL_PROJECTION);
	gl.LoadIdentity();
	gls.invertCullFace = false;
	
	gl.Ortho(-1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f);
	gl.MatrixMode(GL_MODELVIEW);
}

void GLWorldDraw::PrecacheMaterial(const details::MatRef &mat) {

	Shader::Uniforms u(Shader::Uniforms::kDefault);
	u.eyePos = Vec3::Zero;
	u.pfxVars = Vec2::Zero;
	u.tcGen = Mat4::Identity;
	
	u.lights.numLights = kMaxLights;

	for (int i = 0; i < kMaxLights; ++i) {
		u.lights.lights[i].diffuse = Vec3(1.0f, 1.0f, 1.0f);
		u.lights.lights[i].specular = Vec3(1.0f, 1.0f, 1.0f);
		u.lights.lights[i].intensity = 1.0f;
		u.lights.lights[i].pos = Vec3::Zero;
		u.lights.lights[i].radius = 1.0f;
		u.lights.lights[i].flags = r::LightDef::kFlag_Diffuse|r::LightDef::kFlag_Specular;
	}

	mat.mat->BindStates(
		kDepthTest_Disable|kDepthWriteMask_Disable|kCullFaceMode_None|kColorWriteMask_RGBA|kScissorTest_Disable|kStencilTest_Disable,
		kBlendMode_Off
	);
	
	mat.mat->BindTextures(mat.loader);

	for (int i = r::Shader::kPass_Default; i <= r::Shader::kPass_Fullbright; ++i) {

		if (!mat.mat->shader->HasPass((Shader::Pass)i))
			continue;

		mat.mat->shader->Begin((Shader::Pass)i, *mat.mat);
		m_precacheMesh->BindAll(mat.mat->shader.get().get());
		mat.mat->shader->BindStates(u, false);
		CommitStates();
		m_precacheMesh->Draw();
		mat.mat->shader->End();
	}
}

void GLWorldDraw::EndPrecacheMaterials() {
	m_precacheMesh.reset();
}

void GLWorldDraw::FlipMatrixHack(bool enable) {
	m_flipMatrix = enable;
}

void GLWorldDraw::BindRenderTarget() {
	int vpx, vpy, vpw, vph;
	world->game->Viewport(vpx, vpy, vpw, vph);

	if (!m_framebufferRT) {
		m_framebufferRT.reset(new (ZRender) GLRenderTarget(
			GL_TEXTURE_2D,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			vpw,
			vph,
			0/*GL_DEPTH_COMPONENT24_ARB*/,
			4*vpw*vph,
			TX_FilterBilinear,
			false
		));

		m_framebufferRT->CreateDepthBufferTexture();

		m_fogRT.reset(new (ZRender) GLRenderTarget(
			GL_TEXTURE_2D,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			vpw,
			vph,
			0,
			4*vpw*vph,
			TX_FilterBilinear,
			false
		));
	}
	
	GLRenderTarget::DiscardFramebuffer(GLRenderTarget::kDiscard_All);

	m_activeRT = m_framebufferRT;
	m_activeRT->BindFramebuffer(GLRenderTarget::kDiscard_All);
	m_stencilRef = 0;
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

	int srcW = (int)(vpw * srcScale[0]);
	int srcH = (int)(vph * srcScale[1]);
	
	GLRenderTarget::Ref curRT(m_activeRT);
				
	if ((curRT->tex->width != srcW) || (curRT->tex->height != srcH)) {
		m_activeRT = m_rtCache->NextRenderTarget(srcW, srcH, true);
		Copy(curRT, m_activeRT, GLRenderTarget::kDiscard_All);
		curRT = m_activeRT;
	}
		
	curRT->BindTexture(0);
	m_framebufferRT->BindTexture(1);

	if (chain) {
		// select output size
		int dstW = (int)(vpw * dstScale[0]);
		int dstH = (int)(vph * dstScale[1]);

		m_activeRT = m_rtCache->NextRenderTarget(dstW, dstH, true);
		m_activeRT->BindFramebuffer(GLRenderTarget::kDiscard_All);
	} else {
		GLRenderTarget::DiscardFramebuffer(GLRenderTarget::kDiscard_Depth); // don't need depth anymore.
		BindFramebuffer(true, false);
		m_activeRT.reset();
	}

	return Vec2(1.f/srcW, 1.f/srcH);
}

void GLWorldDraw::Copy(
	const r::GLRenderTarget::Ref &src,
	const r::GLRenderTarget::Ref &dst,
	r::GLRenderTarget::DiscardFlags discard
) {
	m_copy_M.material->BindTextures(m_copy_M.loader);
	src->BindTexture(0);
	if (dst)
		dst->BindFramebuffer(discard);
	m_copy_M.material->BindStates();

	m_copy_M.material->shader->Begin(r::Shader::kPass_Default, *m_copy_M.material);

	gl.MatrixMode(GL_PROJECTION);
	gl.PushMatrix();
	gl.LoadIdentity();

	bool oldCullFace = gls.invertCullFace;

	if (m_flipMatrix) { // HACK blit to frame buffer
		// render target tc's are flipped about Y, so correct for this in the transform.
		gl.Scalef(1.f, -1.f, 1.f);
		gls.invertCullFace = true;
	} else {
		gls.invertCullFace = false;
	}

	gl.Ortho(0.0, 1.0, 1.0, 0.0, -1.0, 1.0);

	//gls.invertCullFace = false;

	gl.MatrixMode(GL_MODELVIEW);
	gl.PushMatrix();
	gl.LoadIdentity();

	m_rectMesh->BindAll(m_copy_M.material->shader.get().get());

	m_copy_M.material->shader->BindStates();
	gls.Commit();
	
	m_rectMesh->CompileArrayStates(*m_copy_M.material->shader.get());
	m_rectMesh->Draw();

	m_copy_M.material->shader->End();

	gl.MatrixMode(GL_PROJECTION);
	gl.PopMatrix();

	gl.MatrixMode(GL_MODELVIEW);
	gl.PopMatrix();

	gls.invertCullFace = oldCullFace;
}

void GLWorldDraw::BeginFogDepthWrite(r::Material &fog, bool front) {
	if (m_activeRT.get() != m_framebufferRT.get()) {
		Copy(m_fogRT, m_framebufferRT, GLRenderTarget::kDiscard_None);
		m_activeRT = m_framebufferRT;
	}

	fog.BindStates(
		r::kColorWriteMask_Off|
		r::kDepthWriteMask_Enable|
		r::kDepthTest_Less|
		(front ? (r::kCullFaceMode_Back|r::kCullFaceMode_CCW) : (r::kCullFaceMode_Front|r::kCullFaceMode_CCW)),
		r::kBlendMode_Off
	);
}

void GLWorldDraw::BeginFogDraw(r::Material &fog) {

	if (m_activeRT.get() != m_fogRT.get()) {
		Copy(m_framebufferRT, m_fogRT, GLRenderTarget::kDiscard_All);
		m_activeRT = m_fogRT;
	}
	
	gls.SetMTSource(kMaterialTextureSource_Texture, 0, m_framebufferRT->depthTex);
	
	fog.BindStates(
		r::kColorWriteMask_RGBA|
		r::kDepthWriteMask_Disable|
		r::kDepthTest_Disable|
		r::kCullFaceMode_Back|r::kCullFaceMode_CCW,
		0
	);
}

void GLWorldDraw::BeginFog() {
	
}

void GLWorldDraw::EndFog() {
	if (m_activeRT.get() != m_framebufferRT.get()) {
	// last fog volume doesn't need to keep this intact.
		Copy(m_fogRT, m_framebufferRT, GLRenderTarget::kDiscard_All);
		m_activeRT = m_framebufferRT;
	}
}

void GLWorldDraw::RenderLightStencil(
	const ViewDef &view,
	const Vec4 **rects,
	int numRects
) {
	m_clearStencil_M.material->BindTextures(m_clearStencil_M.loader);
	m_clearStencil_M.material->BindStates(
		kStencilTest_Enable|
		kDepthWriteMask_Disable|
		kDepthTest_Disable|
		kColorWriteMask_Off
	);

	m_clearStencil_M.material->shader->Begin(r::Shader::kPass_Default, *m_clearStencil_M.material);

	gl.MatrixMode(GL_PROJECTION);
	gl.PushMatrix();
	gl.LoadIdentity();

	bool oldCullFace = gls.invertCullFace;
	gls.invertCullFace = false;

	gl.Ortho(0.0, 1.0, 1.0, 0.0, -1.0, 1.0);

	gl.MatrixMode(GL_MODELVIEW);
	gl.PushMatrix();
	gl.LoadIdentity();
	
	m_rectMesh->BindAll(m_clearStencil_M.material->shader.get().get());

	gls.StencilMask(0xff);

	if (++m_stencilRef == 256) {
		// stencil has wrapped, clear it
		m_stencilRef = 1;
		gls.StencilFunc(GL_ALWAYS, 0, 0xff);
		gls.StencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
		m_clearStencil_M.material->shader->BindStates();
		gls.Commit();
		m_rectMesh->CompileArrayStates(*m_clearStencil_M.material->shader.get());
		m_rectMesh->Draw();
	}

	// write stencilRef into buffer over pixels
	// that lights are touching.
	gls.StencilFunc(GL_ALWAYS, m_stencilRef, 0xff);
	gls.StencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	const float kVw = (float)view.viewport[2];
	const float kVh = (float)view.viewport[3];

	for (int i = 0; i < numRects; ++i) {
		// scale and translate into position
		gl.PushMatrix();

		const Vec4 &r = *rects[i];

		gl.Translatef(
			r[0] / kVw,
			r[1] / kVh,
			0.f
		);

		gl.Scalef(
			r[2] / kVw,
			r[3] / kVh,
			1.f
		);

		m_clearStencil_M.material->shader->BindStates();
		gls.Commit();
		m_rectMesh->CompileArrayStates(*m_clearStencil_M.material->shader.get());
		m_rectMesh->Draw();

		gl.PopMatrix();
	}
	
	m_clearStencil_M.material->shader->End();

	gl.MatrixMode(GL_PROJECTION);
	gl.PopMatrix();

	gl.MatrixMode(GL_MODELVIEW);
	gl.PopMatrix();

	gls.invertCullFace = oldCullFace;

	gls.StencilFunc(GL_EQUAL, m_stencilRef, 0xff);
	gls.StencilMask(0);
}

void GLWorldDraw::BeginUnifiedShadows() {
#if defined(PRERENDER_SHADOWS)
	if (m_unifiedShadowRTCache)
		m_unifiedShadowRTCache->Reset();
	m_shadowRT.reset();
#endif
}

void GLWorldDraw::EndUnifiedShadows() {
}

bool GLWorldDraw::BindUnifiedShadowRenderTarget(r::Material &shadowMaterial) {

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

		vpw = math::Min(PowerOf2(vpw), 1024);
		vph = math::Min(PowerOf2(vph), 1024);

		m_unifiedShadowRTCache.reset(new (ZRender) GLRenderTargetCache(
			vpw,
			vph,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			depth,
			GLRenderTargetCache::kDepthInstanceMode_Shared,
#if defined(RAD_OPT_OGLES)
			TX_FilterBilinear,
#else
			TX_Mipmap|TX_FilterTrilinear,
#endif
			4,
			depthBytesPP
		));

		m_unifiedShadowRTCache->CreateRenderTargets(kNumShadowTextures);
	}

#if defined(PRERENDER_SHADOWS)
	const bool kWrap = false;
#else
	const bool kWrap = true;
#endif

	if (m_shadowRT)
		GLRenderTarget::DiscardFramebuffer(GLRenderTarget::kDiscard_All);

	m_shadowRT = m_unifiedShadowRTCache->NextRenderTarget(kWrap);
	if (!m_shadowRT)
		return false;

	m_shadowRT->BindFramebuffer(GLRenderTarget::kDiscard_All);

	gls.Scissor(
		8,
		8,
		m_shadowRT->tex->size-16,
		m_shadowRT->tex->size-16
	);
	CHECK_GL_ERRORS();

	shadowMaterial.BindStates(kScissorTest_Enable);

	return true;
}

bool GLWorldDraw::BindUnifiedShadowTexture(
	r::Material &projectedTexture
) {
#if defined(PRERENDER_SHADOWS)
	m_shadowRT = m_unifiedShadowRTCache->NextRenderTarget(false);
	if (!m_shadowRT)
		return false;
#endif
	RAD_ASSERT(m_shadowRT);
	m_shadowRT->BindTexture(0);
#if !defined(RAD_OPT_OGLES)
	GLTexture::GenerateMipmaps(m_shadowRT->tex); // <-- causes logical buffer load (slooow)
#endif
	m_shadowRT.reset();

#if !defined(PRERENDER_SHADOWS)
	if (m_activeRT) {
		m_activeRT->BindFramebuffer(GLRenderTarget::kDiscard_None);
	}
	else {
		BindFramebuffer(false, false);
	}
#endif

	projectedTexture.BindStates(0, kBlendModeSource_SrcAlpha|kBlendModeDest_InvSrcAlpha);
	return true;
}

void GLWorldDraw::BindFramebuffer(bool discardHint, bool copy) {
	int vpx, vpy, vpw, vph;
	world->game->Viewport(vpx, vpy, vpw, vph);
	App::Get()->engine->sys->r->BindFramebuffer();
	gls.Viewport(0, 0, vpw, vph);
	
	if (discardHint)
		ClearBackBuffer(); // discard hint

	if (m_activeRT && copy) {
		Copy(m_activeRT, GLRenderTarget::Ref(), GLRenderTarget::kDiscard_All);
		m_activeRT.reset();
	}
}

Mat4 GLWorldDraw::MakePerspectiveMatrix(
	float left, 
	float right, 
	float top, 
	float bottom, 
	float near, 
	float far,
	bool txAddressBias
) {
	Mat4 m = Mat4::PerspectiveOffCenterRH(left, right, bottom, top, near, far);
	if (txAddressBias) {
		Mat4 bias = Mat4(
			Vec4(0.5f, 0.0f, 0.0f, 0.0f),
			Vec4(0.0f, 0.5f, 0.0f, 0.0f),
			Vec4(0.0f, 0.0f, 0.5f, 0.0f),
			Vec4(0.5f, 0.5f, 0.5f, 1.0f)
		);
		m = m * (bias);
	}
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

	if (m_flipMatrix) { 
		// render target tc's are flipped about Y, so correct for this in the perspective transform.
		gl.Scalef(1.f, -1.f, 1.f);
		gls.invertCullFace = true;
	} else {
		gls.invertCullFace = false;
	}

	gl.Perspective(yfov, xaspect, 4.0, camera.farClip.get());

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

	if (m_flipMatrix) { 
		// render target tc's are flipped about Y, so correct for this in the transform.
		gl.Scalef(1.f, -1.f, 1.f);
		gls.invertCullFace = true;
	} else {
		gls.invertCullFace = false;
	}

	gl.Ortho((double)vpx, (double)(vpx+vpw), (double)(vpy+vph), (double)vpy, -1.0, 1.0);

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
	
	if (m_flipMatrix) { 
		// render target tc's are flipped about Y, so correct for this in the transform.
		gl.Scalef(1.f, -1.f, 1.f);
		gls.invertCullFace = true;
	} else {
		gls.invertCullFace = false;
	}

	gl.Ortho(
		(double)left, 
		(double)right,
		(double)bottom,
		(double)top,
		(double)near,
		(double)far
	);

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

void GLWorldDraw::BindLitMaterialStates(r::Material &mat, bool lightStencil) {
	int flags = 0;
	
	if (mat.depthWrite)
		flags |= kDepthTest_Equal|kDepthWriteMask_Disable;

	if (lightStencil) {
		flags |= kStencilTest_Enable;
	} else {
		flags |= kStencilTest_Disable;
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
		CreateOverlay(vpw, vph, m_overlay, false);
		m_overlaySize[0] = vpw;
		m_overlaySize[1] = vph;
	}
}

void GLWorldDraw::BindPostFXQuad(const r::Material &mat) {
	BindOverlay(mat);
}

void GLWorldDraw::DrawPostFXQuad() {
	DrawOverlay();
}

void GLWorldDraw::BindOverlay(const r::Material &mat) {
	BOOST_STATIC_ASSERT(sizeof(GLWorldDraw::OverlayVert)==16);
	
	CreateScreenOverlay();

	const r::Mesh::Ref &mesh = m_overlay;
	mesh->BindAll(mat.shader.get().get());
}

void GLWorldDraw::DrawOverlay() {
	const r::Mesh::Ref &mesh = m_overlay;
	mesh->Draw();
	CHECK_GL_ERRORS();
}

void GLWorldDraw::CreateOverlay(
	int vpw, 
	int vph,
	r::Mesh::Ref &mesh,
	bool invY
) {
	mesh.reset(new (ZRender) Mesh());
	int stream = mesh->AllocateStream(kStreamUsage_Static, sizeof(OverlayVert), OverlayDiv*OverlayDiv);

	mesh->MapSource(
		stream,
		kMaterialGeometrySource_Vertices,
		0,
		sizeof(OverlayVert),
		0,
		2
	);

	mesh->MapSource(
		stream,
		kMaterialGeometrySource_TexCoords,
		0,
		sizeof(OverlayVert),
		sizeof(float)*2,
		2
	);

	GLVertexBuffer::Ptr::Ref vb = mesh->Map(stream);
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

	vb = mesh->MapIndices(kStreamUsage_Static, sizeof(U16), (OverlayDiv-1)*(OverlayDiv-1)*6);
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

void GLWorldDraw::CreateRect() {

	m_rectMesh.reset(new (ZRender) Mesh());
	int stream = m_rectMesh->AllocateStream(r::kStreamUsage_Static, sizeof(OverlayVert), 4, false);

	m_rectMesh->MapSource(
		stream,
		kMaterialGeometrySource_Vertices, 
		0, 
		sizeof(OverlayVert), 
		0,
		2
	);

	m_rectMesh->MapSource(
		stream,
		kMaterialGeometrySource_TexCoords,
		0,
		sizeof(OverlayVert),
		sizeof(float)*2,
		2
	);

	GLVertexBuffer::Ptr::Ref vb = m_rectMesh->Map(stream);
	RAD_ASSERT(vb);
	OverlayVert *verts = (OverlayVert*)vb->ptr.get();

	verts[0].st[0] = 0.f;
	verts[0].st[1] = 0.f;
	verts[0].xy[0] = 0.f;
	verts[0].xy[1] = 0.f;
	
	verts[1].st[0] = 1.f;
	verts[1].st[1] = 0.f;
	verts[1].xy[0] = 1.f;
	verts[1].xy[1] = 0.f;

	verts[2].st[0] = 1.f;
	verts[2].st[1] = 1.f;
	verts[2].xy[0] = 1.f;
	verts[2].xy[1] = 1.f;

	verts[3].st[0] = 0.f;
	verts[3].st[1] = 1.f;
	verts[3].xy[0] = 0.f;
	verts[3].xy[1] = 1.f;

	vb.reset(); // unmap

	// setup triangle indices

	vb = m_rectMesh->MapIndices(kStreamUsage_Static, sizeof(U16), 6);
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

Mat4 GLWorldDraw::GetModelViewProjectionMatrix(bool txAddressBias) {
	if (txAddressBias) {
		Mat4 bias = Mat4(
			Vec4(0.5f, 0.0f, 0.0f, 0.0f),
			Vec4(0.0f, 0.5f, 0.0f, 0.0f),
			Vec4(0.0f, 0.0f, 0.5f, 0.0f),
			Vec4(0.5f, 0.5f, 0.5f, 1.0f)
		);
		return gl.GetModelViewMatrix() * gl.GetProjectionMatrix() * bias;
	}

	return gl.GetModelViewProjectionMatrix();
}

} // world
