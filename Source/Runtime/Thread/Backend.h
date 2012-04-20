// Backend.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "ThreadDef.h"

#if defined(RAD_OPT_WINX)
	#include "../Win/WinThread.h"
#elif defined(RAD_OPT_POSIXTHREADS)
	#include "../Posix/PosixThread.h"
#else
	#error RAD_ERROR_UNSUP_PLAT
#endif
