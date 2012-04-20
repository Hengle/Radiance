// WinOpts.h
// Sets global switches and defined for Windows 95/98/2000/XP.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#if defined(RAD_OPT_DLL) && !defined(RADENG_OPT_STATIC)
	#if defined(RADENG_OPT_EXPORT)
		#define RADENG_API                 RAD_SYMEXPORT
		#define RADENG_TEMPLATE_EXTERN
	#else
		#define RADENG_API                 RAD_SYMIMPORT
		#define RADENG_TEMPLATE_EXTERN     extern
	#endif
#else
	#define RADENG_API
	#define RADENG_TEMPLATE_EXTERN
#endif

#define RADENG_CALL  RAD_ANSICALL
#define RADENG_CLASS RADENG_API
#define RADENG_TEMPLATE_EXPLICIT(_c_) RADENG_TEMPLATE_EXTERN template class RADENG_CLASS _c_
