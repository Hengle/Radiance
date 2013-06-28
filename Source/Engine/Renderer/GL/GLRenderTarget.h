// GLRenderTarget.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "GLTable.h"
#include "GLTexture.h"
#include "../RendererDef.h"
#include <Runtime/PushPack.h>

namespace r {

class RADENG_CLASS GLRenderTarget
{
public:

	typedef boost::shared_ptr<GLRenderTarget> Ref;
	
	GLRenderTarget(
		GLenum target, 
		GLenum format, 
		GLenum type,
		GLsizei width, 
		GLsizei height, 
		GLsizei depthFormat, 
		GLsizei size,
		int flags,
		bool autoGenMips
	);

	virtual ~GLRenderTarget();

	GLTexture::Ref tex;
	GLenum depthFormat;
	int depthSize;
	GLuint id[2];
};

} // r

#include <Runtime/PopPack.h>
