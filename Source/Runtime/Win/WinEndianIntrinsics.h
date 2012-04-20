// WinEndianInstrinsics.h
// Inline Win32 Endian Intrinsics.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include <stdlib.h>

#define RADENDIAN_OPT_BYTESWAP_INTRINSICS

namespace endian {

#if _INTEGRAL_MAX_BITS < 64
#define RADENDIAN_OPT_NEED_DEFAULT_64SWAP
#else
inline RADRT_API U64 RADRT_CALL Swap(U64 val)
{
	return _byteswap_uint64(val);
}
#endif

inline RADRT_API U16 RADRT_CALL Swap(U16 val)
{
	return _byteswap_ushort(val);
}

inline RADRT_API U32 RADRT_CALL Swap(U32 val)
{
	return _byteswap_ulong(val);
}

} // endian
