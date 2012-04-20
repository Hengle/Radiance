// IntInterface.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Base.h"

#if defined(RAD_OPT_WINX) 
	#include "../Win/WinInterface.h"
#elif defined(RAD_OPT_GCC)
	#include "../GCC/GCCInterface.h"
#else
	#error RAD_ERROR_UNSUP_PLAT
#endif
