// WinHeaders.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Base.h"

//////////////////////////////////////////////////////////////////////////////////////////

#if !defined(RAD_OPT_WIN)
	#error "Include error, this file only works under windows!"
#endif

//
// Is windows already included?
//
#if defined(_WINDOWS_)
	
	// Check the version.
	#if !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0403)
		#error <windows.h> was included before base with a _WIN32_WINNT version < 0x0403. Please define _WIN32_WINNT to be >= 0x0403"
		//
		// If you got the above error, copy and paste the following code before you include windows.h
		//
		//#if !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0403)
		//	#undef _WIN32_WINNT
		//	#define _WIN32_WINNT 0x0403
		//#endif
		//
	#endif

#else

	#if !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0403)
		#undef _WIN32_WINNT
		#define _WIN32_WINNT 0x0403
	#endif

	#if defined(NULL)
		#undef NULL
	#endif

	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX

	#include <windows.h>
	#include <winsock2.h>

	#undef WIN32_LEAN_AND_MEAN
	#undef NOMINMAX

#endif