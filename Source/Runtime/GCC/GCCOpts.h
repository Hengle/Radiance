// GCCOpts.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#if defined(__ppc__)
	#define RAD_OPT_PPC
#elif defined(__arm__)
	#define RAD_OPT_ARM
#else
	#define RAD_OPT_INTEL
#endif

#if defined(__linux__)
	#define RAD_OPT_LINUX
	#define RAD_OPT_ANSI_STRINGBASE
#elif defined(__APPLE__)
	#define RAD_OPT_APPLE
	#include <TargetConditionals.h>
	#if TARGET_OS_MAC==1
		#define RAD_OPT_OSX
	#elif TARGET_OS_IPHONE==1
		#define RAD_OPT_IOS
		#if defined(TARGET_IPHONE_SIMULATOR)
			#define RAD_OPT_IOS_SIMULATOR
		#else
			#define RAD_OPT_IOS_DEVICE
		#endif
	#else
		#error RAD_ERROR_UNSUP_PLAT
	#endif
	#if defined(TARGET_RT_64_BIT)
		#define RAD_OPT_MACHINE_WORD_SIZE 8
	#else
		#define RAD_OPT_MACHINE_WORD_SIZE 4
	#endif
	#define RAD_OPT_ANSI_STRINGBASE
	#define RAD_OPT_PTHREAD_NO_SPINLOCK
#else
	#error RAD_ERROR_UNSUP_PLAT
#endif

#if defined(__BIG_ENDIAN__)
	#define RAD_OPT_BIG_ENDIAN
#else
	#define RAD_OPT_LITTLE_ENDIAN
#endif

#if defined(__cplusplus) && defined(__GNUC__)

	#if __GNUC__ < 4
		#error "G++ 4.0 or higher is required."
	#endif

	#define RAD_STDEXT __gnu_cxx

	#define RAD_OPT_GNUC __GNUC__
	#define RAD_OPT_GCC  __GNUC__
	#define RAD_OPT_GCC_MINOR __GNUC_MINOR__
	#define RAD_OPT_GCC_PATCH __GNUC_PATCHLEVEL__

	#if __GNUC__ > 4 || __GNUC_MINOR__ >= 4
		#define RAD_OPT_GCC_BSWAP_INTRINSICS
	#endif

	#define RAD_STRUCT_ALIGN 8

	#if _CPPRTTI
		#define RAD_OPT_RTTI
	#endif

	#ifdef _CPPUNWIND
		#define RAD_OPT_EXCEPTIONS
	#endif

	#if defined(_DEBUG) && !defined(NDEBUG)
		#define RAD_OPT_DEBUG
	#endif

	#define RAD_OPT_4BYTE_WCHAR        // 4 byte wchar_t platform.

	#define RAD_FASTCALL
	#define RAD_STDCALL
	#define RAD_ANSICALL
	#define RAD_ANSICALL_TRAITS

	#define RAD_THREAD_VAR __thread

	#define RAD_FORCEINLINE inline
	#define RAD_SYMIMPORT   __attribute__ ((visibility("default")))
	#define RAD_SYMEXPORT   __attribute__ ((visibility("default")))
	#define RAD_NOVTABLE

	#define RAD_CLR_PUBLIC

	// Compiler type traits (unavailable from any other method).

	#define RAD_TT_IS_UNION(T)   false

#else

	#error RAD_ERROR_UNSUP_PLAT

#endif

#define RAD_OPT_PUSHPACKFILE         "./GCC/GCCPushPack.h"
#define RAD_OPT_POPPACKFILE          "./GCC/GCCPopPack.h"
#define RAD_OPT_PUSHSYSTEMMACROSFILE "./GCC/GCCPushSystemMacros.h"
#define RAD_OPT_POPSYSTEMMACROSFILE  "./GCC/GCCPopSystemMacros.h"

#define RADRT_API   __attribute__ ((visibility("default")))
#define RADRT_CALL
#define RADRT_CLASS RADRT_API
#define RADRT_TEMPLATE_EXTERN
#define RADRT_TEMPLATE_EXPLICIT(_c_) RADRT_TEMPLATE_EXTERN template class RADRT_CLASS _c_

#include "GCCTypes.h"
