// PushSystemMacros.h
// Cleanup platform macro pollution.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "Base/Opts.h"

#if defined(PRIVATE_RAD_OPT_PUSH_SYSTEMMACROS)
	#error "unmatched/nested <Runtime/PushSystemMacros.h> detected!"
#endif

#define PRIVATE_RAD_OPT_PUSH_SYSTEMMACROS
#include RAD_OPT_PUSHSYSTEMMACROSFILE
