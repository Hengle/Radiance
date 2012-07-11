/*! \file NativeAppBackend.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup Main
*/

#pragma once

#include <Runtime/Base.h>

#if defined(RAD_OPT_WIN)
#include "Win/WinNativeApp.h"
#else
#error RAD_ERROR_UNSUP_PLAT
#endif
