// Endian.h
// Endian Conversion.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IntEndian.h"
#include "../PushPack.h"


namespace endian {

//////////////////////////////////////////////////////////////////////////////////////////
//                                                                                     
// Endian Routines                                                                     
//                                                                                     
//////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////
// endian::Swap()                                                              
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API U16 RADRT_CALL Swap(U16);
RADRT_API U32 RADRT_CALL Swap(U32);
RADRT_API U64 RADRT_CALL Swap(U64);
RADRT_API S16 RADRT_CALL Swap(S16);
RADRT_API S32 RADRT_CALL Swap(S32);
RADRT_API S64 RADRT_CALL Swap(S64);
RADRT_API F32 RADRT_CALL Swap(F32);
RADRT_API F64 RADRT_CALL Swap(F64);

//////////////////////////////////////////////////////////////////////////////////////////
// endian:: SwapArray()                                                              
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API void RADRT_CALL SwapArray(S16* pData, UReg nNum, UReg nStride);
RADRT_API void RADRT_CALL SwapArray(U16* pData, UReg nNum, UReg nStride);
RADRT_API void RADRT_CALL SwapArray(S32* pData, UReg nNum, UReg nStride);
RADRT_API void RADRT_CALL SwapArray(U32* pData, UReg nNum, UReg nStride);
RADRT_API void RADRT_CALL SwapArray(S64* pData, UReg nNum, UReg nStride);
RADRT_API void RADRT_CALL SwapArray(U64* pData, UReg nNum, UReg nStride);
RADRT_API void RADRT_CALL SwapArray(F32* pData, UReg nNum, UReg nStride);
RADRT_API void RADRT_CALL SwapArray(F64* pData, UReg nNum, UReg nStride);

//////////////////////////////////////////////////////////////////////////////////////////
// endian::ByteSwapCodes()
//////////////////////////////////////////////////////////////////////////////////////////

struct ByteSwapCodes;

//////////////////////////////////////////////////////////////////////////////////////////
// endian::SwapStruct()
//////////////////////////////////////////////////////////////////////////////////////////
// The storing flag controls how a variably sized array's "size" is interpretted.
// If storing is true, then the count field is assumed to be in machine order, and does
// not need to be swapped before use.
//

RADRT_API void RADRT_CALL SwapStruct(void* pStruct, ByteSwapCodes* pSwapCodes, bool storing, bool bSwapArrays = true);

//////////////////////////////////////////////////////////////////////////////////////////
// endian::SwapStructArray()
//////////////////////////////////////////////////////////////////////////////////////////
// The swapStore flag controls how a variably sized array's "size" is interpretted.
// If swapStore is true, then the count field is assumed to be in machine order, and does
// not need to be swapped before use.
//

RADRT_API void RADRT_CALL SwapStructArray(void* pStruct, ByteSwapCodes* pSwapCodes, UReg nStride, UReg nNum, bool storing, bool bSwapArrays = true);

//////////////////////////////////////////////////////////////////////////////////////////
// endian::SwapLittle()                                                              
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API S16 RADRT_CALL SwapLittle(S16);
RADRT_API U16 RADRT_CALL SwapLittle(U16);
RADRT_API S32 RADRT_CALL SwapLittle(S32);
RADRT_API U32 RADRT_CALL SwapLittle(U32);
RADRT_API S64 RADRT_CALL SwapLittle(S64);
RADRT_API U64 RADRT_CALL SwapLittle(U64);
RADRT_API F32 RADRT_CALL SwapLittle(F32);
RADRT_API F64 RADRT_CALL SwapLittle(F64);

//////////////////////////////////////////////////////////////////////////////////////////
// endian:: SwapLittleArray()                                                              
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API void RADRT_CALL SwapLittleArray(S16* pData, UReg nNum, UReg nStride);
RADRT_API void RADRT_CALL SwapLittleArray(U16* pData, UReg nNum, UReg nStride);
RADRT_API void RADRT_CALL SwapLittleArray(S32* pData, UReg nNum, UReg nStride);
RADRT_API void RADRT_CALL SwapLittleArray(U32* pData, UReg nNum, UReg nStride);
RADRT_API void RADRT_CALL SwapLittleArray(S64* pData, UReg nNum, UReg nStride);
RADRT_API void RADRT_CALL SwapLittleArray(U64* pData, UReg nNum, UReg nStride);
RADRT_API void RADRT_CALL SwapLittleArray(F32* pData, UReg nNum, UReg nStride);
RADRT_API void RADRT_CALL SwapLittleArray(F64* pData, UReg nNum, UReg nStride);

//////////////////////////////////////////////////////////////////////////////////////////
// endian::SwapLittleStruct()
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API void RADRT_CALL SwapLittleStruct(void* pStruct, ByteSwapCodes* pSwapCodes, bool bSwapArrays = true);

//////////////////////////////////////////////////////////////////////////////////////////
// endian::SwapLittleStructArray()
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API void RADRT_CALL SwapLittleStructArray(void* pStruct, ByteSwapCodes* pSwapCodes, UReg nStride, UReg nNum, bool bSwapArrays = true);

//////////////////////////////////////////////////////////////////////////////////////////
// endian::SwapBig()                                                              
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API S16 RADRT_CALL SwapBig(S16);
RADRT_API U16 RADRT_CALL SwapBig(U16);
RADRT_API S32 RADRT_CALL SwapBig(S32);
RADRT_API U32 RADRT_CALL SwapBig(U32);
RADRT_API S64 RADRT_CALL SwapBig(S64);
RADRT_API U64 RADRT_CALL SwapBig(U64);
RADRT_API F32 RADRT_CALL SwapBig(F32);
RADRT_API F64 RADRT_CALL SwapBig(F64);

//////////////////////////////////////////////////////////////////////////////////////////
// endian:: SwapBigArray()                                                              
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API void RADRT_CALL SwapBigArray(S16* pData, UReg nNum, UReg nStride);
RADRT_API void RADRT_CALL SwapBigArray(U16* pData, UReg nNum, UReg nStride);
RADRT_API void RADRT_CALL SwapBigArray(S32* pData, UReg nNum, UReg nStride);
RADRT_API void RADRT_CALL SwapBigArray(U32* pData, UReg nNum, UReg nStride);
RADRT_API void RADRT_CALL SwapBigArray(S64* pData, UReg nNum, UReg nStride);
RADRT_API void RADRT_CALL SwapBigArray(U64* pData, UReg nNum, UReg nStride);
RADRT_API void RADRT_CALL SwapBigArray(F32* pData, UReg nNum, UReg nStride);
RADRT_API void RADRT_CALL SwapBigArray(F64* pData, UReg nNum, UReg nStride);

//////////////////////////////////////////////////////////////////////////////////////////
// endian::SwapBigStruct()
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API void RADRT_CALL SwapBigStruct(void* pStruct, ByteSwapCodes* pSwapCodes, bool bSwapArrays = true);

//////////////////////////////////////////////////////////////////////////////////////////
// endian::SwapBigStructArray()
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API void RADRT_CALL SwapBigStructArray(void* pStruct, ByteSwapCodes* pSwapCodes, UReg nStride, UReg nNum, bool bSwapArrays = true);

//////////////////////////////////////////////////////////////////////////////////////////
// endian::ByteSwapFieldOfs
//////////////////////////////////////////////////////////////////////////////////////////

typedef U16 ByteSwapFieldOfs;

//////////////////////////////////////////////////////////////////////////////////////////
// CODES / ENUMS
//////////////////////////////////////////////////////////////////////////////////////////

enum
{
	NoSwapS8 = 1,
	NoSwapU8,
	SwapS16,
	SwapU16,
	SwapS32,
	SwapU32,
	SwapS64,
	SwapU64,
	SwapF32,
	SwapF64,

	RAD_FLAG_BIT(SwapVarFlag, 11),
	RAD_FLAG_BIT(SwapPtrFlag, 12),
	RAD_FLAG_BIT(SwapStructFlag, 13),
	RAD_FLAG_BIT(SwapFieldFlag, 14),
	RAD_FLAG_BIT(SwapArrayFlag, 15),

	SwapFlags = (SwapVarFlag|SwapPtrFlag|SwapStructFlag|SwapFieldFlag|SwapArrayFlag),

	SwapEnd = MaxU16
};

//////////////////////////////////////////////////////////////////////////////////////////
// endian::ByteSwapCodes
//////////////////////////////////////////////////////////////////////////////////////////

struct ByteSwapCodes
{
	U16 code;
	ByteSwapFieldOfs field_ofs;
	ByteSwapFieldOfs array_ofs;
	U16 array_stride;
	U32 field_array_code_or_count;
	struct ByteSwapCodes* struct_swap_codes;
};

//////////////////////////////////////////////////////////////////////////////////////////
// MACROS
//////////////////////////////////////////////////////////////////////////////////////////

#define RAD_BYTE_SWAP_STATIC_STRUCT_NAME(struct_name)	__private_struct_for_byte_swapping_##struct_name
#define RAD_BYTE_SWAP_BEGIN_CODES(struct_name, code_name)	\
static struct_name RAD_BYTE_SWAP_STATIC_STRUCT_NAME(struct_name);	\
static endian::ByteSwapCodes code_name[] = {	
#define RAD_BYTE_SwapEnd_CODES		};

#define RAD_ENDIAN_GET_FIELD_OFS(_struct_, _field_) ((U16)(((U8*)&(RAD_BYTE_SWAP_STATIC_STRUCT_NAME(_struct_)._field_)) - ((U8*)&(RAD_BYTE_SWAP_STATIC_STRUCT_NAME(_struct_)))))

#define RAD_BYTE_SWAP_FIELD(_struct_, _code_, _field_)                                                                { (endian::_code_)|endian::SwapField, RAD_ENDIAN_GET_FIELD_OFS(_struct_, _field_), 0, 0, 0, 0 }
#define RAD_BYTE_SWAP_STRUCT(_struct_, _field_, _struct_codes_)                                                       { endian::SwapStruct, RAD_ENDIAN_GET_FIELD_OFS(_struct_, _field_), 0, 0, 0, _struct_codes_ }
#define RAD_BYTE_SWAP_VAR_FIELD_ARRAY(_struct_, _code_, _count_field_, _array_code_,  _array_field_ )                 { (endian::_code_)|endian::SwapField|endian::SwapArray|endian::SwapVar, RAD_ENDIAN_GET_FIELD_OFS(_struct_, _count_field_), RAD_ENDIAN_GET_FIELD_OFS(_struct_, _array_field_), 0, endian::_array_code_, 0 }
#define RAD_BYTE_SWAP_FIXED_FIELD_ARRAY(_struct_, _count_, _array_code_, _array_field_ )                              { (endian::_array_code_)|endian::SwapField|endian::SwapArray, 0, RAD_ENDIAN_GET_FIELD_OFS(_struct_, _array_field_), 0, _count_, 0 }
#define RAD_BYTE_SWAP_FIXED_STRUCT_ARRAY(_struct_, _count_, _array_type_, _array_codes_, _array_field_ )              { endian::SwapStruct|endian::SwapArray, 0, RAD_ENDIAN_GET_FIELD_OFS(_struct_, _array_field_), sizeof(_array_type_), _count_, _array_codes_ }
#define RAD_BYTE_SWAP_VAR_STRUCT_ARRAY(_struct_, _code_, _count_field_, _array_type_, _array_codes_, _array_field_ )  { (endian::_code_)|endian::SwapStruct|endian::SwapArray|endian::SwapVar, RAD_ENDIAN_GET_FIELD_OFS(_struct_, _count_field_), RAD_ENDIAN_GET_FIELD_OFS(_struct_, _array_field_), sizeof(_array_type_), 0, _array_codes_ }

} // endian


#include "../PopPack.h"
#include "Endian.inl"
