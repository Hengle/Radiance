// Endian.cpp
// Endian Conversion.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "Endian.h"


namespace endian {

//////////////////////////////////////////////////////////////////////////////////////////
//
// Endian Routines
//
//////////////////////////////////////////////////////////////////////////////////////////

#if 0
//////////////////////////////////////////////////////////////////////////////////////////
// Endian Test Code
//////////////////////////////////////////////////////////////////////////////////////////

struct test_struct0
{
	U8 crap0;
	U8 crap1;
	U16 crap2;
	S16 crap3;
	U32 crap4;
	S32 crap5;
	F32 crap6;
	F64 crap7;
};

RAD_BYTE_SWAP_BEGIN_CODES(test_struct0, test_struct0_bsc)
	RAD_BYTE_SWAP_FIELD(test_struct0, SwapU16, crap2),
	RAD_BYTE_SWAP_FIELD(test_struct0, SwapS16, crap3),
	RAD_BYTE_SWAP_FIELD(test_struct0, SwapU32, crap4),
	RAD_BYTE_SWAP_FIELD(test_struct0, SwapS32, crap5),
	RAD_BYTE_SWAP_FIELD(test_struct0, SwapF32, crap6),
	RAD_BYTE_SWAP_FIELD(test_struct0, SwapF64, crap7)
RAD_BYTE_SwapEnd_CODES

#define ARRAY_COUNT		6
struct test_struct1
{
	U16 crap0;
	S16 crap1;
	U32 crap2;
	S32 crap3;

	test_struct0 struct0;
	test_struct0 struct1[ARRAY_COUNT];

	U16 count0;
	test_struct0* struct2;

	U16 count1;
	test_struct0* struct3;

	U32 array0[ARRAY_COUNT];

	U32 count3;
	U16 array1[ARRAY_COUNT];

};

RAD_BYTE_SWAP_BEGIN_CODES(test_struct1, test_struct1_bsc)
	RAD_BYTE_SWAP_FIELD(test_struct1, SwapU16, crap0),
	RAD_BYTE_SWAP_FIELD(test_struct1, SwapS16, crap1),
	RAD_BYTE_SWAP_FIELD(test_struct1, SwapU32, crap2),
	RAD_BYTE_SWAP_FIELD(test_struct1, SwapS32, crap3),
	RAD_BYTE_SWAP_STRUCT(test_struct1, struct0, test_struct0_bsc),
	RAD_BYTE_SWAP_FIXED_STRUCT_ARRAY(test_struct1, ARRAY_COUNT, test_struct0, test_struct0_bsc, struct1),
	RAD_BYTE_SWAP_VAR_STRUCT_ARRAY(test_struct1, SwapU16, count0, test_struct0, test_struct0_bsc, struct2),
	RAD_BYTE_SWAP_FIXED_FIELD_ARRAY(test_struct1, ARRAY_COUNT, SwapU32, array0),
	RAD_BYTE_SWAP_VAR_FIELD_ARRAY(test_struct1, SwapU32, count3, SwapU16, array1)
RAD_BYTE_SwapEnd_CODES

#endif

// The auto-swap code remains basically untested, and unused.
//#pragma message ("NOTE: this byte swapping code needs to actually be tested.")

//void Test()
//{
//	test_struct1 st;
//	SwapStruct(&st, test_struct1_bsc);
//}

//////////////////////////////////////////////////////////////////////////////////////////
// endian::SwapField()
//////////////////////////////////////////////////////////////////////////////////////////

static void SwapField(void* pField, U32 code)
{
	RAD_ASSERT(pField);
	switch(code & ~SwapFlags)
	{
	case SwapS16: Swap(*((S16*)pField)); break;
	case SwapU16: Swap(*((U16*)pField)); break;
	case SwapS32: Swap(*((S32*)pField)); break;
	case SwapU32: Swap(*((U32*)pField)); break;
	case SwapS64: Swap(*((S64*)pField)); break;
	case SwapU64: Swap(*((U64*)pField)); break;
	case SwapF32: Swap(*((F32*)pField)); break;
	case SwapF64: Swap(*((F64*)pField)); break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// endian::SwapFieldArray()
//////////////////////////////////////////////////////////////////////////////////////////

static void SwapFieldArray(void* pField, U32 code, UReg nNum, UReg nStride)
{
	RAD_ASSERT(pField);
	RAD_ASSERT(nStride);

	switch(code & ~SwapFlags)
	{
	case SwapS16: SwapArray(((S16*)pField), nNum, nStride); break;
	case SwapU16: SwapArray(((U16*)pField), nNum, nStride); break;
	case SwapS32: SwapArray(((S32*)pField), nNum, nStride); break;
	case SwapU32: SwapArray(((U32*)pField), nNum, nStride); break;
	case SwapS64: SwapArray(((S64*)pField), nNum, nStride); break;
	case SwapU64: SwapArray(((U64*)pField), nNum, nStride); break;
	case SwapF32: SwapArray(((F32*)pField), nNum, nStride); break;
	case SwapF64: SwapArray(((F64*)pField), nNum, nStride); break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// endian::GetFieldValue()
//////////////////////////////////////////////////////////////////////////////////////////

static U32 GetFieldValue(void* pField, U32 code)
{
	RAD_ASSERT(pField);
	switch(code & ~SwapFlags)
	{
	case NoSwapU8: return *((U8*)pField);
	case NoSwapS8: return *((U8*)pField);
	case SwapS16: return *((S16*)pField);
	case SwapU16: return *((U16*)pField);
	case SwapS32: return *((S32*)pField);
	case SwapU32: return *((U32*)pField);
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
// endian::GetCodeStride()
//////////////////////////////////////////////////////////////////////////////////////////

static UReg GetCodeStride(U32 code)
{
	switch(code & ~SwapFlags)
	{
	case NoSwapU8:
	case NoSwapS8: return 1;
	case SwapS16:
	case SwapU16: return 2;
	case SwapF32:
	case SwapS32:
	case SwapU32: return 4;
	case SwapF64:
	case SwapS64:
	case SwapU64: return 8;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
// endian::SwapStruct()
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API void RADRT_CALL SwapStruct(void* pStruct, ByteSwapCodes* pSwapCodes, bool storing, bool bSwapArrays)
{
	U8* pStructBase = (U8*)pStruct;
	U8* pField;
	U8* pArray;
	UReg nArrayCount;
	U32 nCode;
	ByteSwapCodes* pCode = pSwapCodes;

	//
	// Find each code, and swap each field accordingly.
	//
	while (pCode->code != SwapEnd)
	{
		pField = pStructBase + pCode->field_ofs;
		pArray = pStructBase + pCode->array_ofs;

		if ((pCode->code&(SwapFieldFlag|SwapStructFlag)) == SwapFieldFlag)
		{
			if ((pCode->code&SwapArrayFlag) && bSwapArrays) // a field array
			{
				if (pCode->code&SwapVarFlag) // a variable field array
				{
					nCode = pCode->field_array_code_or_count;
					if (storing)
					{
						nArrayCount = GetFieldValue(pField, pCode->code);
						SwapField(pField, pCode->code);
					}
					else
					{
						SwapField(pField, pCode->code);
						nArrayCount = GetFieldValue(pField, pCode->code);
					}
				}
				else
				{
					// fixed.
					nArrayCount = pCode->field_array_code_or_count;
					nCode = pCode->code;
				}

				SwapFieldArray(pArray, nCode, nArrayCount, GetCodeStride(nCode));
			}
			else
			{
				//
				// single field swap.
				//
				SwapField(pField, pCode->code);
			}
		}
		else
		{
			RAD_ASSERT(pCode->code & SwapStructFlag);

			if ((pCode->code&SwapArrayFlag) && bSwapArrays) // a struct array
			{
				if (pCode->code&SwapVarFlag) // a variable struct array
				{
					if (storing)
					{
						nArrayCount = GetFieldValue(pField, pCode->code);
						SwapField(pField, pCode->code);
					}
					else
					{
						SwapField(pField, pCode->code);
						nArrayCount = GetFieldValue(pField, pCode->code);
					}
				}
				else
				{
					// fixed.
					nArrayCount = pCode->field_array_code_or_count;
				}

				SwapStructArray(pArray, pCode->struct_swap_codes, pCode->array_stride, nArrayCount, true);
			}
			else
			{
				//
				// single struct swap.
				//
				SwapStruct(pField, pCode->struct_swap_codes, bSwapArrays);
			}
		}

		pCode++; // next field.
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// endian::SwapStructArray()
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API void RADRT_CALL SwapStructArray(void* pStruct, ByteSwapCodes* pSwapCodes, UReg nStride, UReg nNum, bool storing, bool bSwapArrays)
{
	for (UReg i = 0; i < nNum; i++)
	{
		SwapStruct(pStruct, pSwapCodes, storing, bSwapArrays);
		pStruct = (void*)(((U8*)pStruct) + nStride);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// endian::SwapArray()
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API void RADRT_CALL SwapArray(S16* pData, UReg nNum, UReg nStride)
{
	for (UReg i = 0; i < nNum; i++)
	{
		*pData = Swap(*pData);
		pData = (S16*)(((U8*)pData) + nStride);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// endian::SwapArray()
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API void RADRT_CALL SwapArray(U16* pData, UReg nNum, UReg nStride)
{
	for (UReg i = 0; i < nNum; i++)
	{
		*pData = Swap(*pData);
		pData = (U16*)(((U8*)pData) + nStride);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// endian::SwapArray()
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API void RADRT_CALL SwapArray(S32* pData, UReg nNum, UReg nStride)
{
	for (UReg i = 0; i < nNum; i++)
	{
		*pData = Swap(*pData);
		pData = (S32*)(((U8*)pData) + nStride);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// endian::SwapArray()
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API void RADRT_CALL SwapArray(U32* pData, UReg nNum, UReg nStride)
{
	for (UReg i = 0; i < nNum; i++)
	{
		*pData = Swap(*pData);
		pData = (U32*)(((U8*)pData) + nStride);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// endian::SwapArray()
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API void RADRT_CALL SwapArray(S64* pData, UReg nNum, UReg nStride)
{
	for (UReg i = 0; i < nNum; i++)
	{
		*pData = Swap(*pData);
		pData = (S64*)(((U8*)pData) + nStride);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// endian::SwapArray()
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API void RADRT_CALL SwapArray(U64* pData, UReg nNum, UReg nStride)
{
	for (UReg i = 0; i < nNum; i++)
	{
		*pData = Swap(*pData);
		pData = (U64*)(((U8*)pData) + nStride);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// endian::SwapArray()
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API void RADRT_CALL SwapArray(F32* pData, UReg nNum, UReg nStride)
{
	for (UReg i = 0; i < nNum; i++)
	{
		*pData = Swap(*pData);
		pData = (F32*)(((U8*)pData) + nStride);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// endian::SwapArray()
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API void RADRT_CALL SwapArray(F64* pData, UReg nNum, UReg nStride)
{
	for (UReg i = 0; i < nNum; i++)
	{
		*pData = Swap(*pData);
		pData = (F64*)(((U8*)pData) + nStride);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// endian::Swap()
//////////////////////////////////////////////////////////////////////////////////////////

#if !defined(RADENDIAN_OPT_BYTESWAP_INTRINSICS) || defined(RADENDIAN_OPT_NEED_DEFAULT_64SWAP)

RADRT_API U64 RADRT_CALL Swap(U64 val)
{
	U64 Mem;
	U8 *DestPtr;
	DestPtr = (U8 *)&Mem;
	DestPtr[7] = ((const U8 *)&val)[0];
	DestPtr[6] = ((const U8 *)&val)[1];
	DestPtr[5] = ((const U8 *)&val)[2];
	DestPtr[4] = ((const U8 *)&val)[3];
	DestPtr[3] = ((const U8 *)&val)[4];
	DestPtr[2] = ((const U8 *)&val)[5];
	DestPtr[1] = ((const U8 *)&val)[6];
	DestPtr[0] = ((const U8 *)&val)[7];
	return Mem;
}

#endif // !defined(RADENDIAN_OPT_BYTESWAP_INTRINSICS)

} // endian

