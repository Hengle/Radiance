// Endian.inl
// Inline Endian Conversion.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.


namespace endian {

//////////////////////////////////////////////////////////////////////////////////////////
// endian::SwapLittle()
//////////////////////////////////////////////////////////////////////////////////////////

inline RADRT_API S16 RADRT_CALL SwapLittle(S16 i)
{
#if defined(RAD_OPT_BIG_ENDIAN)
	return Swap(i);
#else
	return i;
#endif
}

inline RADRT_API U16 RADRT_CALL SwapLittle(U16 i)
{
#if defined(RAD_OPT_BIG_ENDIAN)
	return Swap(i);
#else
	return i;
#endif
}

inline RADRT_API S32 RADRT_CALL SwapLittle(S32 i)
{
#if defined(RAD_OPT_BIG_ENDIAN)
	return Swap(i);
#else
	return i;
#endif
}

inline RADRT_API U32 RADRT_CALL SwapLittle(U32 i)
{
#if defined(RAD_OPT_BIG_ENDIAN)
	return Swap(i);
#else
	return i;
#endif
}

inline RADRT_API S64 RADRT_CALL SwapLittle(S64 i)
{
#if defined(RAD_OPT_BIG_ENDIAN)
	return Swap(i);
#else
	return i;
#endif
}

inline RADRT_API U64 RADRT_CALL SwapLittle(U64 i)
{
#if defined(RAD_OPT_BIG_ENDIAN)
	return Swap(i);
#else
	return i;
#endif
}

inline RADRT_API F32 RADRT_CALL SwapLittle(F32 i)
{
#if defined(RAD_OPT_BIG_ENDIAN)
	return Swap(i);
#else
	return i;
#endif
}

inline RADRT_API F64 RADRT_CALL SwapLittle(F64 i)
{
#if defined(RAD_OPT_BIG_ENDIAN)
	return Swap(i);
#else
	return i;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////
// endian:: SwapLittleArray()
//////////////////////////////////////////////////////////////////////////////////////////

inline RADRT_API void RADRT_CALL SwapLittleArray(S16* pData, UReg nNum, UReg nStride)
{
#if defined(RAD_OPT_BIG_ENDIAN)
	SwapArray(pData, nNum, nStride);
#endif
}

inline RADRT_API void RADRT_CALL SwapLittleArray(U16* pData, UReg nNum, UReg nStride)
{
#if defined(RAD_OPT_BIG_ENDIAN)
	SwapArray(pData, nNum, nStride);
#endif
}

inline RADRT_API void RADRT_CALL SwapLittleArray(S32* pData, UReg nNum, UReg nStride)
{
#if defined(RAD_OPT_BIG_ENDIAN)
	SwapArray(pData, nNum, nStride);
#endif
}

inline RADRT_API void RADRT_CALL SwapLittleArray(U32* pData, UReg nNum, UReg nStride)
{
#if defined(RAD_OPT_BIG_ENDIAN)
	SwapArray(pData, nNum, nStride);
#endif
}

inline RADRT_API void RADRT_CALL SwapLittleArray(S64* pData, UReg nNum, UReg nStride)
{
#if defined(RAD_OPT_BIG_ENDIAN)
	SwapArray(pData, nNum, nStride);
#endif
}

inline RADRT_API void RADRT_CALL SwapLittleArray(U64* pData, UReg nNum, UReg nStride)
{
#if defined(RAD_OPT_BIG_ENDIAN)
	SwapArray(pData, nNum, nStride);
#endif
}

inline RADRT_API void RADRT_CALL SwapLittleArray(F32* pData, UReg nNum, UReg nStride)
{
#if defined(RAD_OPT_BIG_ENDIAN)
	SwapArray(pData, nNum, nStride);
#endif
}

inline RADRT_API void RADRT_CALL SwapLittleArray(F64* pData, UReg nNum, UReg nStride)
{
#if defined(RAD_OPT_BIG_ENDIAN)
	SwapArray(pData, nNum, nStride);
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////
// endian::SwapLittleStruct()
//////////////////////////////////////////////////////////////////////////////////////////

inline RADRT_API void RADRT_CALL SwapLittleStruct(void* pStruct, ByteSwapCodes* pSwapCodes, bool bSwapArrays)
{
#if defined(RAD_OPT_BIG_ENDIAN)
	SwapStruct(pStruct, pSwapLittleCodes, true, bSwapLittleArrays);
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////
// endian::SwapLittleStructArray()
//////////////////////////////////////////////////////////////////////////////////////////

inline RADRT_API void RADRT_CALL SwapLittleStructArray(void* pStruct, ByteSwapCodes* pSwapCodes, UReg nStride, UReg nNum, bool bSwapArrays)
{
#if defined(RAD_OPT_BIG_ENDIAN)
	SwapStructArray(pStruct, pSwapCodes, nStride, nNum, true, bSwapArrays);
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////
// endian::SwapBig()
//////////////////////////////////////////////////////////////////////////////////////////

inline RADRT_API S16 RADRT_CALL SwapBig(S16 i)
{
#if defined(RAD_OPT_LITTLE_ENDIAN)
	return Swap(i);
#else
	return i;
#endif
}

inline RADRT_API U16 RADRT_CALL SwapBig(U16 i)
{
#if defined(RAD_OPT_LITTLE_ENDIAN)
	return Swap(i);
#else
	return i;
#endif
}

inline RADRT_API S32 RADRT_CALL SwapBig(S32 i)
{
#if defined(RAD_OPT_LITTLE_ENDIAN)
	return Swap(i);
#else
	return i;
#endif
}

inline RADRT_API U32 RADRT_CALL SwapBig(U32 i)
{
#if defined(RAD_OPT_LITTLE_ENDIAN)
	return Swap(i);
#else
	return i;
#endif
}

inline RADRT_API S64 RADRT_CALL SwapBig(S64 i)
{
#if defined(RAD_OPT_LITTLE_ENDIAN)
	return Swap(i);
#else
	return i;
#endif
}

inline RADRT_API U64 RADRT_CALL SwapBig(U64 i)
{
#if defined(RAD_OPT_LITTLE_ENDIAN)
	return Swap(i);
#else
	return i;
#endif
}

inline RADRT_API F32 RADRT_CALL SwapBig(F32 i)
{
#if defined(RAD_OPT_LITTLE_ENDIAN)
	return Swap(i);
#else
	return i;
#endif
}

inline RADRT_API F64 RADRT_CALL SwapBig(F64 i)
{
#if defined(RAD_OPT_LITTLE_ENDIAN)
	return Swap(i);
#else
	return i;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////
// endian:: SwapBigArray()
//////////////////////////////////////////////////////////////////////////////////////////

inline RADRT_API void RADRT_CALL SwapBigArray(S16* pData, UReg nNum, UReg nStride)
{
#if defined(RAD_OPT_LITTLE_ENDIAN)
	SwapArray(pData, nNum, nStride);
#endif
}

inline RADRT_API void RADRT_CALL SwapBigArray(U16* pData, UReg nNum, UReg nStride)
{
#if defined(RAD_OPT_LITTLE_ENDIAN)
	SwapArray(pData, nNum, nStride);
#endif
}

inline RADRT_API void RADRT_CALL SwapBigArray(S32* pData, UReg nNum, UReg nStride)
{
#if defined(RAD_OPT_LITTLE_ENDIAN)
	SwapArray(pData, nNum, nStride);
#endif
}

inline RADRT_API void RADRT_CALL SwapBigArray(U32* pData, UReg nNum, UReg nStride)
{
#if defined(RAD_OPT_LITTLE_ENDIAN)
	SwapArray(pData, nNum, nStride);
#endif
}

inline RADRT_API void RADRT_CALL SwapBigArray(S64* pData, UReg nNum, UReg nStride)
{
#if defined(RAD_OPT_LITTLE_ENDIAN)
	SwapArray(pData, nNum, nStride);
#endif
}

inline RADRT_API void RADRT_CALL SwapBigArray(U64* pData, UReg nNum, UReg nStride)
{
#if defined(RAD_OPT_LITTLE_ENDIAN)
	SwapArray(pData, nNum, nStride);
#endif
}

inline RADRT_API void RADRT_CALL SwapBigArray(F32* pData, UReg nNum, UReg nStride)
{
#if defined(RAD_OPT_LITTLE_ENDIAN)
	SwapArray(pData, nNum, nStride);
#endif
}

inline RADRT_API void RADRT_CALL SwapBigArray(F64* pData, UReg nNum, UReg nStride)
{
#if defined(RAD_OPT_LITTLE_ENDIAN)
	SwapArray(pData, nNum, nStride);
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////
// endian::SwapBigStruct()
//////////////////////////////////////////////////////////////////////////////////////////

inline RADRT_API void RADRT_CALL SwapBigStruct(void* pStruct, ByteSwapCodes* pSwapCodes, bool bSwapArrays)
{
#if defined(RAD_OPT_LITTLE_ENDIAN)
	SwapStruct(pStruct, pSwapCodes, true, bSwapArrays);
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////
// endian::SwapBigStructArray()
//////////////////////////////////////////////////////////////////////////////////////////

inline RADRT_API void RADRT_CALL SwapBigStructArray(void* pStruct, ByteSwapCodes* pSwapCodes, UReg nStride, UReg nNum, bool bSwapArrays)
{
#if defined(RAD_OPT_LITTLE_ENDIAN)
	SwapStructArray(pStruct, pSwapCodes, nStride, nNum, true, bSwapArrays);
#endif
}

} // namespace endian


#include "Backend.h"


namespace endian {

#if !defined(RADENDIAN_OPT_BYTESWAP_INTRINSICS)

inline RADRT_API U16 RADRT_CALL Swap(U16 val)
{
	return ((val>>8)&0xff) | (val<<8);
}

inline RADRT_API U32 RADRT_CALL Swap(U32 val)
{
	return ((val>>24)&0xFF) | ((val>>8)&0xFF00) | ((val<<8)&0xFF0000) | (val<<24);
}

// NOTE: the 64 bit Swap is in Endian.cpp

#endif

inline RADRT_API S16 RADRT_CALL Swap(S16 val)
{
	U16 result = Swap(*reinterpret_cast<U16 *>(&val));
	return *reinterpret_cast<S16 *>(&result);
}

inline RADRT_API S32 RADRT_CALL Swap(S32 val)
{
	U32 result = Swap(*reinterpret_cast<U32 *>(&val));
	return *reinterpret_cast<S32 *>(&result);
}

inline RADRT_API S64 RADRT_CALL Swap(S64 val)
{
	U64 result = Swap(*reinterpret_cast<U64 *>(&val));
	return *reinterpret_cast<S64 *>(&result);
}

inline RADRT_API F32 RADRT_CALL Swap(F32 val)
{
	U32 result = Swap(*reinterpret_cast<U32 *>(&val));
	return *reinterpret_cast<F32 *>(&result);
}

inline RADRT_API F64 RADRT_CALL Swap(F64 val)
{
	U64 result = Swap(*reinterpret_cast<U64 *>(&val));
	return *reinterpret_cast<F64 *>(&result);
}

} // endian

