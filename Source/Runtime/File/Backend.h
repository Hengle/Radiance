// Backend.h
// Platform Agnostic File System
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#if defined(RAD_OPT_WIN)
	#include "../Win/WinFile.h"
#elif defined(RAD_OPT_POSIXFILES)
	#include "../Posix/PosixFile.h"
#else
	#error RAD_ERROR_UNSUP_PLAT
#endif
