// RBackend.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// PC Rendering Backend
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "RBackendDef.h"
#include <Runtime/PushPack.h>

#if defined(RAD_OPT_PC_TOOLS)
class QGLFormat;
#endif

namespace r {

#if defined(RAD_OPT_PC_TOOLS)
RADENG_API void RADENG_CALL SetQGLFormat(QGLFormat &fmt);
#endif

enum
{
	RAD_FLAG(ForceWidth),
	RAD_FLAG(ForceHeight),
	RAD_FLAG(MinBPP),
	RAD_FLAG(ForceHz),
	RAD_FLAG(Fullscreen),
	RAD_FLAG(Windowed),
	RAD_FLAG(_4x3),
	RAD_FLAG(_16x9),
	RAD_FLAG(_16x10),
	Default = Fullscreen|Windowed|_4x3|_16x9|_16x10
};

RAD_REFLECTED_INTERFACE_BEGIN(IRBackend, IInterface, r.IRBackend)

	virtual VidModeVecRef VidModeList(
		int width=0, 
		int height=0, 
		int bpp=0,
		int hz=0,
		int filter=Default
	) = 0;

	virtual bool CheckVidMode(const VidMode &mode) = 0;
	virtual bool SetVidMode(const VidMode &mode) = 0;
	virtual void VidReset() = 0;
	virtual bool VidBind() = 0;
	virtual bool CheckCaps() = 0;

	// selects the closest valid video mode.
	virtual void SelectVidMode(VidMode &mode) = 0;

	virtual HContext BindContext() = 0;

RAD_INTERFACE_END

} // r

#include <Runtime/PopPack.h>
