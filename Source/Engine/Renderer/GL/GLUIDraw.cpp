// GLUIDraw.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "GLUIDraw.h"
#include "GLTable.h"
#include "GLState.h"
#include "../TextModel.h"

using namespace r;

namespace ui {

namespace {

struct OverlayVert {
	float xy[2];
	float st[2];
};

enum {
	BaseRectSize = 64,
	OverlayDiv = 8
};

}

RBDraw::Ref RBDraw::New() {
	return RBDraw::Ref(new (ZUI) GLDraw());
}

GLDraw::GLDraw() {
	InitRectVerts(BaseRectSize, BaseRectSize);
}

void GLDraw::SetViewport(
	int src[4],
	int dst[4]
) {
	gl.MatrixMode(GL_PROJECTION);
	gl.LoadIdentity();
	gl.Ortho((double)src[0], (double)(src[0]+src[2]), (double)(src[1]+src[3]), (double)src[1], -1.0, 1.0);
	gl.MatrixMode(GL_MODELVIEW);
	gl.LoadIdentity();

	m_srcvp[0] = (float)src[0];
	m_srcvp[1] = (float)src[1];
	m_srcvp[2] = (float)src[2];
	m_srcvp[3] = (float)src[3];
	
	m_dstvp[0] = (float)dst[0];
	m_dstvp[1] = (float)dst[1];
	m_dstvp[2] = (float)dst[2];
	m_dstvp[3] = (float)dst[3];
	
	m_todst[0] = m_dstvp[2] / m_srcvp[2];
	m_todst[1] = m_dstvp[3] / m_srcvp[3];
}

void GLDraw::DrawRect(
	const Rect &r, 
	const Rect *clip,
	const Vec3 &zRot, // (X, Y) is rotation center, Z is rotation in degrees
	r::Material &m,
	asset::MaterialLoader *l,
	bool sampleMaterialColor,
	const Vec4 &rgba
) {
	int flags = 0;

	if (clip) {
		flags |= kScissorTest_Enable;
		float x = ((clip->x - m_srcvp[0]) * m_todst[0]) + m_dstvp[0];
		float y = ((clip->y - m_srcvp[1]) * m_todst[1]) + m_dstvp[1];
		float w = clip->w * m_todst[0];
		float h = clip->h * m_todst[1];
		
		gls.Scissor(
			FloorFastInt(x),
			FloorFastInt(m_dstvp[3]-(y+h)),
			FloorFastInt(w),
			FloorFastInt(h)
		);
	} else {
		flags |= kScissorTest_Disable;
	}

	gl.MatrixMode(GL_MODELVIEW);
	gl.PushMatrix();
	gl.Translatef((float)r.x, (float)r.y, 0.f);

	if (zRot[2] != 0.f) {
		float cx = zRot[0]-r.x;
		float cy = zRot[1]-r.y;
		gl.Translatef(cx, cx, 0.f);
		gl.Rotatef(zRot[2], 0.f, 0.f, 1.f);
		gl.Translatef(-cx, -cy, 0.f);
	}

	gl.Scalef(r.w / (float)BaseRectSize, r.h / (float)BaseRectSize, 1.f);

	m.BindStates(flags);
	m.BindTextures(l);
	m.shader->Begin(r::Shader::kPass_Default, m);

	gls.DisableAllMGSources();
	gls.SetMGSource(
		r::kMaterialGeometrySource_Vertices,
		0,
		m_rectVB,
		2,
		GL_FLOAT,
		GL_FALSE,
		sizeof(OverlayVert),
		0
	);
	gls.SetMGSource(
		r::kMaterialGeometrySource_TexCoords,
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

	Shader::Uniforms u(rgba);
	m.shader->BindStates(u, sampleMaterialColor);
	gls.Commit();
	gl.DrawElements(GL_TRIANGLES, (OverlayDiv-1)*(OverlayDiv-1)*6, GL_UNSIGNED_SHORT, 0);
	CHECK_GL_ERRORS();
	m.shader->End();

	gl.PopMatrix();
}

void GLDraw::DrawTextModel(
	const Rect &r,
	const Rect *clip,
	const Vec3 &zRot, // (X, Y) is rotation center, Z is rotation in degrees
	r::Material &material,
	r::TextModel &model,
	bool sampleMaterialColor,
	const Vec4 &rgba
) {
	int flags = 0;

	if (clip) {
		flags |= kScissorTest_Enable;
		float x = ((clip->x - m_srcvp[0]) * m_todst[0]) + m_dstvp[0];
		float y = ((clip->y - m_srcvp[1]) * m_todst[1]) + m_dstvp[1];
		float w = clip->w * m_todst[0];
		float h = clip->h * m_todst[1];
		
		gls.Scissor(
			FloorFastInt(x),
			FloorFastInt(m_dstvp[3]-(y+h)),
			FloorFastInt(w),
			FloorFastInt(h)
		);
	} else {
		flags |= kScissorTest_Disable;
	}

	gl.MatrixMode(GL_MODELVIEW);
	gl.PushMatrix();
	gl.Translatef((float)r.x, (float)r.y, 0.f);
	model.Draw(material, sampleMaterialColor, rgba, flags, 0);
	gl.PopMatrix();
}

void GLDraw::BeginBatchText(
	const Rect &r,
	const Rect *clip,
	r::Material &material
) {
	int flags = 0;

	if (clip) {
		flags |= kScissorTest_Enable;
		// Scissor is in window coordinates (viewport)
		float x = ((clip->x - m_srcvp[0]) * m_todst[0]) + m_dstvp[0];
		float y = ((clip->y - m_srcvp[1]) * m_todst[1]) + m_dstvp[1];
		float w = clip->w * m_todst[0];
		float h = clip->h * m_todst[1];
		
		gls.Scissor(
			FloorFastInt(x),
			FloorFastInt(m_dstvp[3]-(y+h)),
			FloorFastInt(w),
			FloorFastInt(h)
		);
	}

	gl.MatrixMode(GL_MODELVIEW);
	gl.PushMatrix();
	gl.Translatef((float)r.x, (float)r.y, 0.f);
	
	material.BindStates(flags, 0);
}

void GLDraw::BatchDrawTextModel(
	r::Material &material,
	r::TextModel &model
) {
	model.BatchDraw(material);
}

void GLDraw::EndBatchText() {
	gl.MatrixMode(GL_MODELVIEW);
	gl.PopMatrix();
}

void GLDraw::InitRectVerts(int vpw, int vph) {
	m_rectVB.reset(
		new GLVertexBuffer(
			GL_ARRAY_BUFFER_ARB, 
			GL_STATIC_DRAW_ARB, 
			sizeof(OverlayVert)*OverlayDiv*OverlayDiv
		)
	);

	RAD_ASSERT(m_rectVB);

	GLVertexBuffer::Ptr::Ref vb = m_rectVB->Map();
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
			v.st[1] = yf / vph;
		}
	}

	vb.reset(); // unmap

	// setup triangle indices

	m_rectIB.reset(
		new GLVertexBuffer(
			GL_ELEMENT_ARRAY_BUFFER_ARB,
			GL_STATIC_DRAW_ARB,
			sizeof(U16)*(OverlayDiv-1)*(OverlayDiv-1)*6
		)
	);

	vb = m_rectIB->Map();
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

} // ui
