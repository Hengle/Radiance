// WinOpts.h
// Platform options for Windows 95/98/2000/XP.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once

#if !defined(RAD_OPT_WINX)
	#error "Include error, this file only works under Windows!"
#endif

#if defined(__RADREFLECT_REFLECTMAP_H__)
	#pragma warning(disable : 4503)

	#if defined(RAD_OPT_REPORT_DISABLED_WARNINGS)
		#pragma message("Radiance Reflection: disabling warning C4503, maximum decorated name length exceeded")
	#endif
#endif // defined(__RADREFLECT_REFLECTMAP_H__)
