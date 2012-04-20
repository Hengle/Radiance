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
#define RAD_OPT_MACHINE_WORD_SIZE 4

#if defined(_MSC_VER)

	// _MSC_VER == 1310 for .NET 2003
	// _MSC_VER == 1400 for Visual Studio 2005
	#if _MSC_VER < 1400
		#error "This version of Microsoft Visual Studio is not supported."
	#endif

	#define RAD_STDEXT stdext
	#define RAD_OPT_VISUAL_C_2005

	//#define _CRT_SECURE_NO_DEPRECATE
	//#define _CRT_NONSTDC_NO_DEPRECATE
	//#define _CRT_NON_CONFORMING_SWPRINTFS

	#define RAD_OPT_VISUAL_C _MSC_VER

	//
	// Set structure alignment.
	//
	#define RAD_STRUCT_ALIGN 8

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

	#define RAD_FASTCALL           __fastcall
	#define RAD_STDCALL            __stdcall
	#define RAD_ANSICALL           __cdecl

	#define RAD_FASTCALL_TRAITS
	#define RAD_STDCALL_TRAITS
	#define RAD_ANSICALL_TRAITS

	#define RAD_THREAD_VAR __declspec(thread)

	#define RAD_FORCEINLINE        __forceinline
	#define RAD_SYMIMPORT          __declspec(dllimport)
	#define RAD_SYMEXPORT          __declspec(dllexport)
	#define RAD_NOVTABLE           __declspec(novtable)

	// Compiler type traits (unavailable from any other method).

	#define RAD_TT_IS_UNION(T)   __is_union(T)

	//#pragma warning (disable:4290) // C++ exception specification ignored except to indicate a function is not __declspec(nothrow)

	// starting in VS2005, alot of the old C style string functions were marked deprecated.
	#pragma warning (disable:4996) // 'X' was declared deprecated
	#pragma warning (disable:4251) // class 'X' needs to have dll-interface to be used by clients of class 'Y'
	#pragma warning (disable:4275) // non dll-interface class 'X' used as base for dll-interface class 'Y'
	#pragma warning (disable:6246) // Local declaration of 'xxx' hides declaration of the same name in outer scope.

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
