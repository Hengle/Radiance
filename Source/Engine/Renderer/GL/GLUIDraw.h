// GLUIDraw.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../../UI/UIWidget.h"
#include "GLVertexBuffer.h"

namespace ui {

class RADENG_CLASS GLDraw : public RBDraw
{
public:

	GLDraw();

	virtual void SetViewport(
		int src[4],
		int dst[4]
	);

	virtual void DrawRect(
		const Rect &r, 
		r::Material &m,
		const asset::MaterialLoader::Ref &l,
		bool sampleMaterialColor,
		const Vec4 &rgba
	);

	virtual void DrawTextModel(
		const Rect &r,
		const Rect *clip,
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

	r::GLVertexBuffer::Ref m_rectVB;
	r::GLVertexBuffer::Ref m_rectIB;
	float m_vp[4];
};

} // ui
