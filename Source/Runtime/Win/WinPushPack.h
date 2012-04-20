// WinPushPack.h
// Set structure alignment for Windows 95/98/2000/XP.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#if !defined(RAD_OPT_WINX)
	#error "Include error, this file only works under windows!"
#endif

#pragma pack(push, RAD_PRIVATE_STRUCT_ALIGN)