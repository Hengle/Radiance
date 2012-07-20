// Opts.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include <Runtime/Base.h>

#if defined(RAD_OPT_WINX)
	#include "Win/WinOpts.h"
#elif defined(RAD_OPT_GCC)
	#include "GCC/GCCOpts.h"
#else
	#error RAD_ERROR_UNSUP_PLAT
#endif
