/*! \file SIMD_sse2.cpp
	\copyright Copyright (c) 2010 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup runtime
*/

// timings taken for i7 from http://www.agner.org/optimize/instruction_tables.pdf

#include "SIMD.h"

#if defined(RAD_OPT_WINX)

#include "../StringBase.h"

namespace {

void SkinVerts4(
	float *outVerts, 
	const float *bones, 
	const float *vertices,
	const U16 *boneIndices,
	int numVerts
)
{
	RAD_ASSERT(IsAligned(outVerts, SIMDDriver::Alignment));
	RAD_ASSERT(IsAligned(bones, SIMDDriver::Alignment));
	RAD_ASSERT(IsAligned(vertices, SIMDDriver::Alignment));
	RAD_ASSERT(IsAligned(boneIndices, SIMDDriver::Alignment));

	for (int i = 0; i < numVerts; ++i)
	{

#if defined(RAD_OPT_WINX)
		const float *bone0 = bones + boneIndices[0]*SIMDDriver::NumBoneFloats;
		const float *bone1 = bones + boneIndices[1]*SIMDDriver::NumBoneFloats;
		const float *bone2 = bones + boneIndices[2]*SIMDDriver::NumBoneFloats;
		const float *bone3 = bones + boneIndices[3]*SIMDDriver::NumBoneFloats;
		__asm {
			mov      esi, vertices   // 1 cycles (issued same cycle)
			mov      edi, outVerts   // 1 cycles (issued same cycle)
			mov      edx, bone0      // 1 cycles (issued same cycle)

			movaps   xmm4, [esi] // xyzw 2 cycles (1 issue)
			movaps   xmm5, [esi] // xyzw 2 cycles (1 issue)
			movaps   xmm6, [esi] // xyzw 2 cycles (1 issue)
			movaps   xmm7, [esi] // xyzw 2 cycles (1 issue)
			movaps   xmm0, [edx] // 2 cycles (1 issue)
			movaps   xmm1, [edx+0x10] // 2 cycles (1 issue)
			movaps   xmm2, [edx+0x20] // 2 cycles (1 issue)
			movaps   xmm3, [edx+0x30] // 2 cycles (1 issue)

			unpcklps xmm4, xmm4 // xxyy (xmm4 ready) 1 cycle (1 issue)
			unpcklps xmm5, xmm5 // xxyy (xmm5 ready) 1 cycle (1 issue)
			unpckhps xmm7, xmm7 // zzww (xmm7 ready) 1 cycle (1 issue)
			unpckhps xmm6, xmm6 // zzww (xmm6 ready) 1 cycle (1 issue)
			
			movhlps  xmm7, xmm7 // wwww 1 cycle (1 issue)
			movlhps  xmm4, xmm4 // xxxx 1 cycle (1 issue)
			movhlps  xmm5, xmm5 // yyyy 1 cycle (1 issue)
			movlhps  xmm6, xmm6 // zzzz 1 cycle (1 issue)
			
			mulps    xmm3, xmm7 // translation * weight 4 cycles (1 issue)
			mulps    xmm4, xmm0 // xxxx * row0 4 cycles (1 issue)
			mulps    xmm5, xmm1 // yyyy * row1 4 cycles (1 issue)
			mulps    xmm6, xmm2 // zzzz * row2 4 cycles (1 issue)
			
			mov      edx, bone1      // 1 cycles (issued same cycle)

			addps    xmm3, xmm4 // 3 cycles (1 issue)
			movaps   xmm0, [edx] // 2 cycles (1 issue)

			addps    xmm5, xmm6 // 3 cycles (1 issue)
			
			movaps   xmm1, [edx+0x10] // 2 cycles (1 issue)
			movaps   xmm2, [edx+0x20] // 2 cycles (1 issue)
			movaps   xmm7, [edx+0x30] // 2 cycles (1 issue)
		
			addps    xmm3, xmm5 // 3 cycles (1 issue)

			// xmm3 contains result of previous bone

			movaps   xmm4, [esi+0x10] // xyzw 2 cycles (1 issue)
			movaps   xmm5, [esi+0x10] // xyzw 2 cycles (1 issue)
			movaps   xmm6, [esi+0x10] // xyzw 2 cycles (1 issue)

			unpcklps xmm4, xmm4 // xxyy (xmm4 ready) 1 cycle (1 issue)
			unpcklps xmm5, xmm5 // xxyy (xmm5 ready) 1 cycle (1 issue)
			unpckhps xmm6, xmm6 // zzww (xmm6 ready) 1 cycle (1 issue)
			
			movlhps  xmm4, xmm4 // xxxx 1 cycle (1 issue)
			movhlps  xmm5, xmm5 // yyyy 1 cycle (1 issue)
			movlhps  xmm6, xmm6 // zzzz 1 cycle (1 issue)

			mulps    xmm4, xmm0 // xxxx * row0 4 cycles (1 issue)
			movaps   xmm0, [esi+0x10] // xyzw 2 cycles (1 issue)
			mulps    xmm5, xmm1 // yyyy * row1 4 cycles (1 issue)
			mulps    xmm6, xmm2 // zzzz * row2 4 cycles (1 issue)
			unpckhps xmm0, xmm0 // zzww (xmm0 ready) 1 cycle (1 issue)
			movhlps  xmm0, xmm0 // wwww 1 cycle (1 issue)

			addps    xmm5, xmm4 // 3 cycles (1 issue)
			mov      edx, bone2 // 1 cycles (issued same cycle)
			mulps    xmm7, xmm0 // translation * weight 4 cycles (1 issue)
			addps    xmm5, xmm6 // 3 cycles (1 issue)

			movaps   xmm0, [edx] // 2 cycles (1 issue)
			movaps   xmm1, [edx+0x10] // 2 cycles (1 issue)

			addps    xmm7, xmm5 // 3 cycles (1 issue)

			movaps   xmm2, [edx+0x20] // 2 cycles (1 issue)
			movaps   xmm6, [edx+0x30] // 2 cycles (1 issue)

			addps    xmm3, xmm7

			movaps   xmm4, [esi+0x20] // xyzw 2 cycles (1 issue)
			movaps   xmm5, [esi+0x20] // xyzw 2 cycles (1 issue)
			movaps   xmm7, [esi+0x20] // xyzw 2 cycles (1 issue)

			unpcklps xmm4, xmm4 // xxyy (xmm4 ready) 1 cycle (1 issue)
			unpcklps xmm5, xmm5 // xxyy (xmm5 ready) 1 cycle (1 issue)
			unpckhps xmm7, xmm7 // zzww (xmm6 ready) 1 cycle (1 issue)
			
			movlhps  xmm4, xmm4 // xxxx 1 cycle (1 issue)
			movhlps  xmm5, xmm5 // yyyy 1 cycle (1 issue)
			movlhps  xmm7, xmm7 // zzzz 1 cycle (1 issue)

			mulps    xmm4, xmm0 // xxxx * row0 4 cycles (1 issue)
			movaps   xmm0, [esi+0x20] // xyzw 2 cycles (1 issue)
			mulps    xmm5, xmm1 // yyyy * row1 4 cycles (1 issue)
			mulps    xmm7, xmm2 // zzzz * row2 4 cycles (1 issue)
			unpckhps xmm0, xmm0 // zzww (xmm0 ready) 1 cycle (1 issue)
			movhlps  xmm0, xmm0 // wwww 1 cycle (1 issue)
			
			addps    xmm5, xmm4 // 3 cycles (1 issue)
			mulps    xmm6, xmm0 // translation * weight 4 cycles (1 issue)
				
			mov      edx, bone3 // 1 cycles (issued same cycle)
			addps    xmm5, xmm7 // 3 cycles (1 issue)
			movaps   xmm0, [edx] // 2 cycles (1 issue)
			movaps   xmm1, [edx+0x10] // 2 cycles (1 issue)
			
			addps    xmm5, xmm6 // 3 cycles (1 issue)

			movaps   xmm2, [edx+0x20] // 2 cycles (1 issue)
			movaps   xmm7, [edx+0x30] // 2 cycles (1 issue)

			addps    xmm3, xmm5 // 3 cycles (1 issue)

			movaps   xmm4, [esi+0x30] // xyzw 2 cycles (1 issue)
			movaps   xmm5, [esi+0x30] // xyzw 2 cycles (1 issue)
			movaps   xmm6, [esi+0x30] // xyzw 2 cycles (1 issue)

			unpcklps xmm4, xmm4 // xxyy (xmm4 ready) 1 cycle (1 issue)
			unpcklps xmm5, xmm5 // xxyy (xmm5 ready) 1 cycle (1 issue)
			unpckhps xmm6, xmm6 // zzww (xmm6 ready) 1 cycle (1 issue)
			
			movlhps  xmm4, xmm4 // xxxx 1 cycle (1 issue)
			movhlps  xmm5, xmm5 // yyyy 1 cycle (1 issue)
			movlhps  xmm6, xmm6 // zzzz 1 cycle (1 issue)
			
			mulps    xmm4, xmm0 // xxxx * row0 4 cycles (1 issue)
			movaps   xmm0, [esi+0x30] // xyzw 2 cycles (1 issue)
			mulps    xmm5, xmm1 // yyyy * row1 4 cycles (1 issue)
			mulps    xmm6, xmm2 // zzzz * row2 4 cycles (1 issue)
			unpckhps xmm0, xmm0 // zzww (xmm0 ready) 1 cycle (1 issue)
			movhlps  xmm0, xmm0 // wwww 1 cycle (1 issue)

			addps    xmm5, xmm4 // 3 cycles (1 issue)
			mulps    xmm7, xmm0 // translation * weight 4 cycles (1 issue)

			// 2 cycle stall waiting for xmm5 
			addps    xmm5, xmm6 // 3 cycles (1 issue)

			// 3 cycle stall waiting for xmm5
			addps    xmm7, xmm5 // 3 cycles (1 issue)

			// 3 cycle stall waiting for xmm7
			addps    xmm3, xmm7
			
			// 3 cycle stall waiting for xmm3
			movaps  [edi], xmm3
			
		}
#else
#error "implement me!"
#endif

		vertices += 16;
		boneIndices += 4;
		outVerts += 4;
	}
}

void SkinVerts3(
	float *outVerts, 
	const float *bones, 
	const float *vertices,
	const U16 *boneIndices,
	int numVerts
)
{
	RAD_ASSERT(IsAligned(outVerts, SIMDDriver::Alignment));
	RAD_ASSERT(IsAligned(bones, SIMDDriver::Alignment));
	RAD_ASSERT(IsAligned(vertices, SIMDDriver::Alignment));
	RAD_ASSERT(IsAligned(boneIndices, SIMDDriver::Alignment));

	for (int i = 0; i < numVerts; ++i)
	{

#if defined(RAD_OPT_WINX)
		const float *bone0 = bones + boneIndices[0]*SIMDDriver::NumBoneFloats;
		const float *bone1 = bones + boneIndices[1]*SIMDDriver::NumBoneFloats;
		const float *bone2 = bones + boneIndices[2]*SIMDDriver::NumBoneFloats;
		__asm {
			mov      esi, vertices   // 1 cycles (issued same cycle)
			mov      edi, outVerts   // 1 cycles (issued same cycle)
			mov      edx, bone0      // 1 cycles (issued same cycle)

			movaps   xmm4, [esi] // xyzw 2 cycles (1 issue)
			movaps   xmm5, [esi] // xyzw 2 cycles (1 issue)
			movaps   xmm6, [esi] // xyzw 2 cycles (1 issue)
			movaps   xmm7, [esi] // xyzw 2 cycles (1 issue)
			movaps   xmm0, [edx] // 2 cycles (1 issue)
			movaps   xmm1, [edx+0x10] // 2 cycles (1 issue)
			movaps   xmm2, [edx+0x20] // 2 cycles (1 issue)
			movaps   xmm3, [edx+0x30] // 2 cycles (1 issue)

			unpcklps xmm4, xmm4 // xxyy (xmm4 ready) 1 cycle (1 issue)
			unpcklps xmm5, xmm5 // xxyy (xmm5 ready) 1 cycle (1 issue)
			unpckhps xmm7, xmm7 // zzww (xmm7 ready) 1 cycle (1 issue)
			unpckhps xmm6, xmm6 // zzww (xmm6 ready) 1 cycle (1 issue)
			
			movhlps  xmm7, xmm7 // wwww 1 cycle (1 issue)
			movlhps  xmm4, xmm4 // xxxx 1 cycle (1 issue)
			movhlps  xmm5, xmm5 // yyyy 1 cycle (1 issue)
			movlhps  xmm6, xmm6 // zzzz 1 cycle (1 issue)
			
			mulps    xmm3, xmm7 // translation * weight 4 cycles (1 issue)
			mulps    xmm4, xmm0 // xxxx * row0 4 cycles (1 issue)
			mulps    xmm5, xmm1 // yyyy * row1 4 cycles (1 issue)
			mulps    xmm6, xmm2 // zzzz * row2 4 cycles (1 issue)
			
			mov      edx, bone1      // 1 cycles (issued same cycle)

			addps    xmm3, xmm4 // 3 cycles (1 issue)
			movaps   xmm0, [edx] // 2 cycles (1 issue)

			addps    xmm5, xmm6 // 3 cycles (1 issue)
			
			movaps   xmm1, [edx+0x10] // 2 cycles (1 issue)
			movaps   xmm2, [edx+0x20] // 2 cycles (1 issue)
			movaps   xmm7, [edx+0x30] // 2 cycles (1 issue)
		
			addps    xmm3, xmm5 // 3 cycles (1 issue)

			// xmm3 contains result of previous bone

			movaps   xmm4, [esi+0x10] // xyzw 2 cycles (1 issue)
			movaps   xmm5, [esi+0x10] // xyzw 2 cycles (1 issue)
			movaps   xmm6, [esi+0x10] // xyzw 2 cycles (1 issue)

			unpcklps xmm4, xmm4 // xxyy (xmm4 ready) 1 cycle (1 issue)
			unpcklps xmm5, xmm5 // xxyy (xmm5 ready) 1 cycle (1 issue)
			unpckhps xmm6, xmm6 // zzww (xmm6 ready) 1 cycle (1 issue)
			
			movlhps  xmm4, xmm4 // xxxx 1 cycle (1 issue)
			movhlps  xmm5, xmm5 // yyyy 1 cycle (1 issue)
			movlhps  xmm6, xmm6 // zzzz 1 cycle (1 issue)

			mulps    xmm4, xmm0 // xxxx * row0 4 cycles (1 issue)
			movaps   xmm0, [esi+0x10] // xyzw 2 cycles (1 issue)
			mulps    xmm5, xmm1 // yyyy * row1 4 cycles (1 issue)
			mulps    xmm6, xmm2 // zzzz * row2 4 cycles (1 issue)
			unpckhps xmm0, xmm0 // zzww (xmm0 ready) 1 cycle (1 issue)
			movhlps  xmm0, xmm0 // wwww 1 cycle (1 issue)

			addps    xmm5, xmm4 // 3 cycles (1 issue)
			mov      edx, bone2 // 1 cycles (issued same cycle)
			mulps    xmm7, xmm0 // translation * weight 4 cycles (1 issue)
			addps    xmm5, xmm6 // 3 cycles (1 issue)

			movaps   xmm0, [edx] // 2 cycles (1 issue)
			movaps   xmm1, [edx+0x10] // 2 cycles (1 issue)

			addps    xmm7, xmm5 // 3 cycles (1 issue)

			movaps   xmm2, [edx+0x20] // 2 cycles (1 issue)
			movaps   xmm6, [edx+0x30] // 2 cycles (1 issue)

			addps    xmm3, xmm7

			movaps   xmm4, [esi+0x20] // xyzw 2 cycles (1 issue)
			movaps   xmm5, [esi+0x20] // xyzw 2 cycles (1 issue)
			movaps   xmm7, [esi+0x20] // xyzw 2 cycles (1 issue)

			unpcklps xmm4, xmm4 // xxyy (xmm4 ready) 1 cycle (1 issue)
			unpcklps xmm5, xmm5 // xxyy (xmm5 ready) 1 cycle (1 issue)
			unpckhps xmm7, xmm7 // zzww (xmm6 ready) 1 cycle (1 issue)
			
			movlhps  xmm4, xmm4 // xxxx 1 cycle (1 issue)
			movhlps  xmm5, xmm5 // yyyy 1 cycle (1 issue)
			movlhps  xmm7, xmm7 // zzzz 1 cycle (1 issue)

			mulps    xmm4, xmm0 // xxxx * row0 4 cycles (1 issue)
			movaps   xmm0, [esi+0x20] // xyzw 2 cycles (1 issue)
			mulps    xmm5, xmm1 // yyyy * row1 4 cycles (1 issue)
			mulps    xmm7, xmm2 // zzzz * row2 4 cycles (1 issue)
			unpckhps xmm0, xmm0 // zzww (xmm0 ready) 1 cycle (1 issue)
			movhlps  xmm0, xmm0 // wwww 1 cycle (1 issue)
			
			addps    xmm5, xmm4 // 3 cycles (1 issue)
			mulps    xmm6, xmm0 // translation * weight 4 cycles (1 issue)
				
			// 2 cycle stall waiting for xmm5 
			addps    xmm5, xmm7 // 3 cycles (1 issue)

			// 3 cycle stall waiting for xmm5
			addps    xmm5, xmm6 // 3 cycles (1 issue)

			// 3 cycle stall waiting for xmm5
			addps    xmm3, xmm5 // 3 cycles (1 issue)
			
			// 3 cycle stall waiting for xmm3
			movaps  [edi], xmm3
			
		}
#else
#error "implement me!"
#endif

		vertices += 12;
		boneIndices += 3;
		outVerts += 4;
	}
}

void SkinVerts2(
	float *outVerts, 
	const float *bones, 
	const float *vertices,
	const U16 *boneIndices,
	int numVerts
)
{
	RAD_ASSERT(IsAligned(outVerts, SIMDDriver::Alignment));
	RAD_ASSERT(IsAligned(bones, SIMDDriver::Alignment));
	RAD_ASSERT(IsAligned(vertices, SIMDDriver::Alignment));
	RAD_ASSERT(IsAligned(boneIndices, SIMDDriver::Alignment));

	for (int i = 0; i < numVerts; ++i)
	{

#if defined(RAD_OPT_WINX)
		const float *bone0 = bones + boneIndices[0]*SIMDDriver::NumBoneFloats;
		const float *bone1 = bones + boneIndices[1]*SIMDDriver::NumBoneFloats;
		__asm {
			mov      esi, vertices   // 1 cycles (issued same cycle)
			mov      edi, outVerts   // 1 cycles (issued same cycle)
			mov      edx, bone0      // 1 cycles (issued same cycle)

			movaps   xmm4, [esi] // xyzw 2 cycles (1 issue)
			movaps   xmm5, [esi] // xyzw 2 cycles (1 issue)
			movaps   xmm6, [esi] // xyzw 2 cycles (1 issue)
			movaps   xmm7, [esi] // xyzw 2 cycles (1 issue)
			movaps   xmm0, [edx] // 2 cycles (1 issue)
			movaps   xmm1, [edx+0x10] // 2 cycles (1 issue)
			movaps   xmm2, [edx+0x20] // 2 cycles (1 issue)
			movaps   xmm3, [edx+0x30] // 2 cycles (1 issue)

			unpcklps xmm4, xmm4 // xxyy (xmm4 ready) 1 cycle (1 issue)
			unpcklps xmm5, xmm5 // xxyy (xmm5 ready) 1 cycle (1 issue)
			unpckhps xmm7, xmm7 // zzww (xmm7 ready) 1 cycle (1 issue)
			unpckhps xmm6, xmm6 // zzww (xmm6 ready) 1 cycle (1 issue)
			
			movhlps  xmm7, xmm7 // wwww 1 cycle (1 issue)
			movlhps  xmm4, xmm4 // xxxx 1 cycle (1 issue)
			movhlps  xmm5, xmm5 // yyyy 1 cycle (1 issue)
			movlhps  xmm6, xmm6 // zzzz 1 cycle (1 issue)
			
			mulps    xmm3, xmm7 // translation * weight 4 cycles (1 issue)
			mulps    xmm4, xmm0 // xxxx * row0 4 cycles (1 issue)
			mulps    xmm5, xmm1 // yyyy * row1 4 cycles (1 issue)
			mulps    xmm6, xmm2 // zzzz * row2 4 cycles (1 issue)
			
			mov      edx, bone1      // 1 cycles (issued same cycle)

			addps    xmm3, xmm4 // 3 cycles (1 issue)
			movaps   xmm0, [edx] // 2 cycles (1 issue)

			addps    xmm5, xmm6 // 3 cycles (1 issue)
			
			movaps   xmm1, [edx+0x10] // 2 cycles (1 issue)
			movaps   xmm2, [edx+0x20] // 2 cycles (1 issue)
			movaps   xmm7, [edx+0x30] // 2 cycles (1 issue)
		
			addps    xmm3, xmm5 // 3 cycles (1 issue)

			// xmm3 contains result of previous bone

			movaps   xmm4, [esi+0x10] // xyzw 2 cycles (1 issue)
			movaps   xmm5, [esi+0x10] // xyzw 2 cycles (1 issue)
			movaps   xmm6, [esi+0x10] // xyzw 2 cycles (1 issue)

			unpcklps xmm4, xmm4 // xxyy (xmm4 ready) 1 cycle (1 issue)
			unpcklps xmm5, xmm5 // xxyy (xmm5 ready) 1 cycle (1 issue)
			unpckhps xmm6, xmm6 // zzww (xmm6 ready) 1 cycle (1 issue)
			
			movlhps  xmm4, xmm4 // xxxx 1 cycle (1 issue)
			movhlps  xmm5, xmm5 // yyyy 1 cycle (1 issue)
			movlhps  xmm6, xmm6 // zzzz 1 cycle (1 issue)

			mulps    xmm4, xmm0 // xxxx * row0 4 cycles (1 issue)
			movaps   xmm0, [esi+0x10] // xyzw 2 cycles (1 issue)
			mulps    xmm5, xmm1 // yyyy * row1 4 cycles (1 issue)
			mulps    xmm6, xmm2 // zzzz * row2 4 cycles (1 issue)
			unpckhps xmm0, xmm0 // zzww (xmm0 ready) 1 cycle (1 issue)
			movhlps  xmm0, xmm0 // wwww 1 cycle (1 issue)

			addps    xmm5, xmm4 // 3 cycles (1 issue)
			mulps    xmm7, xmm0 // translation * weight 4 cycles (1 issue)

			// 2 cycle stall waiting for xmm5 
			addps    xmm5, xmm6 // 3 cycles (1 issue)

			// 3 cycle stall waiting for xmm5
			addps    xmm7, xmm5 // 3 cycles (1 issue)

			// 3 cycle stall waiting for xmm7
			addps    xmm3, xmm7
			
			// 3 cycle stall waiting for xmm3
			movaps  [edi], xmm3
		}
#else
#error "implement me!"
#endif
		
		vertices += 8;
		boneIndices += 2;
		outVerts += 4;
	}
}

void SkinVerts1(
	float *outVerts, 
	const float *bones, 
	const float *vertices,
	const U16 *boneIndices,
	int numVerts
)
{
	RAD_ASSERT(IsAligned(outVerts, SIMDDriver::Alignment));
	RAD_ASSERT(IsAligned(bones, SIMDDriver::Alignment));
	RAD_ASSERT(IsAligned(vertices, SIMDDriver::Alignment));
	RAD_ASSERT(IsAligned(boneIndices, SIMDDriver::Alignment));

	for (int i = 0; i < numVerts; ++i)
	{
		const float *bone0 = bones + boneIndices[0]*SIMDDriver::NumBoneFloats;
#if defined(RAD_OPT_WINX)
		__asm {
			mov      esi, vertices   // 1 cycles (issued same cycle)
			mov      edi, outVerts   // 1 cycles (issued same cycle)
			mov      edx, bone0      // 1 cycles (issued same cycle)

			movaps   xmm4, [esi] // xyzw 2 cycles (1 issue)
			movaps   xmm5, [esi] // xyzw 2 cycles (1 issue)
			movaps   xmm6, [esi] // xyzw 2 cycles (1 issue)
			movaps   xmm0, [edx] // 2 cycles (1 issue)
			movaps   xmm1, [edx+0x10] // 2 cycles (1 issue)
			movaps   xmm2, [edx+0x20] // 2 cycles (1 issue)
			movaps   xmm3, [edx+0x30] // 2 cycles (1 issue)

			unpcklps xmm4, xmm4 // xxyy (xmm4 ready) 1 cycle (1 issue)
			unpcklps xmm5, xmm5 // xxyy (xmm5 ready) 1 cycle (1 issue)
			unpckhps xmm6, xmm6 // zzww (xmm6 ready) 1 cycle (1 issue)
			
			movlhps  xmm4, xmm4 // xxxx 1 cycle (1 issue)
			movhlps  xmm5, xmm5 // yyyy 1 cycle (1 issue)
			movlhps  xmm6, xmm6 // zzzz 1 cycle (1 issue)

			mulps    xmm4, xmm0 // xxxx * row0 4 cycles (1 issue)
			mulps    xmm5, xmm1 // yyyy * row1 4 cycles (1 issue)
			mulps    xmm6, xmm2 // zzzz * row2 4 cycles (1 issue)
		
			// 1 cycle stall waiting for xmm4
			addps    xmm3, xmm4 // 3 cycles (1 issue)
			
			// 2 cycle stall waiting for xmm6 
			addps    xmm5, xmm6 // 3 cycles (1 issue)

			// 3 cycle stall waiting for xmm5
			addps    xmm3, xmm5 // 3 cycles (1 issue)
			
			// 3 cycle stall waiting for xmm3
			movaps  [edi], xmm3
		}
#else
#error "implement me!"
#endif

		vertices += 4;
		++boneIndices;
		outVerts += 4;
	}
}

}

const SIMDDriver *SIMD_sse2_bind()
{
	static SIMDDriver d;

	d.SkinVerts[3] = &SkinVerts4;
	d.SkinVerts[2] = &SkinVerts3;
	d.SkinVerts[1] = &SkinVerts2;
	d.SkinVerts[0] = &SkinVerts1;

	string::cpy(d.name, "SIMD_sse2");
	return &d;
}

#endif


