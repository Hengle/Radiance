// BackendDef.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#if defined(RAD_OPT_WINX)
	#include "../Win/WinThreadDef.h"
#elif defined(RAD_OPT_POSIXTHREADS)
	#include "../Posix/PosixThreadDef.h"
#else
	#error RAD_ERROR_UNSUP_PLAT
#endif
