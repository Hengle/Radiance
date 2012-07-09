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
	int flags
) : depthFormat(_depthFormat), depthSize(0)
{
	id[0] = id[1] = 0;
	gl.GenFramebuffersEXT(1, &id[0]);
	CHECK_GL_ERRORS();
	RAD_ASSERT(id[0]);
	gls.BindBuffer(GL_FRAMEBUFFER_EXT, id[0]);

	if (depthFormat != 0)
	{
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
		switch (depthFormat)
		{
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
	GLTexture::SetFlags(tex, flags, 0, false);
	
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

GLRenderTarget::~GLRenderTarget()
{
	if (id[0])
		gl.DeleteFramebuffersEXT(1, &id[0]);
	if (id[1])
	{
		gl.DeleteRenderbuffersEXT(1, &id[1]);
		ZTextures.Get().Dec(depthSize, 0);
	}
}

} // r
