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

#define MUL_BONE_4x3(_vofs, _bofs) \
	


#define LOAD_MUL_YYYY(_vofs, _bofs) \
	movaps   xmm1, [esi+_vofs] \
	movaps   xmm7, [edx+_bofs+0x10] \
	unpcklps xmm1, xmm1 \
	movlhps  xmm1, xmm1 \
	mulps    xmm1, xmm7

#define LOAD_MUL_ZZZZ(_vofs, _bofs) \
	movaps   xmm2, [esi+_vofs] \
	movaps   xmm7, [edx+_bofs+0x20] \
	unpckhps xmm2, xmm2 \
	movlhps  xmm2, xmm2 \
	mulps    xmm2, xmm7

#define LOAD_MUL_WWWW(_vofs, _bofs) \
	movaps   xmm3, [esi+_vofs] \
	movaps   xmm7, [edx+_bofs+0x30] \
	unpckhps xmm3, xmm3 \
	movhlps  xmm3, xmm3 \
	mulps    xmm3, xmm7

#define LOAD_WWWW(_vofs) \
	movaps   xmm3, [esi+_vofs] \

#define STORE_WWWW \


void SkinVerts1B1T(
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
			mov edi, outVerts
			mov edx, bone0

			movaps  xmm0, [esi] // v xyzw
			movaps  xmm4, [esi+0x10] // n xyzw

			// stall xmm0 (1)

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

}

const SIMDDriver *SIMD_sse2_bind()
{
	static SIMDDriver d;

	d = *SIMD_ref_bind();
	d.SkinVerts[0][0] = &SkinVerts1B1T;

	string::cpy(d.name, "SIMD_sse2");
	return &d;
}

#endif


