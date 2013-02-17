// GLUIDraw.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../../UI/UIWidget.h"
#include "GLVertexBuffer.h"

namespace ui {

class RADENG_CLASS GLDraw : public RBDraw {
public:

	GLDraw();

	virtual void SetViewport(
		int src[4],
		int dst[4]
	);

	virtual void DrawRect(
		const Rect &r, 
		const Rect *clip,
		const Vec3 &zRot, // (X, Y) is rotation center, Z is rotation in degrees
		r::Material &m,
		asset::MaterialLoader *l,
		bool sampleMaterialColor,
		const Vec4 &rgba
	);

	virtual void DrawCircle(
		const Rect &r, 
		float percent, // 0 = no wedge, 1 = full circle (winds clockwise), 0 -> -1 winds counter-clockwise
		const Rect *clip,
		const Vec3 &zRot, // (X, Y) is rotation center, Z is rotation in degrees
		r::Material &m,
		asset::MaterialLoader *l,
		bool sampleMaterialColor = true,
		const Vec4 &rgba = Vec4(1, 1, 1, 1)
	);

	virtual void DrawTextModel(
		const Rect &r,
		const Rect *clip,
		const Vec3 &zRot, // (X, Y) is rotation center, Z is rotation in degrees
		r::Material &material,
		r::TextModel &model,
		bool sampleMaterialColor,
		const Vec4 &rgba
	);

	virtual void BeginBatchText(
		const Rect &r,
		const Rect *clip,
		r::Material &material
	);

	virtual void BatchDrawTextModel(
		r::Material &material,
		r::TextModel &model
	);

	virtual void EndBatchText();

private:

	void InitRectVerts(int vpw, int vph);
	void InitCircleVerts(int vpw, int vph);
	
	r::GLVertexBuffer::Ref m_rectVB;
	r::GLVertexBuffer::Ref m_rectIB;
	r::GLVertexBuffer::Ref m_circleVB;
	r::GLVertexBuffer::Ref m_circleIB;
	float m_srcvp[4];
	float m_dstvp[4];
	float m_todst[2];
};

} // ui
