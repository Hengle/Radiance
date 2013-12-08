// GLWorldDebugDraw.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "GLWorldDraw.h"
#if defined(WORLD_DEBUG_DRAW)
#include "GLState.h"
#undef min
#undef max

using namespace r;

namespace world {

void GLWorldDraw::BeginDebugDraw() {
}

void GLWorldDraw::EndDebugDraw() {
	gls.DisableAllMGSources();
	gls.BindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, GLVertexBuffer::Ref(), false);
}

void GLWorldDraw::AllocateDebugVerts(int num) {
	if (num <= m_numDebugVerts)
		return;

	m_numDebugVerts = num;

	m_debugVerts.reset(
		new r::GLVertexBuffer(
			GL_ARRAY_BUFFER_ARB, 
			GL_DYNAMIC_DRAW_ARB, 
			num * kDebugVertSize
		)
	);

}

void GLWorldDraw::AllocateDebugIndices(int num) {
	if (num <= m_numDebugIndices)
		return;

	m_numDebugIndices = num;

	m_debugIndices.reset(
		new r::GLVertexBuffer(
			GL_ELEMENT_ARRAY_BUFFER_ARB,
			GL_DYNAMIC_DRAW_ARB,
			num * sizeof(U16)
		)
	);
}

void GLWorldDraw::DebugUploadVerts(
	const Vec3 *verts, 
	int numVerts
) {
	gls.DisableAllMGSources();
	AllocateDebugVerts(numVerts);

	r::GLVertexBuffer::Ptr::Ref vb = m_debugVerts->Map();
	memcpy(vb->ptr, verts, kDebugVertSize*numVerts);
	vb.reset();

	gls.SetMGSource(
		kMaterialGeometrySource_Vertices, 
		0, 
		m_debugVerts,
		3,
		GL_FLOAT,
		GL_FALSE,
		sizeof(Vec3),
		0
	);
}

void GLWorldDraw::DebugUploadIndices(
	const U16 *src,
	int numIndices
) {
	if (!src) {
		gls.BindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, GLVertexBuffer::Ref(), false);
		return;
	}

	AllocateDebugIndices(numIndices);

	r::GLVertexBuffer::Ptr::Ref ib = m_debugIndices->Map();
	U16 *dst = (U16*)ib->ptr.get();
	memcpy(dst, src, numIndices * sizeof(U16));
	ib.reset();
}

int GLWorldDraw::DebugTesselateVerts(int numVerts) {

	int numTris = numVerts - 2;
	RAD_VERIFY(numTris > 0);
	AllocateDebugIndices(numTris * 3);

	r::GLVertexBuffer::Ptr::Ref ib = m_debugIndices->Map();
	U16 *indices = (U16*)ib->ptr.get();

	for (int i = 1; i < numVerts-1; ++i) {
		indices[0] = i;
		indices[1] = i+1;
		indices[2] = 0;
		indices += 3;
	}

	ib.reset();
	gls.BindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, m_debugIndices, false);
	return numTris * 3;
}

void GLWorldDraw::DebugDrawLineLoop(int numVerts) {
	gl.DrawArrays(GL_LINE_LOOP, 0, (GLsizei)numVerts);
}

void GLWorldDraw::DebugDrawLineStrip(int numVerts) {
	gl.DrawArrays(GL_LINE_STRIP, 0, (GLsizei)numVerts);
}

void GLWorldDraw::DebugDrawIndexedTris(int numIndices) {
	gl.DrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_SHORT, 0);
}

void GLWorldDraw::DebugDrawIndexedLineLoop(int numIndices) {
	gl.DrawElements(GL_LINE_LOOP, numIndices, GL_UNSIGNED_SHORT, 0);
}

void GLWorldDraw::DebugDrawIndexedLineStrip(int numIndices) {
	gl.DrawElements(GL_LINE_STRIP, numIndices, GL_UNSIGNED_SHORT, 0);
}

void GLWorldDraw::DebugDrawTris(int num) {
	gl.DrawArrays(GL_TRIANGLES, 0, (GLsizei)num);
}

void GLWorldDraw::DebugDrawPoly(int num) {
	int numIndices = DebugTesselateVerts(num);
	gl.DrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_SHORT, 0);
}

} // world

#endif // defined(WORLD_DEBUG_DRAW)
