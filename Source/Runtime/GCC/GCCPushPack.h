// GCCPushPack.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#if !defined(RAD_OPT_GCC)
	#error "this file can only be included by a gcc build"
#endif

#if defined(RAD_STRUCT_ALIGN_OVERRIDE)
	#define RAD_PRIVATE_STRUCT_ALIGN RAD_STRUCT_ALIGN_OVERRIDE
#else
	#define RAD_PRIVATE_STRUCT_ALIGN RAD_STRUCT_ALIGN
#endif

#if RAD_PRIVATE_STRUCT_ALIGN == 1
	#pragam pack(push, 1)
#elif RAD_PRIVATE_STRUCT_ALIGN == 2
	#pragma pack(push, 2)
#elif RAD_PRIVATE_STRUCT_ALIGN == 4
	#pragma pack(push, 4)
#elif RAD_PRIVATE_STRUCT_ALIGN == 8
	#pragma pack(push, 8)
#elif RAD_PRIVATE_STRUCT_ALIGN == 16
	#pragma pack(push, 16)
#elif RAD_PRIVATE_STRUCT_ALIGN == 32
	#pragma pack(push, 32)
#elif RAD_PRIVATE_STRUCT_ALIGN == 64
	#pragma pack(push, 64)
#elif RAD_PRIVATE_STRUCT_ALIGN == 128
	#pragma pack(push, 128)
#elif RAD_PRIVATE_STRUCT_ALIGN == 256
	#pragma pack(push, 256)
#else
	#error "gcc invalid struct alignment"
#endif

#undef RAD_PRIVATE_STRUCT_ALIGN
