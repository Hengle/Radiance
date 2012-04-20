// PushPack.h
// Set structure alignment.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "Base/Opts.h"

#if defined(PRIVATE_RAD_OPT_PUSH_PACK)
	#error "umatched/nested 'Runtime/PushPack.h' detected"
#endif

#if defined(RAD_STRUCT_ALIGN_OVERRIDE)
	#define RAD_PRIVATE_STRUCT_ALIGN RAD_STRUCT_ALIGN_OVERRIDE
#else
	#define RAD_PRIVATE_STRUCT_ALIGN RAD_STRUCT_ALIGN
#endif

#define PRIVATE_RAD_OPT_PUSH_PACK
#include RAD_OPT_PUSHPACKFILE
#undef RAD_PRIVATE_STRUCT_ALIGN

#include "PushSystemMacros.h"
