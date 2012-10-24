// RBackend.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "RBackendDef.h"
#include <Main/NativeDeviceContext.h>
#include <Runtime/PushPack.h>

namespace r {

RAD_REFLECTED_INTERFACE_BEGIN(IRBackend, IInterface, r.IRBackend)

#if defined(RAD_OPT_PC)
	virtual void VidReset() = 0;
	virtual bool CheckCaps() = 0;
#endif
	
	virtual bool VidBind() = 0;

	virtual HContext CreateContext(
		const NativeDeviceContext::Ref &nativeContext
	) = 0;

RAD_INTERFACE_END

} // r

#include <Runtime/PopPack.h>
