// GCCEndianInstrinsics.inl
// Inline Win32 Endian Intrinsics.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#define RADENDIAN_OPT_BYTESWAP_INTRINSICS


namespace endian {

inline RADRT_API U64 RADRT_CALL Swap(U64 val)
{
	return __builtin_bswap64(val);
}

inline RADRT_API U16 RADRT_CALL Swap(U16 val)
{
	return (val << 8) | (val >> 8);
}

inline RADRT_API U32 RADRT_CALL Swap(U32 val)
{
	return  __builtin_bswap32(val);
}

} // endian

