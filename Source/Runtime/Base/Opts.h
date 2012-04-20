// Opts.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#if defined(_WIN32) && !defined(WIN32)
    #define WIN32 _WIN32
#endif

#if defined(WIN32)
	#include "../Win/WinOpts.h"
#elif defined(_XBOX_VER) && (_XBOX_VER >= 200)
	#include "../XBox360/XBox360Opts.h"
#elif defined(__linux__) || defined(__APPLE__)
	#include "../GCC/GCCOpts.h"
#else
	#error RAD_ERROR_UNSUP_PLAT
#endif

#if RAD_OPT_MACHINE_WORD_SIZE == 4
	#define RAD_OPT_MACHINE_SIZE_32
#elif RAD_OPT_MACHINE_WORD_SIZE == 8
	#define RAD_OPT_MACHINE_SIZE_64
#else
	#error RAD_ERROR_UNSUP_PLAT
#endif
