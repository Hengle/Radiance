// GLRenderTarget.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "GLRenderTarget.h"
#include "GLState.h"

namespace r {

GLRenderTarget::GLRenderTarget(
	GLenum target, 
	GLenum format,
	GLenum type,
	GLsizei width, 
	GLsizei height, 
	GLsizei _depthFormat, 
	GLsizei size,
	int flags,
	bool autoGenMips
) : depthFormat(_depthFormat), depthSize(0) {
	id[0] = id[1] = 0;
	gl.GenFramebuffersEXT(1, &id[0]);
	CHECK_GL_ERRORS();
	RAD_ASSERT(id[0]);
	gls.BindBuffer(GL_FRAMEBUFFER_EXT, id[0]);

	if (depthFormat != 0) {
		gl.GenRenderbuffersEXT(1, &id[1]);
		CHECK_GL_ERRORS();
		RAD_ASSERT(id[1]);
		gls.BindBuffer(GL_RENDERBUFFER_EXT, id[1]);
		CHECK_GL_ERRORS();
		gl.RenderbufferStorageEXT(GL_RENDERBUFFER_EXT, depthFormat, width, height);
		CHECK_GL_ERRORS();
		gl.FramebufferRenderbufferEXT(
			GL_FRAMEBUFFER_EXT,
			GL_DEPTH_ATTACHMENT_EXT,
			GL_RENDERBUFFER_EXT,
			id[1]
		);
		CHECK_GL_ERRORS();
		switch (depthFormat) {
		case GL_DEPTH_COMPONENT24_ARB:
			depthSize = width*height*3;
			break;
#if defined(RAD_OPT_PC)
		case GL_DEPTH_COMPONENT32_ARB:
			depthSize = width*height*4;
			break;
#endif
		}
		ZTextures.Get().Inc(depthSize, 0);
	}

	GLenum iformat = GLInternalFormat(format, type);

	tex.reset(new (ZRender) GLTexture(
		target,
		iformat,
		width,
		height,
		0,
		size
	));

	gls.SetTexture(0, tex, true);
	GLTexture::SetFlags(tex, flags, 0, autoGenMips);
	
	RAD_ASSERT(tex->target==GL_TEXTURE_2D);

	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		iformat,
		width,
		height,
		0,
		format,
		type,
		0
	);

	CHECK_GL_ERRORS();

	gl.FramebufferTexture2DEXT(
		GL_FRAMEBUFFER_EXT,
		GL_COLOR_ATTACHMENT0_EXT,
		GL_TEXTURE_2D,
		tex->id,
		0
	);

	CHECK_GL_ERRORS();
	RAD_ASSERT(gl.CheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) == GL_FRAMEBUFFER_COMPLETE_EXT);
}

GLRenderTarget::GLRenderTarget(
	const GLTexture::Ref &_tex
) : tex(_tex), depthFormat(0), depthSize(0) {
	id[0] = id[1] = 0;
	gl.GenFramebuffersEXT(1, &id[0]);
	CHECK_GL_ERRORS();
	RAD_ASSERT(id[0]);
	gls.BindBuffer(GL_FRAMEBUFFER_EXT, id[0]);

	gl.FramebufferTexture2DEXT(
		GL_FRAMEBUFFER_EXT,
		GL_COLOR_ATTACHMENT0_EXT,
		GL_TEXTURE_2D,
		tex->id,
		0
	);

	CHECK_GL_ERRORS();
	RAD_ASSERT(gl.CheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) == GL_FRAMEBUFFER_COMPLETE_EXT);
}

void GLRenderTarget::CreateDepthBufferTexture() {
	RAD_ASSERT(tex);
	gls.BindBuffer(GL_FRAMEBUFFER_EXT, id[0]);

	depthTex.reset(new (ZRender) GLTexture(
		GL_TEXTURE_2D,
		GL_DEPTH_COMPONENT32_ARB,
		tex->width,
		tex->height,
		0,
		tex->width*tex->height*4
	));

	gls.SetTexture(0, depthTex, true);
	GLTexture::SetFlags(depthTex, 0, 0, false);

#if defined(RAD_OPT_PC)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
#endif
	
	RAD_ASSERT(depthTex->target==GL_TEXTURE_2D);

	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_DEPTH_COMPONENT32_ARB,
		tex->width,
		tex->height,
		0,
		GL_DEPTH_COMPONENT,
		GL_UNSIGNED_INT,
		0
	);

	CHECK_GL_ERRORS();

	gl.FramebufferTexture2DEXT(
		GL_FRAMEBUFFER_EXT,
		GL_DEPTH_ATTACHMENT_EXT,
		GL_TEXTURE_2D,
		depthTex->id,
		0
	);

	CHECK_GL_ERRORS();
	RAD_ASSERT(gl.CheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) == GL_FRAMEBUFFER_COMPLETE_EXT);
}

void GLRenderTarget::AttachDepthBuffer(const GLTexture::Ref &tex) {
	RAD_ASSERT(tex);
	gls.BindBuffer(GL_FRAMEBUFFER_EXT, id[0]);

	depthTex = tex;

	gl.FramebufferTexture2DEXT(
		GL_FRAMEBUFFER_EXT,
		GL_DEPTH_ATTACHMENT_EXT,
		GL_TEXTURE_2D,
		depthTex->id,
		0
	);

	CHECK_GL_ERRORS();
	RAD_ASSERT(gl.CheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) == GL_FRAMEBUFFER_COMPLETE_EXT);
}

GLRenderTarget::~GLRenderTarget() {
	if (id[0])
		gl.DeleteFramebuffersEXT(1, &id[0]);
	if (id[1]) {
		gl.DeleteRenderbuffersEXT(1, &id[1]);
		ZTextures.Get().Dec(depthSize, 0);
	}
}

void GLRenderTarget::BindTexture(int index) {
	if (index == -1) {
		for (int i = 0; i < kMaterialTextureSource_MaxIndices; ++i) {
			if (!gls.MaterialTextureSource(kMaterialTextureSource_Texture, i))
				gls.SetMTSource(kMaterialTextureSource_Texture, i, tex);
		}
	} else {
		gls.SetMTSource(kMaterialTextureSource_Texture, index, tex);
	}
}

void GLRenderTarget::BindFramebuffer(DiscardFlags flags) {
	gls.BindBuffer(GL_FRAMEBUFFER_EXT, id[0]);
	CHECK_GL_ERRORS();
	GLRenderTarget::DiscardFramebuffer(flags);
		
	GLbitfield mask = 0;
	int glsFlags = kScissorTest_Disable;

	if (flags&kDiscard_Color) {
		mask |= GL_COLOR_BUFFER_BIT;
		glsFlags |= kColorWriteMask_RGBA;
	}

	if (flags&kDiscard_Depth) {
		mask |= GL_DEPTH_BUFFER_BIT;
		glsFlags |= kDepthWriteMask_Enable;
	}

	gls.Viewport(0, 0, tex->width, tex->height);
	
	if (mask) {
		gls.Set(glsFlags, -1, true); // for glClear()
		glClear(mask|GL_STENCIL_BUFFER_BIT);
	}
}

void GLRenderTarget::DiscardFramebuffer(DiscardFlags flags) {
#if defined(RAD_OPT_OGLES)
	if (flags == kDiscard_None)
		return;

	GLenum attachments[2] = { 0, 0 };
	int num = 0;

	if (flags&kDiscard_Color)
		attachments[num++] = GL_COLOR_ATTACHMENT0_EXT;
	if (flags&kDiscard_Depth)
		attachments[num++] = GL_DEPTH_ATTACHMENT_EXT;
    glDiscardFramebufferEXT(GL_FRAMEBUFFER_EXT, num, attachments);
#endif
}

///////////////////////////////////////////////////////////////////////////////

GLRenderTargetCache::GLRenderTargetCache() : 
m_format(0), 
m_type(0), 
m_depth(0), 
m_flags(0),
m_next(0),
m_colorSize(0),
m_depthSize(0),
m_width(0),
m_height(0),
m_depthMode(kDepthInstanceMode_None) {
}

GLRenderTargetCache::GLRenderTargetCache(
	GLsizei width,
	GLsizei height,
	GLenum format,
	GLenum type,
	GLenum depth,
	DepthInstanceMode depthMode,
	int flags,
	int colorBytesPP,
	int depthBytesPP
) { 
	SetFormats(
		width,
		height,
		format,
		type,
		depth,
		depthMode,
		flags,
		colorBytesPP,
		depthBytesPP
	);
}

void GLRenderTargetCache::SetFormats(
	GLsizei width,
	GLsizei height,
	GLenum format,
	GLenum type,
	GLenum depth,
	DepthInstanceMode depthMode,
	int flags,
	int colorBytesPP,
	int depthBytesPP
) {
	RAD_ASSERT(width>0);
	RAD_ASSERT(height>0);
	RAD_ASSERT(format);
	RAD_ASSERT(type);
	RAD_ASSERT((depthMode == kDepthInstanceMode_None) || depth);
	RAD_ASSERT(colorBytesPP>0);

	Clear();
	m_width = width;
	m_height = height;
	m_format = format;
	m_type = type;
	m_depthMode = depthMode;
	if (depthMode != kDepthInstanceMode_None) {
		m_depth = depth;
		m_depthSize = depthBytesPP*width*height;
	} else {
		m_depth = 0;
		m_depthSize = 0;
	}
	m_flags = flags;
	m_next = 0;
	m_colorSize = colorBytesPP*width*height;
}

void GLRenderTargetCache::CreateRenderTargets(int num) {
	RAD_ASSERT(num > 0);
	Clear();

	m_next = 0;
	
	// first one always has depth buffer if set
	GLRenderTarget::Ref base(new (ZRender) GLRenderTarget(
		GL_TEXTURE_2D,
		m_format,
		m_type,
		m_width,
		m_height,
		m_depth,
		m_colorSize+m_depthSize,
		m_flags,
		false
	));

	m_vec.push_back(base);

	if (num < 2)
		return;

	for (int i = 1; i < num; ++i) {
		GLRenderTarget::Ref t(new (ZRender) GLRenderTarget(
			GL_TEXTURE_2D,
			m_format,
			m_type,
			m_width,
			m_height,
			(m_depthMode == kDepthInstanceMode_Unique) ? m_depth : 0,
			(m_depthMode == kDepthInstanceMode_Unique) ? (m_colorSize+m_depthSize) : m_colorSize,
			m_flags,
			false
		));

		if (m_depthMode == kDepthInstanceMode_Shared) {
			gls.BindBuffer(GL_FRAMEBUFFER_EXT, t->id[0]);
			CHECK_GL_ERRORS();
			gls.BindBuffer(GL_RENDERBUFFER_EXT, base->id[1]);
			CHECK_GL_ERRORS();
			gl.FramebufferRenderbufferEXT(
				GL_FRAMEBUFFER_EXT,
				GL_DEPTH_ATTACHMENT_EXT,
				GL_RENDERBUFFER_EXT,
				base->id[1]
			);
			CHECK_GL_ERRORS();
			RAD_ASSERT(gl.CheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) == GL_FRAMEBUFFER_COMPLETE);
		}

		m_vec.push_back(t);
	}
}

GLRenderTarget::Ref GLRenderTargetCache::NextRenderTarget(bool wrap) {
	if (m_next >= (int)m_vec.size()) {
		if (wrap) {
			m_next = 0;
		} else {
			return GLRenderTarget::Ref();
		}
	}

	GLRenderTarget::Ref t = m_vec[m_next++];
	return t;
}

void GLRenderTargetCache::Reset() {
	m_next = 0;
}

void GLRenderTargetCache::Clear() {
	m_vec.clear();
}

///////////////////////////////////////////////////////////////////////////////

GLRenderTargetMultiCache::GLRenderTargetMultiCache() : 
m_colorBytesPP(0), 
m_depthBytesPP(0), 
m_flags(0), 
m_numRenderTargets(0),
m_format(0),
m_type(0),
m_depth(0),
m_depthMode(GLRenderTargetCache::kDepthInstanceMode_None) {

}

GLRenderTargetMultiCache::GLRenderTargetMultiCache(
	int numRenderTargets,
	GLenum format,
	GLenum type,
	GLenum depth,
	GLRenderTargetCache::DepthInstanceMode depthMode,
	int flags,
	int colorBytesPP,
	int depthBytesPP
) {
	SetFormats(numRenderTargets, format, type, depth, depthMode, flags, colorBytesPP, depthBytesPP);
}

void GLRenderTargetMultiCache::SetFormats(
	int numRenderTargets,
	GLenum format,
	GLenum type,
	GLenum depth,
	GLRenderTargetCache::DepthInstanceMode depthMode,
	int flags,
	int colorBytesPP,
	int depthBytesPP
) {
	RAD_ASSERT(numRenderTargets>0);

	Clear();

	m_numRenderTargets = numRenderTargets;
	m_format = format;
	m_type = type;
	m_depth = depth;
	m_depthMode = depthMode;
	m_flags = flags;
	m_colorBytesPP = colorBytesPP;
	m_depthBytesPP = depthBytesPP;
}

GLRenderTarget::Ref GLRenderTargetMultiCache::NextRenderTarget(
	GLsizei width,
	GLsizei height,
	bool wrap
) {
	RAD_ASSERT(width>0);
	RAD_ASSERT(height>0);

	const Key kKey(width, height);

	Map::iterator it = m_map.find(kKey);
	if (it != m_map.end()) {
		return it->second->NextRenderTarget(wrap);
	}

	GLRenderTargetCache::Ref cache(
		new (ZRender) GLRenderTargetCache(
			width,
			height,
			m_format,
			m_type,
			m_depth,
			m_depthMode,
			m_flags,
			m_colorBytesPP,
			m_depthBytesPP
	));

	cache->CreateRenderTargets(m_numRenderTargets);

	m_map[kKey] = cache;
	return cache->NextRenderTarget(wrap);
}

void GLRenderTargetMultiCache::Clear() {
	m_map.clear();
}

} // r
