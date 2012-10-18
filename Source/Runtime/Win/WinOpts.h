// WinOpts.h
// Sets global switches and defined for Windows 95/98/2000/XP.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#if !defined(WIN32)
	#error "Include error, this file only works under windows!"
#endif

#define RAD_OPT_WINX

#define RAD_OPT_INTEL
#define RAD_OPT_WIN
#define RAD_OPT_LITTLE_ENDIAN

#if defined(_WIN64)
	#define RAD_OPT_MACHINE_WORD_SIZE 8
#else
	#define RAD_OPT_MACHINE_WORD_SIZE 4
#endif

#if defined(_MSC_VER)

	// _MSC_VER == 1310 for .NET 2003
	// _MSC_VER == 1400 for Visual Studio 2005
	#if _MSC_VER < 1400
		#error "This version of Microsoft Visual Studio is not supported."
	#endif

	#define RAD_STDEXT stdext

	#if _MSC_VER >= 1600
		#define RAD_OPT_VISUAL_C_2010
	#else
		#define RAD_OPT_VISUAL_C_2005
	#endif

	//#define _CRT_SECURE_NO_DEPRECATE
	//#define _CRT_NONSTDC_NO_DEPRECATE
	//#define _CRT_NON_CONFORMING_SWPRINTFS

	#define RAD_OPT_VISUAL_C _MSC_VER

	//
	// Set structure alignment.
	//
	#define RAD_STRUCT_ALIGN 8

	#define RAD_ALIGN(_a) __declspec(align(_a))

// http://connect.microsoft.com/VisualStudio/feedback/details/682695/c-alignof-fails-to-properly-evalute-alignment-of-dependent-types
	#define RAD_ALIGNOF(_t) (sizeof(_t) - sizeof(_t) + __alignof(_t))

	//
	// CLR Enabled?
	//
	#if defined(__CLR_VER)
		#define RAD_OPT_CLR __CLR_VER
		#define RAD_CLR_PUBLIC public
	#else
		#define RAD_CLR_PUBLIC
	#endif

	//
	// Run time type information enabled
	//
	#if _CPPRTTI
		#define RAD_OPT_RTTI
	#endif

	//
	// Exception handling enabled
	//
	#ifdef _CPPUNWIND
		#define RAD_OPT_EXCEPTIONS
	#endif

	//
	// Debug enabled
	//
	#if defined(_DEBUG) && !defined(NDEBUG)
		#define RAD_OPT_DEBUG
	#endif

	//
	// Character set
	//
	#if defined(_UNICODE)
		#define RAD_OPT_WSTRING
	#elif defined(_MBCS)
		#define RAD_OPT_MBSTRING
	#else
		#define RAD_OPT_ASCIISTRING
	#endif

	#if defined(_WIN64)
		#define RAD_FASTCALL
		#define RAD_STDCALL
		#define RAD_ANSICALL

		#define RAD_ANSICALL_TRAITS
	#else
		#define RAD_FASTCALL           __fastcall
		#define RAD_STDCALL            __stdcall
		#define RAD_ANSICALL           __cdecl

		#define RAD_FASTCALL_TRAITS
		#define RAD_STDCALL_TRAITS
		#define RAD_ANSICALL_TRAITS
	#endif

	#define RAD_THREAD_VAR __declspec(thread)

	#define RAD_BEGIN_STD_NAMESPACE namespace std {
	#define RAD_END_STD_NAMESPACE }

	#define RAD_FORCEINLINE        __forceinline
	#define RAD_SYMIMPORT          __declspec(dllimport)
	#define RAD_SYMEXPORT          __declspec(dllexport)
	#define RAD_NOVTABLE           __declspec(novtable)

	// Compiler type traits (unavailable from any other method).

	#define RAD_TT_IS_UNION(T)   __is_union(T)

#else

	#error "unrecognized target compiler"

#endif

#define RAD_OPT_PUSHPACKFILE         "./Win/WinPushPack.h"
#define RAD_OPT_POPPACKFILE          "./Win/WinPopPack.h"
#define RAD_OPT_PUSHSYSTEMMACROSFILE "./Win/WinPushMacros.h"
#define RAD_OPT_POPSYSTEMMACROSFILE  "./Win/WinPopMacros.h"

#if defined(RAD_OPT_DLL) && !defined(RADRT_OPT_STATIC)
	#if defined(RADRT_OPT_EXPORT)
		#define RADRT_API                 RAD_SYMEXPORT
		#define RADRT_TEMPLATE_EXTERN
	#else
		#define RADRT_API                 RAD_SYMIMPORT
		#define RADRT_TEMPLATE_EXTERN     extern
	#endif
#else
	#define RADRT_API
	#define RADRT_TEMPLATE_EXTERN
#endif

#define RADRT_CALL  RAD_ANSICALL
#define RADRT_CLASS RADRT_API
#define RADRT_TEMPLATE_EXPLICIT(_c_) RADRT_TEMPLATE_EXTERN template class RADRT_CLASS _c_

#include "WinTypes.h"
