/*! \file GLContext.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup Main
*/

#pragma once

#include <Runtime/Base.h>
#include "../NativeDeviceContext.h"

struct GLPixelFormat {

	GLPixelFormat() :
	red(0),
	green(0),
	blue(0),
	alpha(0),
	depth(0),
	stencil(0),
	mSamples(0),
	doubleBuffer(false)
	{
	}

	int red;
	int green;
	int blue;
	int alpha;
	int depth;
	int stencil;
	int mSamples;
	bool doubleBuffer;
};

class GLDeviceContext : public NativeDeviceContext {
public:
	typedef boost::shared_ptr<GLDeviceContext> Ref;

	virtual void Bind() = 0;
	virtual void Unbind() = 0;
	virtual void SwapBuffers() = 0;
	virtual void BindFramebuffer() = 0;
};
