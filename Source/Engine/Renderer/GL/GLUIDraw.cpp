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
	kBaseRectSize = 64,
	kOverlayDiv = 8,
	kHalfOverlayDiv = kOverlayDiv / 2,
	kNumCircleSteps = 360,
	kNumCircleVerts = (kNumCircleSteps * (kHalfOverlayDiv - 1)) + 1,
	kNumCircleStepQuads = kHalfOverlayDiv - 2,
	kNumCircleStepTris = kNumCircleStepQuads * 2 + 1
};

}

RBDraw::Ref RBDraw::New() {
	return RBDraw::Ref(new (ZUI) GLDraw());
}

GLDraw::GLDraw() {
	InitRectVerts(kBaseRectSize, kBaseRectSize);
	InitCircleVerts(kBaseRectSize, kBaseRectSize);
}

void GLDraw::Begin() {
}

void GLDraw::End() {
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

	gl.Scalef(r.w / kBaseRectSize, r.h / kBaseRectSize, 1.f);

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
	gl.DrawElements(GL_TRIANGLES, (kOverlayDiv-1)*(kOverlayDiv-1)*6, GL_UNSIGNED_SHORT, 0);
	CHECK_GL_ERRORS();
	m.shader->End();

	gl.PopMatrix();
}

void GLDraw::DrawCircle(
	const Rect &r, 
	float percent,
	const Rect *clip,
	const Vec3 &zRot, // (X, Y) is rotation center, Z is rotation in degrees
	r::Material &m,
	asset::MaterialLoader *l,
	bool sampleMaterialColor,
	const Vec4 &rgba
) {
	percent = math::Clamp(percent, -1.f, 1.f);

	int base;
	int numElems;

	if (percent >= 0.f) {
		base = 0;
		numElems = FloatToInt(percent * kNumCircleSteps) * kNumCircleStepTris * 3;
	} else {
		base = FloatToInt((1 + percent) * kNumCircleSteps) * kNumCircleStepTris * 3;
		numElems = (kNumCircleSteps*kNumCircleStepTris*3) - base;
	}

	if (numElems < 1)
		return;

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

	gl.Scalef(r.w / (float)kBaseRectSize, r.h / (float)kBaseRectSize, 1.f);

	m.BindStates(flags);
	m.BindTextures(l);
	m.shader->Begin(r::Shader::kPass_Default, m);

	gls.DisableAllMGSources();
	gls.SetMGSource(
		r::kMaterialGeometrySource_Vertices,
		0,
		m_circleVB,
		2,
		GL_FLOAT,
		GL_FALSE,
		sizeof(OverlayVert),
		0
	);
	gls.SetMGSource(
		r::kMaterialGeometrySource_TexCoords,
		0,
		m_circleVB,
		2,
		GL_FLOAT,
		GL_FALSE,
		sizeof(OverlayVert),
		sizeof(float)*2
	);
	gls.BindBuffer(
		GL_ELEMENT_ARRAY_BUFFER_ARB,
		m_circleIB
	);

	Shader::Uniforms u(rgba);
	m.shader->BindStates(u, sampleMaterialColor);
	gls.Commit();

	gl.DrawElements(GL_TRIANGLES, numElems, GL_UNSIGNED_SHORT, (const void*)(base*sizeof(U16)));
	
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
			sizeof(OverlayVert)*kOverlayDiv*kOverlayDiv
		)
	);

	RAD_ASSERT(m_rectVB);

	GLVertexBuffer::Ptr::Ref vb = m_rectVB->Map();
	RAD_ASSERT(vb);
	OverlayVert *verts = (OverlayVert*)vb->ptr.get();

	float xInc = vpw / ((float)kOverlayDiv-1);
	float yInc = vph / ((float)kOverlayDiv-1);

	int x, y;
	float xf, yf;

	for (y = 0, yf = 0.f; y < kOverlayDiv; ++y, yf += yInc) {
		for (x = 0, xf = 0.f; x < kOverlayDiv; ++x, xf += xInc) {
			OverlayVert &v = verts[y*kOverlayDiv+x];
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
			sizeof(U16)*(kOverlayDiv-1)*(kOverlayDiv-1)*6
		)
	);

	vb = m_rectIB->Map();
	RAD_ASSERT(vb);
	U16 *indices = (U16*)vb->ptr.get();

	for (y = 0; y < kOverlayDiv-1; ++y) {
		for (x = 0; x < kOverlayDiv-1; ++x) {
			U16 *idx = &indices[y*(kOverlayDiv-1)*6+x*6];

			idx[0] = (U16)(y*kOverlayDiv+x);
			idx[1] = (U16)((y+1)*kOverlayDiv+x);
			idx[2] = (U16)((y+1)*kOverlayDiv+x+1);
			idx[3] = (U16)(y*kOverlayDiv+x);
			idx[4] = (U16)((y+1)*kOverlayDiv+x+1);
			idx[5] = (U16)(y*kOverlayDiv+x+1);
		}
	}

	vb.reset(); // unmap
}

void GLDraw::InitCircleVerts(int vpw, int vph) {
	m_circleVB.reset(
		new GLVertexBuffer(
			GL_ELEMENT_ARRAY_BUFFER_ARB,
			GL_STATIC_DRAW_ARB,
			sizeof(OverlayVert)*kNumCircleVerts
		)
	);

	RAD_ASSERT(m_circleVB);

	GLVertexBuffer::Ptr::Ref vb = m_circleVB->Map();
	RAD_ASSERT(vb);
	OverlayVert *verts = (OverlayVert*)vb->ptr.get();
	RAD_DEBUG_ONLY(OverlayVert *endVert = verts + kNumCircleVerts);

	float hw = vpw * 0.5f;
	float hh = vph * 0.5f;

	// center vertex
	verts->st[0] = 0.5;
	verts->st[1] = 0.5;
	verts->xy[0] = hw;
	verts->xy[1] = hh;

	++verts;

	float angleStep = math::Constants<float>::_2_PI() / (kNumCircleSteps - 1);
	float angle = -math::Constants<float>::PI_OVER_2();
	float lineStepX = hw / (kHalfOverlayDiv - 1);
	float lineStepY = hh / (kHalfOverlayDiv - 1);

	for (int i = 0; i < kNumCircleSteps; ++i, angle += angleStep) {
		float s, c;
		math::SinAndCos(&s, &c, angle);

		float xl = -1.f;
		float yl = -1.f;

		if (s != 0.f) {
			xl = hw / math::Abs(s);
		}

		if (c != 0.f) {
			yl = hh / math::Abs(c);
		}

		float w = hw;
		float h = hh;

		if ((xl > -1.f) && (xl <= yl)) {
			w = xl;
			h = hh * (xl/hw);
		} else if ((yl > -1.f) && (yl <= xl)) {
			h = yl;
			w = hw * (yl/hh);
		}

		float lineStepX = w / (kHalfOverlayDiv - 1);
		float lineStepY = h / (kHalfOverlayDiv - 1);
	
		float xd = lineStepX;
		float yd = lineStepY;

		for (int k = 1; k < kHalfOverlayDiv; ++k, xd += lineStepX, yd += lineStepY) {
			float x = hw + (c * xd);
			float y = hh + (s * yd);
			float s = x / vpw;
			float t = y / vph;

			RAD_ASSERT(verts < endVert); // sanity check

			verts->xy[0] = x;
			verts->xy[1] = y;
			verts->st[0] = s;
			verts->st[1] = t;
			++verts;
		}
	}

	vb.reset();

	// indices

	m_circleIB.reset(
		new GLVertexBuffer(
			GL_ELEMENT_ARRAY_BUFFER_ARB,
			GL_STATIC_DRAW_ARB,
			sizeof(U16)*kNumCircleStepTris*kNumCircleSteps*3
		)
	);

	vb = m_circleIB->Map();
	RAD_ASSERT(vb);
	U16 *indices = (U16*)vb->ptr.get();
	RAD_DEBUG_ONLY(U16 *endIndices = indices+(kNumCircleStepTris*kNumCircleSteps*3));

	for (int i = 0; i < kNumCircleSteps; ++i) {
		int k = i+1;
		if (k >= kNumCircleSteps)
			k = 0;

		int base0 = i*(kHalfOverlayDiv-1)+1;
		int base1 = k*(kHalfOverlayDiv-1)+1;

		// triangle
		RAD_ASSERT(indices+3 <= endIndices);
		indices[0] = 0; // center
		indices[1] = base1;
		indices[2] = base0;
		indices += 3;

		// quads.
		for (int k = 0; k < kNumCircleStepQuads; ++k) {
			RAD_ASSERT(indices+6 <= endIndices);
			indices[0] = base1+k;
			indices[1] = base1+k+1;
			indices[2] = base0+k;
			indices[3] = base0+k;
			indices[4] = base1+k+1;
			indices[5] = base0+k+1;
			indices += 6;
		}
	}

	vb.reset();
}

} // ui
