// Backend.h
// Endian Backend
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#if defined(RAD_OPT_WINX) && defined(RAD_OPT_VISUAL_C_2005)
	#include "../Win/WinEndianIntrinsics.h"
#elif defined(RAD_OPT_GCC) && defined(RAD_OPT_GCC_BSWAP_INTRINSICS)
	#include "../GCC/GCCEndianIntrinsics.h"
#endif
