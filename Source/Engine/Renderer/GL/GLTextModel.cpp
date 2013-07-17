// GLTextModel.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "GLTextModel.h"
#include "GLState.h"
#include "GLTexture.h"

//#define DUMP_FONT_PAGE

#if defined(DUMP_FONT_PAGE)
#include <stdio.h>
#endif

namespace r {

namespace {

struct GlyphPage : public font::IGlyphPage {
	GlyphPage(int _width, int _height) {
		width = _width;
		height = _height;
		data = safe_zone_malloc(ZFonts, width*height);

		tex.reset(new (ZRender) GLTexture(width*height));
		tex->target = GL_TEXTURE_2D;
		tex->width = width;
		tex->height = height;
		tex->format = GLInternalFormat(GL_ALPHA, GL_UNSIGNED_BYTE);
		GLTexture::SetFlags(tex, TX_FilterBilinear);
	}

	virtual ~GlyphPage() {
		zone_free(data);
	}

	virtual void *Lock(AddrSize &stride) {
		stride = width;
		return data;
	}

	virtual void Unlock() {
		gls.SetTexture(0, tex, true);
		glTexImage2D(
			tex->target,
			0,
			tex->format,
			width,
			height,
			0,
			GL_ALPHA,
			GL_UNSIGNED_BYTE,
			data
		);
#if defined(DUMP_FONT_PAGE)
		static int s_num = 1;
		char buff[256];
		sprintf(buff, "fontpage_%d.raw", s_num++);
		FILE *fp = fopen(buff, "wb");
		fwrite(data, 1, width*height, fp);
		fclose(fp);
#endif
	}

	int width;
	int height;
	void *data;
	GLTexture::Ref tex;
};

} // namespace

TextModel::Ref TextModel::New() {
	return TextModel::Ref(new (ZFonts) GLTextModel());
}

font::IGlyphPage::Ref TextModel::AllocatePage(int width, int height) {
	return font::IGlyphPage::Ref(new (ZFonts) GlyphPage(width, height));
}

GLTextModel::VertexType *GLTextModel::LockVerts(int num) {
	ReserveVerts(num);
	m_lock = m_mesh->Map(m_streamId);
	return (VertexType*)m_lock->ptr.get();
}

void GLTextModel::UnlockVerts() {
	m_lock.reset();
}

void GLTextModel::ReserveVerts(int num) {
	int size = sizeof(VertexType)*num;

	if (!m_mesh || (m_streamSize < size)) {
		m_streamSize = size;
		RAD_ASSERT(!m_lock);
		m_mesh.reset(new (ZRender) Mesh());
		m_streamId = m_mesh->AllocateStream(kStreamUsage_Dynamic, sizeof(VertexType), num);
		m_mesh->MapSource(
			m_streamId,
			kMaterialGeometrySource_Vertices,
			0,
			sizeof(VertexType),
			0,
			2
		);
		m_mesh->MapSource(
			m_streamId,
			kMaterialGeometrySource_TexCoords,
			0,
			sizeof(VertexType),
			sizeof(float)*2,
			2
		);
	}
}

void GLTextModel::BindStates(const r::Material &material, int ofs, int count, font::IGlyphPage &page) {
	m_mesh->BindAll(material.shader.get().get());
	gls.SetMTSource(
		kMaterialTextureSource_Texture,
		0,
		static_cast<GlyphPage&>(page).tex
	);
}

void GLTextModel::DrawVerts(const r::Material &material, int ofs, int count) {
	m_mesh->CompileArrayStates(*material.shader.get());
	glDrawArrays(GL_TRIANGLES, ofs*6, count*6);
}

} // r
