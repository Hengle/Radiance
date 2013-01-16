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

#define RAD_REQ_WINVER 0x502

//
// Is windows already included?
//
#if defined(_WINDOWS_)
	
	// Check the version.
	#if !defined(_WIN32_WINNT) || (_WIN32_WINNT < RAD_REQ_WINVER) || !defined(WINVER) || (WINVER < RAD_REQ_WINVER)
		#error <windows.h> was included before base with a _WIN32_WINNT version < 0x500. Please define _WIN32_WINNT to be >= 0x500"
	#endif

#else

	#if !defined(_WIN32_WINNT) || (_WIN32_WINNT < RAD_REQ_WINVER)
		#undef _WIN32_WINNT
		#define _WIN32_WINNT RAD_REQ_WINVER
	#endif

	#if !defined(WINVER) || (WINVER < RAD_REQ_WINVER)
		#undef WINVER
		#define WINVER RAD_REQ_WINVER
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