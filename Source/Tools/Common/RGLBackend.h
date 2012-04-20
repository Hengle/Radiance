// RGLBackend.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include <Runtime/Base.h>

#if defined(RAD_OPT_WIN)
	#include <Runtime/Win/WinHeaders.h>
	#include <gl/gl.h>
	#include "Win/glext.h"

	#define GL_RGB565 GL_RGB5_EXT
	void glActiveTexture(GLenum texture);
	void glClientActiveTexture(GLenum texture);

	extern PFNGLISRENDERBUFFERPROC glIsRenderbuffer;
	extern PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer;
	extern PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers;
	extern PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers;
	extern PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage;
	extern PFNGLGETRENDERBUFFERPARAMETERIVPROC glGetRenderbufferParameteriv;
	extern PFNGLISFRAMEBUFFERPROC glIsFramebuffer;
	extern PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
	extern PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
	extern PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
	extern PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;
	extern PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer;
	extern PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
	extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC glGetFramebufferAttachmentParameteriv;
	extern PFNGLGENERATEMIPMAPPROC glGenerateMipmap;

#endif
