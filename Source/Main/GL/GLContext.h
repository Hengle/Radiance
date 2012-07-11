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
	int red;
	int green;
	int blue;
	int alpha;
	int depth;
	int stencil;
	int mSamples;
	bool doublebuffer;
};

class GLDeviceContext : public NativeDeviceContext {
public:
	typedef boost::shared_ptr<GLDeviceContext> Ref;

	virtual void Bind() = 0;
	virtual void Unbind() = 0;
	virtual void SwapBuffers() = 0;
};
