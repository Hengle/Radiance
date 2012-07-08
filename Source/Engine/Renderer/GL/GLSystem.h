// GLSystem.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include <Runtime/Base.h>

#if defined(RAD_OPT_WINX)
	#define WIN32_LEAN_AND_MEAN
	#ifndef NOMINMAX
	#define NOMINMAX	/* Don't defined min() and max() */
	#endif
	#include <windows.h>
	#include <GL/gl.h>
	#include <GL/glu.h>
#elif defined(RAD_OPT_OSX)
	#include <OpenGL/gl.h>
	#include <OpenGL/glu.h>
#elif defined(RAD_OPT_IOS)
	#define RAD_OPT_OGLES
	#define RAD_OPT_OGLES2
	#include <OpenGLES/ES2/gl.h>
	#include <OpenGLES/ES2/glext.h>
	#include "../IOS/GLPrototypes.h"
#else
#error RAD_ERROR_UNSUP_PLAT
#endif

#if defined(RAD_OPT_PC) && defined (RAD_OPT_PC_TOOLS)
#include <Cg/cg.h>
#include <Cg/cgGL.h>
#endif

#include "glext.h"

#if defined(RAD_OPT_WINX)
#include "../PC/wglext.h"
#endif
