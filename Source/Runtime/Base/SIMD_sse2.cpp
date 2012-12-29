/*! \file SIMD_sse2.cpp
	\copyright Copyright (c) 2010 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup runtime
*/

// timings taken for i7 from http://www.agner.org/optimize/instruction_tables.pdf

#include RADPCH
#include "SIMD.h"

const SIMDDriver *SIMD_ref_bind();

#if defined(RAD_OPT_WINX)

#include "../StringBase.h"

namespace {

void SkinVerts1B(
	float *outVerts, 
	const float *bones, 
	const float *vertices,
	const U16 *boneIndices,
	int numVerts
) {
	RAD_ASSERT(IsAligned(outVerts, SIMDDriver::kAlignment));
	RAD_ASSERT(IsAligned(bones, SIMDDriver::kAlignment));
	RAD_ASSERT(IsAligned(vertices, SIMDDriver::kAlignment));
	RAD_ASSERT(IsAligned(boneIndices, SIMDDriver::kAlignment));

	for (int i = 0; i < numVerts; ++i) {
		const float *bone0 = bones + boneIndices[0]*SIMDDriver::kNumBoneFloats;
#if defined(RAD_OPT_WINX)
		__asm {
			// movaps r,m m,r = 3
			// movaps r,r = 1
			// addps = 3
			// mulps = 5
			// unpack = 1
			// movhl/lh = 1

			mov esi, vertices
			
			movaps  xmm0, [esi] // v xyzw
			movaps  xmm4, [esi+0x10] // n xyzw

			mov edi, outVerts
			mov edx, bone0

			movaps   xmm2, xmm0
			unpcklps xmm0, xmm0 // v xxyy
			unpckhps xmm2, xmm2 // v zzww
			movaps   xmm1, xmm0
			movlhps  xmm0, xmm0 // v xxxx
			movhlps  xmm1, xmm1 // v yyyy
			
			mulps    xmm0, [edx] // v xxxx * row0
			movlhps  xmm2, xmm2  // v zzzz
			mulps    xmm1, [edx+0x10] // v yyyy * row1
			mulps    xmm2, [edx+0x20] // v zzzz * row2

			movaps   xmm6, xmm4
			unpcklps xmm4, xmm4 // n xxyy
			unpckhps xmm6, xmm6 // n zzww
			movaps   xmm5, xmm4
			addps    xmm1, xmm0 // v xxxx + yyyy
			movlhps  xmm4, xmm4 // n xxxx
			movhlps  xmm5, xmm5 // n yyyy
			mulps    xmm4, [edx] // n xxxx * row0
			mulps    xmm5, [edx+0x10] // n yyyy * row1
			addps    xmm1, xmm2 // v xxxx + yyyy + zzzz
			movlhps  xmm6, xmm6 // n zzzz
			movaps   xmm0, [esi+0x20] // t xyzw
			mulps    xmm6, [edx+0x20] // n zzzz * row2
			addps    xmm1, [edx+0x30] // v xxxx + yyyy + zzzz + row3
			addps    xmm4, xmm5 // n xxxx + yyyyy

			// t xyzw
			movaps   xmm2, xmm0
			unpcklps xmm0, xmm0 // t xxyy
			movaps  [edi], xmm1 // [store] vertex
			movaps   xmm1, xmm0
			addps    xmm4, xmm6 // n xxxx + yyyy + zzzz
			unpckhps xmm2, xmm2 // t zzww
			movlhps  xmm0, xmm0 // t xxxx
			movhlps  xmm1, xmm1 // t yyyy
			mulps    xmm0, [edx] // t xxxx * row0
			mulps    xmm1, [edx+0x10] // t yyyy * row1
			movaps   xmm7, xmm2 // store off zzww
			movlhps  xmm2, xmm2 // t zzzz
			movhlps  xmm7, xmm7 // t wwww
			mulps    xmm2, [edx+0x20] // t zzzz * row2
			movaps   [edi+0x10], xmm4 // [store] normal
			addps    xmm0, xmm1 // t xxxx + yyyy

			// stall xmm2 (3)
			addps    xmm0, xmm2 // t xxxx + yyyy + zzzz
			// stall xmm0 (1)
			shufps   xmm0, xmm0, 93h // wxyz
			// stall xmm0 (2)
			movss    xmm0, xmm7 // (w)xyz
			// stall xmm0 (1)
			shufps   xmm0, xmm0, 39h // xyzw
			// stall xmm0 (2)
			movaps  [edi+0x20], xmm0 // store tangent
		}
#else
#error "implement me!"
#endif

		vertices += 12;
		outVerts += 12;
		++boneIndices;
	}
}
void SkinVerts2B(
	float *outVerts, 
	const float *bones, 
	const float *vertices,
	const U16 *boneIndices,
	int numVerts
) {
	RAD_ASSERT(IsAligned(outVerts, SIMDDriver::kAlignment));
	RAD_ASSERT(IsAligned(bones, SIMDDriver::kAlignment));
	RAD_ASSERT(IsAligned(vertices, SIMDDriver::kAlignment));
	RAD_ASSERT(IsAligned(boneIndices, SIMDDriver::kAlignment));

	for (int i = 0; i < numVerts; ++i) {
		const float *bone0 = bones + boneIndices[0]*SIMDDriver::kNumBoneFloats;
		const float *bone1 = bones + boneIndices[1]*SIMDDriver::kNumBoneFloats;
#if defined(RAD_OPT_WINX)
		__asm {
			// movaps r,m m,r = 3
			// movaps r,r = 1
			// addps = 3
			// mulps = 5
			// unpack = 1
			// movhl/lh = 1

			mov     esi, vertices
			
			movaps  xmm0, [esi] // v xyzw
			movaps  xmm4, [esi+0x10] // v2 xyzw

			mov     edi, outVerts
			mov     edx, bone0
			
			// v1

			movaps   xmm2, xmm0 // v xyzw
			unpcklps xmm0, xmm0 // v xxyy
			unpckhps xmm2, xmm2 // v zzww
			movaps   xmm1, xmm0 // v xxyy
			movaps   xmm3, xmm2 // v zzww
			movlhps  xmm0, xmm0 // v xxxx
			movhlps  xmm1, xmm1 // v yyyy
			mulps    xmm0, [edx] // v xxxx * row0
			mulps    xmm1, [edx+0x10] // v yyyy * row1
			movlhps  xmm2, xmm2 // v zzzz
			movhlps  xmm3, xmm3 // v wwww
			mulps    xmm2, [edx+0x20] // v zzzz * row2
			mulps    xmm3, [edx+0x30] // v wwww * row2

			// v2
			mov      edx, bone1

			movaps   xmm6, xmm4 // v2 xyzw
			addps    xmm0, xmm1 // v xxxx + yyyy
			unpcklps xmm4, xmm4 // v2 xxyy
			unpckhps xmm6, xmm6 // v2 zzww
			movaps   xmm5, xmm4 // v2 xxyy
			addps    xmm0, xmm2 // v xxxx + yyyy + zzzz
			movlhps  xmm4, xmm4 // v2 xxxx
			movhlps  xmm5, xmm5 // v2 yyyy
			mulps    xmm4, [edx] // v2 xxxx * row0
			mulps    xmm5, [edx+0x10] // v2 yyyy * row1
			addps    xmm3, xmm0 // v xxxx + yyyy + zzzz + wwww
			movaps   xmm7, xmm6 // v2 zzww
			movlhps  xmm6, xmm6 // v2 zzzz
			movhlps  xmm7, xmm7 // v2 wwww
			mulps    xmm6, [edx+0x20] // v2 zzzz * row2
			mulps    xmm7, [edx+0x30] // v2 wwww * row3
			addps    xmm5, xmm4 // v2 xxxx + yyyy

			// n1
			mov      edx, bone0
			movaps   xmm0, [esi+0x20] // n1
			movaps   xmm4, [esi+0x30] // n2
			addps    xmm6, xmm3 // v1 + v2 
			addps    xmm7, xmm5 // v2 + v2

			movaps   xmm2, xmm0 // n1 xyzw
			unpcklps xmm0, xmm0 // n1 xxyy
			unpckhps xmm2, xmm2 // n1 zzww
			addps    xmm7, xmm6 // v1 + v2
			movaps   xmm1, xmm0 // n1 xxyy
			movlhps  xmm0, xmm0 // n1 xxxx
			movhlps  xmm1, xmm1 // n1 yyyy
			mulps    xmm0, [edx] // n1 xxxx * row0
			mulps    xmm1, [edx+0x10] // n1 yyyy * row1
			movlhps  xmm2, xmm2 // n1 zzzz
			movaps   [edi], xmm7 // [store] vertex
			mulps    xmm2, [edx+0x20] // n1 zzzz * row2

			// n2
			mov      edx, bone1
			movaps   xmm6, xmm4 // n2 xyzw
			addps    xmm0, xmm1 // n1 xxxx + yyyy
			unpcklps xmm4, xmm4 // n2 xxyy
			unpckhps xmm6, xmm6 // n2 zzww
			movaps   xmm5, xmm4 // n2 xxyy
			movlhps  xmm4, xmm4 // n2 xxxx
			movhlps  xmm5, xmm5 // n2 yyyy
			mulps    xmm4, [edx] // n2 xxxx * row0
			mulps    xmm5, [edx+0x10] // n2 yyyy * row1
			movlhps  xmm6, xmm6 // n2 zzzz
			addps    xmm2, xmm0 // n1 xxxx + yyyy + zzzz
			mulps    xmm6, [edx+0x20] // n2 zzzz * row2

			// t1
			mov      edx, bone0
			movaps   xmm0, [esi+0x40] // t1 xyzw
			addps    xmm5, xmm4 // n2 xxxx + yyyy
			movaps   xmm4, [esi+0x50] // t2 xyzw
			movaps   xmm3, xmm0 // t1 xyzw
			unpcklps xmm0, xmm0 // t1 xxyy
			addps    xmm6, xmm5 // n2 xxxx + yyyy + zzzz
			unpckhps xmm3, xmm3 // t1 zzww
			movaps   xmm1, xmm0 // t1 xxyy
			movlhps  xmm0, xmm0 // t1 xxxx
			addps    xmm6, xmm2 // n1 + n2
			movhlps  xmm1, xmm1 // t1 yyyy
			mulps    xmm0, [edx] // t1 xxxx * row0
			mulps    xmm1, [edx+0x10] // t1 yyyy * row1
			movlhps  xmm3, xmm3 // t1 zzzz
			movaps   [edi+0x10], xmm6 // [store] normal
			mulps    xmm3, [edx+0x20] // t1 zzzz * row2

			// t2
			mov      edx, bone1
			addps    xmm0, xmm1 // t1 xxxx + yyyy
			movaps   xmm6, xmm4 // t2 xyzw
			unpcklps xmm4, xmm4 // t2 xxyy
			unpckhps xmm6, xmm6 // t2 zzww
			addps    xmm0, xmm3 // t1 xxxx + yyyy + zzzz
			movaps   xmm5, xmm4 // t2 xxyy
			movlhps  xmm4, xmm4 // t2 xxxx
			movhlps  xmm5, xmm5 // t2 yyyy
			mulps    xmm4, [edx] // t2 xxxx * row0
			mulps    xmm5, [edx+0x10] // t2 yyyy * row1
			movaps   xmm7, xmm6
			movlhps  xmm6, xmm6 // t2 zzzz
			movhlps  xmm7, xmm7 // t2 wwww
			mulps    xmm6, [edx+0x20] // t2 zzzz * row2

			// stall xmm5 (1)
			addps xmm4, xmm5
			// stall xmm6 (4)
			addps xmm4, xmm6
			// stall xmm4 (3)
			addps xmm4, xmm0 // t1 + t2
			// stall xmm4 (3)
			shufps xmm4, xmm4, 93h // wxyz
			// stall xmm4 (1)
			movss xmm4, xmm7 // (w)xyz
			// stall xmm4 (1)
			shufps xmm4, xmm4, 39h // xyzw
			// stall xmm4 (1)
			movaps [edi+0x20], xmm4 // [store] tangent
		}
#else
#error "implement me!"
#endif

		vertices += 24;
		outVerts += 12;
		boneIndices += 2;
	}
}

void SkinVerts3B(
	float *outVerts, 
	const float *bones, 
	const float *vertices,
	const U16 *boneIndices,
	int numVerts
) {
	RAD_ASSERT(IsAligned(outVerts, SIMDDriver::kAlignment));
	RAD_ASSERT(IsAligned(bones, SIMDDriver::kAlignment));
	RAD_ASSERT(IsAligned(vertices, SIMDDriver::kAlignment));
	RAD_ASSERT(IsAligned(boneIndices, SIMDDriver::kAlignment));

	for (int i = 0; i < numVerts; ++i) {
		const float *bone0 = bones + boneIndices[0]*SIMDDriver::kNumBoneFloats;
		const float *bone1 = bones + boneIndices[1]*SIMDDriver::kNumBoneFloats;
		const float *bone2 = bones + boneIndices[2]*SIMDDriver::kNumBoneFloats;
#if defined(RAD_OPT_WINX)
		__asm {
			// movaps r,m m,r = 3
			// movaps r,r = 1
			// addps = 3
			// mulps = 5
			// unpack = 1
			// movhl/lh = 1

			mov     esi, vertices
			
			movaps  xmm0, [esi] // v xyzw
			movaps  xmm4, [esi+0x10] // v2 xyzw

			mov     edi, outVerts
			mov     edx, bone0
			
			// v1

			movaps   xmm2, xmm0 // v xyzw
			unpcklps xmm0, xmm0 // v xxyy
			unpckhps xmm2, xmm2 // v zzww
			movaps   xmm1, xmm0 // v xxyy
			movaps   xmm3, xmm2 // v zzww
			movlhps  xmm0, xmm0 // v xxxx
			movhlps  xmm1, xmm1 // v yyyy
			mulps    xmm0, [edx] // v xxxx * row0
			mulps    xmm1, [edx+0x10] // v yyyy * row1
			movlhps  xmm2, xmm2 // v zzzz
			movhlps  xmm3, xmm3 // v wwww
			mulps    xmm2, [edx+0x20] // v zzzz * row2
			mulps    xmm3, [edx+0x30] // v wwww * row2

			// v2
			mov      edx, bone1

			movaps   xmm6, xmm4 // v2 xyzw
			addps    xmm0, xmm1 // v xxxx + yyyy
			unpcklps xmm4, xmm4 // v2 xxyy
			unpckhps xmm6, xmm6 // v2 zzww
			movaps   xmm5, xmm4 // v2 xxyy
			addps    xmm0, xmm2 // v xxxx + yyyy + zzzz
			movlhps  xmm4, xmm4 // v2 xxxx
			movhlps  xmm5, xmm5 // v2 yyyy
			mulps    xmm4, [edx] // v2 xxxx * row0
			mulps    xmm5, [edx+0x10] // v2 yyyy * row1
			addps    xmm3, xmm0 // v xxxx + yyyy + zzzz + wwww
			movaps   xmm7, xmm6 // v2 zzww
			movlhps  xmm6, xmm6 // v2 zzzz
			movhlps  xmm7, xmm7 // v2 wwww
			mulps    xmm6, [edx+0x20] // v2 zzzz * row2
			mulps    xmm7, [edx+0x30] // v2 wwww * row3
			addps    xmm5, xmm4 // v2 xxxx + yyyy

			// v3
			movaps   xmm0, [esi+0x20] // v3 xyzw
			mov      edx, bone2
			addps    xmm6, xmm3 // v1 + v2 
			addps    xmm7, xmm5 // v2 + v2

			movaps   xmm2, xmm0 // v3 xyzw
			unpcklps xmm0, xmm0 // v3 xxyy
			unpckhps xmm2, xmm2 // v3 zzww
			addps    xmm7, xmm6 // v1 + v2
			movaps   xmm1, xmm0 // v3 xxyy
			movlhps  xmm0, xmm0 // v3 xxxx
			movhlps  xmm1, xmm1 // v3 yyyy
			mulps    xmm0, [edx] // v3 xxxx * row0
			mulps    xmm1, [edx+0x10] // v3 yyyy * row1
			movaps   xmm3, xmm2 // v3 zzww
			movlhps  xmm2, xmm2 // v3 zzzz
			movhlps  xmm3, xmm3 // v3 wwww
			mulps    xmm2, [edx+0x20] // v3 zzzz * row2
			mulps    xmm3, [edx+0x30] // v3 wwww * row3
			addps    xmm1, xmm0

			// n1
			mov      edx, bone0
			movaps   xmm0, [esi+0x30] // n1
			movaps   xmm4, [esi+0x40] // n2
			addps    xmm7, xmm1 // v1 + v2 + v3
			addps    xmm3, xmm2 // t3 + t3
			movaps   xmm2, xmm0 // n1 xyzw
			unpcklps xmm0, xmm0 // n1 xxyy
			unpckhps xmm2, xmm2 // n1 zzww
			addps    xmm7, xmm3 // v1 + v2 + v3
			movaps   xmm1, xmm0 // n1 xxyy
			movlhps  xmm0, xmm0 // n1 xxxx
			movhlps  xmm1, xmm1 // n1 yyyy
			mulps    xmm0, [edx] // n1 xxxx * row0
			mulps    xmm1, [edx+0x10] // n1 yyyy * row1
			movlhps  xmm2, xmm2 // n1 zzzz
			movaps   [edi], xmm7 // [store] vertex
			mulps    xmm2, [edx+0x20] // n1 zzzz * row2

			// n2
			mov      edx, bone1
			movaps   xmm6, xmm4 // n2 xyzw
			addps    xmm0, xmm1 // n1 xxxx + yyyy
			unpcklps xmm4, xmm4 // n2 xxyy
			unpckhps xmm6, xmm6 // n2 zzww
			movaps   xmm5, xmm4 // n2 xxyy
			movlhps  xmm4, xmm4 // n2 xxxx
			movhlps  xmm5, xmm5 // n2 yyyy
			mulps    xmm4, [edx] // n2 xxxx * row0
			mulps    xmm5, [edx+0x10] // n2 yyyy * row1
			movlhps  xmm6, xmm6 // n2 zzzz
			addps    xmm2, xmm0 // n1 xxxx + yyyy + zzzz
			mulps    xmm6, [edx+0x20] // n2 zzzz * row2

			// n3
			movaps   xmm0, [esi+0x50] // n3 xyzw
			mov      edx, bone2
			addps    xmm4, xmm2 // n1 + n2 xxxx
			movaps   xmm2, xmm0 // n3 xyzw
			unpcklps xmm0, xmm0 // n3 xxyy
			addps    xmm6, xmm5 // n2 yyyy + zzzzz
			unpckhps xmm2, xmm2 // n3 zzww
			movaps   xmm1, xmm0 // n3 xxyy
			movlhps  xmm0, xmm0 // n3 xxxx
			movhlps  xmm1, xmm1 // n3 yyyy
			mulps    xmm0, [edx] // n3 xxxx * row0
			mulps    xmm1, [edx+0x10] // n3 yyyy * row1
			movlhps  xmm2, xmm2 // n3 zzzz
			addps    xmm6, xmm4 // n1 + n2
			mulps    xmm2, [edx+0x20] // n3 zzzz * row2

			// t1
			mov      edx, bone0
			// stall xmm1 (1)
			addps    xmm1, xmm0 // n3 xxxx + yyyy
			movaps   xmm0, [esi+0x60] // t1 xyzw
			movaps   xmm4, [esi+0x70] // t2 xyzw
			movaps   xmm3, xmm0 // t1 xyzw
			unpcklps xmm0, xmm0 // t1 xxyy
			addps    xmm6, xmm1 // n1 + n2 + n3 xxxx yyyy
			unpckhps xmm3, xmm3 // t1 zzww
			movaps   xmm1, xmm0 // t1 xxyy
			movlhps  xmm0, xmm0 // t1 xxxx
			addps    xmm6, xmm2 // n1 + n2 + n3
			movhlps  xmm1, xmm1 // t1 yyyy
			mulps    xmm0, [edx] // t1 xxxx * row0
			mulps    xmm1, [edx+0x10] // t1 yyyy * row1
			movlhps  xmm3, xmm3 // t1 zzzz
			movaps   [edi+0x10], xmm6 // [store] normal
			mulps    xmm3, [edx+0x20] // t1 zzzz * row2

			// t2
			mov      edx, bone1
			addps    xmm1, xmm0 // t1 xxxx + yyyy
			movaps   xmm6, xmm4 // t2 xyzw
			unpcklps xmm4, xmm4 // t2 xxyy
			unpckhps xmm6, xmm6 // t2 zzww
			addps    xmm3, xmm1 // t1 xxxx + yyyy + zzzz
			movaps   xmm5, xmm4 // t2 xxyy
			movlhps  xmm4, xmm4 // t2 xxxx
			movhlps  xmm5, xmm5 // t2 yyyy
			mulps    xmm4, [edx] // t2 xxxx * row0
			mulps    xmm5, [edx+0x10] // t2 yyyy * row1
			movlhps  xmm6, xmm6 // t2 zzzz
			
			// t3
			movaps   xmm0, [esi+0x80] // t3 xyzw
			mulps    xmm6, [edx+0x20] // t2 zzzz * row2
			mov      edx, bone2
			// stall xmm4 (1)
			addps    xmm4, xmm3 // t1 + t2 xxxx
			movaps   xmm2, xmm0 // t3 xyzw
			unpcklps xmm0, xmm0 // t3 xxyy
			unpckhps xmm2, xmm2 // t3 zzww
			movaps   xmm1, xmm0 // t3 xxyy
			movlhps  xmm0, xmm0 // t3 xxxx
			movhlps  xmm1, xmm1 // t3 yyyy
			mulps    xmm0, [edx] // t3 xxxx * row0
			mulps    xmm1, [edx+0x10] // t3 yyyy * row1
			addps    xmm4, xmm5 // t1 + t2 xxxx yyyy
			movaps   xmm3, xmm2 // t3 zzww
			movlhps  xmm2, xmm2 // t3 zzzz
			movhlps  xmm3, xmm3 // t3 wwww
			mulps    xmm2, [edx+0x20] // t3 zzzz * row2
			addps    xmm0, xmm1 // t3 xxxx + yyyy
			addps    xmm4, xmm6 // t1 + t2

			// stall xmm0 (2)
			addps    xmm0, xmm2 // t3
			// stall xmm4 (2)
			addps    xmm4, xmm0 // t1 + t2 + t3
			// stall xmm4 (3)
			shufps   xmm4, xmm4, 93h // wxyz
			// stall xmm4 (1)
			movss    xmm4, xmm3 // (w)xyz
			// stall xmm4 (1)
			shufps   xmm4, xmm4, 39h // xyzw
			// stall xmm4 (1)
			movaps [edi+0x20], xmm4 // [store] tangent
		}
#else
#error "implement me!"
#endif

		vertices += 36;
		outVerts += 12;
		boneIndices += 3;
	}
}

}

const SIMDDriver *SIMD_sse2_bind()
{
	static SIMDDriver d;

	if (d.name[0])
		return &d;

	d = *SIMD_ref_bind();
	d.SkinVerts[0] = &SkinVerts1B;
	d.SkinVerts[1] = &SkinVerts2B;
	d.SkinVerts[2] = &SkinVerts3B;

	// TODO: 4 bones

	string::cpy(d.name, "SIMD_sse2");
	return &d;
}

#endif


