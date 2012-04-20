// PopSystemMacros.h
// Restore platform macro pollution.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "Base/Opts.h"

#if !defined(PRIVATE_RAD_OPT_PUSH_SYSTEMMACROS)
	#error "umatched/nested <Runtime/PopSystemMacros.h> detected"
#endif

#include RAD_OPT_POPSYSTEMMACROSFILE
#undef PRIVATE_RAD_OPT_PUSH_SYSTEMMACROS
