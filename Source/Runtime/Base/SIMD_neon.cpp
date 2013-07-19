/*! \file SIMD_neon.cpp
	\copyright Copyright (c) 2010 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup runtime
*/

#if defined(__ARM_NEON__)

#include "SIMD.h"
#include "../StringBase.h"
#include <iostream>

const SIMDDriver *SIMD_ref_bind();

#define LOAD_BONE_OFS \
	"mov r10, %[bi]				\n\t"

#define STORE_BONE_OFS \
	"mov %[bi], r10				\n\t"

#define LOAD_BONE_PTR \
	"ldrh r9, [r10]				\n\t" \
	"add  r10, r10, #2			\n\t" \
	"lsls r9, r9, #6            \n\t" \
	"add  r9, r9, %[bones]		\n\t"

namespace {

void SkinVerts4(
	float *outVerts, 
	const float *bones, 
	const float *vertices,
	const U16 *boneIndices,
	int numVerts
)
{
	RAD_ASSERT(IsAligned(outVerts, SIMDDriver::kAlignment));
	RAD_ASSERT(IsAligned(bones, SIMDDriver::kAlignment));
	RAD_ASSERT(IsAligned(vertices, SIMDDriver::kAlignment));
	RAD_ASSERT(IsAligned(boneIndices, SIMDDriver::kAlignment));

	for (int i = 0; i < numVerts; ++i)
	{		
		asm volatile (
			LOAD_BONE_PTR
			// q0 =  b0 vertex
			"vld1.32 {q0}, [%[v], :128]!      \n\t"
			// q1-q4 = b0
			"vld1.32 {q1-q2}, [r9, :128]!     \n\t"
			"vld1.32 {q3-q4}, [r9, :128]!     \n\t"
			LOAD_BONE_PTR

			// q1 = b0_col0 * X
			"vmul.f32 q1, q1, d0[0]           \n\t"
			// q5 = b1 vertex (DI)
			"vld1.32 {q5}, [%[v], :128]!     \n\t"

			// q2 = b0_col1 * X
			"vmul.f32 q2, q2, d0[1]			  \n\t"
			// q6-q7 = b1_col0+1 (DI)
			"vld1.32 {q6-q7}, [r9, :128]!     \n\t"
					  
			// q1 = q1 + b0_col2 * Z
			"vmla.f32 q1, q3, d1[0]           \n\t"
			// q8-q9 = b1_col1+2 (DI)
			"vld1.32 {q8-q9}, [r9, :128]!     \n\t"
			LOAD_BONE_PTR

			// q2 = q2 + b0_col3 * W
			"vmla.f32 q2, q4, d1[1]           \n\t"
			// q0 = b2 vertex (DI)
			"vld1.32 {q0}, [%[v], :128]!      \n\t"

			// q1 = q1 + b1_col0 * X
			"vmla.f32 q1, q6, d10[0]          \n\t"
			// q3-q4 = b2_col0+1 (DI)
			"vld1.32 {q3-q4}, [r9, :128]!     \n\t"

			// q2 = q2 + b1_col1 * Y
			"vmla.f32 q2, q7, d10[1]          \n\t"
			// q5-q6 = b2_col2+3 (DI)
			"vld1.32 {q6-q7}, [r9, :128]!     \n\t"
			LOAD_BONE_PTR

			// q1 = q1 + b1_col2 * Z
			"vmla.f32 q1, q8, d11[0]          \n\t"
					  
			// q2 = q2 + b1_col3 * W
			"vmla.f32 q2, q9, d11[1]          \n\t"
			// q5 = b3 vertex (DI)
			"vld1.32 {q5}, [%[v], :128]!      \n\t"

			// q1 = q1 + b2_col0 * X
			"vmla.f32 q1, q3, d0[0]           \n\t"
			// q8-q9 = b3 (DI)
			"vld1.32 {q8-q9}, [r9, :128]!     \n\t"
					  
			// q2 = q2 + b2_col1 * Y
			"vmla.f32 q2, q4, d0[1]           \n\t"
			// q3-q4 = b3 (DI)
			"vld1.32 {q3-q4}, [r9, :128]!     \n\t"

			// q1 = q1 + b2_col2 * Z
			"vmla.f32 q1, q6, d1[0]           \n\t"
			// q2 = q2 + b2_col3 * W
			"vmla.f32 q2, q7, d1[1]           \n\t"
			// q1 = q1 + b3_col0 * X
			"vmla.f32 q1, q8, d10[0]          \n\t"
			// q2 = q2 + b3_col1 * Y
			"vmla.f32 q2, q9, d10[1]          \n\t"
			// q1 = q1 + b3_col2 * Z
			"vmla.f32 q1, q3, d11[0]          \n\t"
			// q2 = q2 + b3_col3 * W
			"vmla.f32 q2, q4, d11[1]          \n\t"

			"vadd.f32 q1, q1, q2              \n\t"
			"vst1.32 {q1}, [%[o], :128]!      \n\t"
		: [o] "+r" (outVerts), [v] "+r" (vertices), [bi] "+r" (boneIndices)
		: [bones] "r" (bones)
		: "cc", "r9", "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "s12", "s13", "s14", "s15", "s16", "s17", "s18", "s19", "s20", "s21", "s22", "s23", "s24", "s25", "s26", "s27", "s28", "s29", "s30", "s31", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d8", "d9", "d10", "d11", "d12", "d13", "d14", "d15", "d16", "d17", "d18", "d19", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9"
		);
	}
}

void SkinVerts3B(
	float *outVerts, 
	const float *bones, 
	const float *vertices,
	const U16 *boneIndices,
	int numVerts
)
{
	RAD_ASSERT(IsAligned(outVerts, SIMDDriver::kAlignment));
	RAD_ASSERT(IsAligned(bones, SIMDDriver::kAlignment));
	RAD_ASSERT(IsAligned(vertices, SIMDDriver::kAlignment));
	RAD_ASSERT(IsAligned(boneIndices, SIMDDriver::kAlignment));
	
	if (numVerts >= 16) {
		
		asm volatile (
		
			LOAD_BONE_OFS
			
			"Lskin3_16:						\n\t"
						
			LOAD_BONE_PTR
			"vld1.32 {q0}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * V1XXXX
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load V2XYZW
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * V1YYYY
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M1_2 * V1ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q15, d1[1]			\n\t" // q1 = M1_TRANSLATE * V1WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M2_0 * V2XXXX
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load V3XYZW
			"vadd.f32 q3, q2, q3				\n\t" // q3 = V1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * V2YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M2_2 * V2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1
			"vmla.f32 q6, q15, d9[1]			\n\t" // q6 = q6 + M2_TRANSLATE * V2WWWW
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N1XYZW
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M3_0 * V3XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vmla.f32 q3, q13, d0[1]			\n\t" // q3 = V1 + M3_1 * V3YYYY
			"vadd.f32 q6, q5, q6				\n\t" // q6 = V2
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M3_2 * V3ZZZZ
			"vmla.f32 q3, q15, d1[1]			\n\t" // q3 = q3 + M3_3 * V3WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M1_0 * N1XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vadd.f32 q1, q1, q6				\n\t" // q1 = V2 + V3
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load N2XYZW
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M1_1 * N1YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M1_2 * N1ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1 + V2 + V3
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q6, q12, d0[0]			\n\t" // q6 = N1 + M2_0 * N2XXXX
			LOAD_BONE_PTR
			"vst1.32 {q3}, [%[o], :128]!		\n\t" // store VXYZW
			"vmla.f32 q5, q13, d0[1]			\n\t" // q5 = N1 + M2_1 * N2YYYY
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N3XYZW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M2_3 * N2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vadd.f32 q6, q5, q6				\n\t" // q6 = N1 + N2
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load T1XYZW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M3_0 * N3XXXX
			// stall q6
			"vadd.f32 q3, q3, q6				\n\t" // q3 = N1 + N2
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M3_1 * N3YYYY
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			// stall q5
			"vadd.f32 q3, q3, q5				\n\t" // q3 = N1 + N2 + N3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vmla.f32 q6, q14, d9[0]			\n\t" // q6 = q6 + M3_3 * N3ZZZZ
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone1 matrix rows 2 + 3
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load T2XYZW
			LOAD_BONE_PTR
			"vadd.f32 q6, q3, q6				\n\t" // q6 = N1 + N2 + N3
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q2, q12, d8[0]			\n\t" // q2 = q2 + M2_0 * T2XXXX
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store NXYZW
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * T2YYYY
			"vld1.32 {q7}, [%[v], :128]!		\n\t" // load T3XYZW
			"vmla.f32 q1, q14, d9[0]			\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			"vadd.f32 q6, q2, q6				\n\t" // q6 = T1 + T2
			LOAD_BONE_PTR
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vmla.f32 q1, q12, d14[0]			\n\t" // q1 = q1 + M3_0 * TXXXX
			"vmla.f32 q6, q13, d14[1]			\n\t" // q6 = q2 + M3_1 * TYYYY
			
			// VERTEX 2
			
			STORE_BONE_OFS
			
			LOAD_BONE_PTR
			"vld1.32 {q0}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d15[0]			\n\t" // q1 = q1 + M3_2 * TZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vadd.f32 q6, q1, q6				\n\t" // q6 = T1 + T2 + T3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * V1XXXX
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load V2XYZW
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * V1YYYY
			"vmov s27, s31						\n\t" // T.WWWW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M1_2 * V1ZZZZ
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store TXYZW
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q15, d1[1]			\n\t" // q1 = M1_TRANSLATE * V1WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M2_0 * V2XXXX
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load V3XYZW
			"vadd.f32 q3, q2, q3				\n\t" // q3 = V1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * V2YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M2_2 * V2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1
			"vmla.f32 q6, q15, d9[1]			\n\t" // q6 = q6 + M2_TRANSLATE * V2WWWW
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N1XYZW
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M3_0 * V3XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vmla.f32 q3, q13, d0[1]			\n\t" // q3 = V1 + M3_1 * V3YYYY
			"vadd.f32 q6, q5, q6				\n\t" // q6 = V2
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M3_2 * V3ZZZZ
			"vmla.f32 q3, q15, d1[1]			\n\t" // q3 = q3 + M3_3 * V3WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M1_0 * N1XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vadd.f32 q1, q1, q6				\n\t" // q1 = V2 + V3
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load N2XYZW
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M1_1 * N1YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M1_2 * N1ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1 + V2 + V3
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q6, q12, d0[0]			\n\t" // q6 = N1 + M2_0 * N2XXXX
			LOAD_BONE_PTR
			"vst1.32 {q3}, [%[o], :128]!		\n\t" // store VXYZW
			"vmla.f32 q5, q13, d0[1]			\n\t" // q5 = N1 + M2_1 * N2YYYY
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N3XYZW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M2_3 * N2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vadd.f32 q6, q5, q6				\n\t" // q6 = N1 + N2
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load T1XYZW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M3_0 * N3XXXX
			// stall q6
			"vadd.f32 q3, q3, q6				\n\t" // q3 = N1 + N2
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M3_1 * N3YYYY
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			// stall q5
			"vadd.f32 q3, q3, q5				\n\t" // q3 = N1 + N2 + N3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vmla.f32 q6, q14, d9[0]			\n\t" // q6 = q6 + M3_3 * N3ZZZZ
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone1 matrix rows 2 + 3
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load T2XYZW
			LOAD_BONE_PTR
			"vadd.f32 q6, q3, q6				\n\t" // q6 = N1 + N2 + N3
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q2, q12, d8[0]			\n\t" // q2 = q2 + M2_0 * T2XXXX
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store NXYZW
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * T2YYYY
			"vld1.32 {q7}, [%[v], :128]!		\n\t" // load T3XYZW
			"vmla.f32 q1, q14, d9[0]			\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			"vadd.f32 q6, q2, q6				\n\t" // q6 = T1 + T2
			LOAD_BONE_PTR
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vmla.f32 q1, q12, d14[0]			\n\t" // q1 = q1 + M3_0 * TXXXX
			"vmla.f32 q6, q13, d14[1]			\n\t" // q6 = q2 + M3_1 * TYYYY
			
			// VERTEX 3
			
			STORE_BONE_OFS
			
			LOAD_BONE_PTR
			"vld1.32 {q0}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d15[0]			\n\t" // q1 = q1 + M3_2 * TZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vadd.f32 q6, q1, q6				\n\t" // q6 = T1 + T2 + T3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * V1XXXX
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load V2XYZW
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * V1YYYY
			"vmov s27, s31						\n\t" // T.WWWW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M1_2 * V1ZZZZ
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store TXYZW
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q15, d1[1]			\n\t" // q1 = M1_TRANSLATE * V1WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M2_0 * V2XXXX
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load V3XYZW
			"vadd.f32 q3, q2, q3				\n\t" // q3 = V1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * V2YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M2_2 * V2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1
			"vmla.f32 q6, q15, d9[1]			\n\t" // q6 = q6 + M2_TRANSLATE * V2WWWW
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N1XYZW
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M3_0 * V3XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vmla.f32 q3, q13, d0[1]			\n\t" // q3 = V1 + M3_1 * V3YYYY
			"vadd.f32 q6, q5, q6				\n\t" // q6 = V2
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M3_2 * V3ZZZZ
			"vmla.f32 q3, q15, d1[1]			\n\t" // q3 = q3 + M3_3 * V3WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M1_0 * N1XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vadd.f32 q1, q1, q6				\n\t" // q1 = V2 + V3
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load N2XYZW
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M1_1 * N1YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M1_2 * N1ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1 + V2 + V3
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q6, q12, d0[0]			\n\t" // q6 = N1 + M2_0 * N2XXXX
			LOAD_BONE_PTR
			"vst1.32 {q3}, [%[o], :128]!		\n\t" // store VXYZW
			"vmla.f32 q5, q13, d0[1]			\n\t" // q5 = N1 + M2_1 * N2YYYY
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N3XYZW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M2_3 * N2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vadd.f32 q6, q5, q6				\n\t" // q6 = N1 + N2
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load T1XYZW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M3_0 * N3XXXX
			// stall q6
			"vadd.f32 q3, q3, q6				\n\t" // q3 = N1 + N2
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M3_1 * N3YYYY
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			// stall q5
			"vadd.f32 q3, q3, q5				\n\t" // q3 = N1 + N2 + N3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vmla.f32 q6, q14, d9[0]			\n\t" // q6 = q6 + M3_3 * N3ZZZZ
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone1 matrix rows 2 + 3
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load T2XYZW
			LOAD_BONE_PTR
			"vadd.f32 q6, q3, q6				\n\t" // q6 = N1 + N2 + N3
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q2, q12, d8[0]			\n\t" // q2 = q2 + M2_0 * T2XXXX
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store NXYZW
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * T2YYYY
			"vld1.32 {q7}, [%[v], :128]!		\n\t" // load T3XYZW
			"vmla.f32 q1, q14, d9[0]			\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			"vadd.f32 q6, q2, q6				\n\t" // q6 = T1 + T2
			LOAD_BONE_PTR
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vmla.f32 q1, q12, d14[0]			\n\t" // q1 = q1 + M3_0 * TXXXX
			"vmla.f32 q6, q13, d14[1]			\n\t" // q6 = q2 + M3_1 * TYYYY
			
			// VERTEX 4
			
			STORE_BONE_OFS
			
			LOAD_BONE_PTR
			"vld1.32 {q0}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d15[0]			\n\t" // q1 = q1 + M3_2 * TZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vadd.f32 q6, q1, q6				\n\t" // q6 = T1 + T2 + T3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * V1XXXX
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load V2XYZW
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * V1YYYY
			"vmov s27, s31						\n\t" // T.WWWW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M1_2 * V1ZZZZ
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store TXYZW
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q15, d1[1]			\n\t" // q1 = M1_TRANSLATE * V1WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M2_0 * V2XXXX
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load V3XYZW
			"vadd.f32 q3, q2, q3				\n\t" // q3 = V1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * V2YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M2_2 * V2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1
			"vmla.f32 q6, q15, d9[1]			\n\t" // q6 = q6 + M2_TRANSLATE * V2WWWW
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N1XYZW
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M3_0 * V3XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vmla.f32 q3, q13, d0[1]			\n\t" // q3 = V1 + M3_1 * V3YYYY
			"vadd.f32 q6, q5, q6				\n\t" // q6 = V2
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M3_2 * V3ZZZZ
			"vmla.f32 q3, q15, d1[1]			\n\t" // q3 = q3 + M3_3 * V3WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M1_0 * N1XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vadd.f32 q1, q1, q6				\n\t" // q1 = V2 + V3
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load N2XYZW
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M1_1 * N1YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M1_2 * N1ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1 + V2 + V3
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q6, q12, d0[0]			\n\t" // q6 = N1 + M2_0 * N2XXXX
			LOAD_BONE_PTR
			"vst1.32 {q3}, [%[o], :128]!		\n\t" // store VXYZW
			"vmla.f32 q5, q13, d0[1]			\n\t" // q5 = N1 + M2_1 * N2YYYY
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N3XYZW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M2_3 * N2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vadd.f32 q6, q5, q6				\n\t" // q6 = N1 + N2
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load T1XYZW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M3_0 * N3XXXX
			// stall q6
			"vadd.f32 q3, q3, q6				\n\t" // q3 = N1 + N2
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M3_1 * N3YYYY
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			// stall q5
			"vadd.f32 q3, q3, q5				\n\t" // q3 = N1 + N2 + N3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vmla.f32 q6, q14, d9[0]			\n\t" // q6 = q6 + M3_3 * N3ZZZZ
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone1 matrix rows 2 + 3
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load T2XYZW
			LOAD_BONE_PTR
			"vadd.f32 q6, q3, q6				\n\t" // q6 = N1 + N2 + N3
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q2, q12, d8[0]			\n\t" // q2 = q2 + M2_0 * T2XXXX
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store NXYZW
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * T2YYYY
			"vld1.32 {q7}, [%[v], :128]!		\n\t" // load T3XYZW
			"vmla.f32 q1, q14, d9[0]			\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			"vadd.f32 q6, q2, q6				\n\t" // q6 = T1 + T2
			LOAD_BONE_PTR
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vmla.f32 q1, q12, d14[0]			\n\t" // q1 = q1 + M3_0 * TXXXX
			"vmla.f32 q6, q13, d14[1]			\n\t" // q6 = q2 + M3_1 * TYYYY
			
			// VERTEX 5
			
			STORE_BONE_OFS
			
			LOAD_BONE_PTR
			"vld1.32 {q0}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d15[0]			\n\t" // q1 = q1 + M3_2 * TZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vadd.f32 q6, q1, q6				\n\t" // q6 = T1 + T2 + T3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * V1XXXX
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load V2XYZW
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * V1YYYY
			"vmov s27, s31						\n\t" // T.WWWW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M1_2 * V1ZZZZ
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store TXYZW
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q15, d1[1]			\n\t" // q1 = M1_TRANSLATE * V1WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M2_0 * V2XXXX
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load V3XYZW
			"vadd.f32 q3, q2, q3				\n\t" // q3 = V1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * V2YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M2_2 * V2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1
			"vmla.f32 q6, q15, d9[1]			\n\t" // q6 = q6 + M2_TRANSLATE * V2WWWW
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N1XYZW
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M3_0 * V3XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vmla.f32 q3, q13, d0[1]			\n\t" // q3 = V1 + M3_1 * V3YYYY
			"vadd.f32 q6, q5, q6				\n\t" // q6 = V2
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M3_2 * V3ZZZZ
			"vmla.f32 q3, q15, d1[1]			\n\t" // q3 = q3 + M3_3 * V3WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M1_0 * N1XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vadd.f32 q1, q1, q6				\n\t" // q1 = V2 + V3
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load N2XYZW
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M1_1 * N1YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M1_2 * N1ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1 + V2 + V3
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q6, q12, d0[0]			\n\t" // q6 = N1 + M2_0 * N2XXXX
			LOAD_BONE_PTR
			"vst1.32 {q3}, [%[o], :128]!		\n\t" // store VXYZW
			"vmla.f32 q5, q13, d0[1]			\n\t" // q5 = N1 + M2_1 * N2YYYY
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N3XYZW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M2_3 * N2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vadd.f32 q6, q5, q6				\n\t" // q6 = N1 + N2
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load T1XYZW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M3_0 * N3XXXX
			// stall q6
			"vadd.f32 q3, q3, q6				\n\t" // q3 = N1 + N2
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M3_1 * N3YYYY
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			// stall q5
			"vadd.f32 q3, q3, q5				\n\t" // q3 = N1 + N2 + N3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vmla.f32 q6, q14, d9[0]			\n\t" // q6 = q6 + M3_3 * N3ZZZZ
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone1 matrix rows 2 + 3
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load T2XYZW
			LOAD_BONE_PTR
			"vadd.f32 q6, q3, q6				\n\t" // q6 = N1 + N2 + N3
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q2, q12, d8[0]			\n\t" // q2 = q2 + M2_0 * T2XXXX
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store NXYZW
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * T2YYYY
			"vld1.32 {q7}, [%[v], :128]!		\n\t" // load T3XYZW
			"vmla.f32 q1, q14, d9[0]			\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			"vadd.f32 q6, q2, q6				\n\t" // q6 = T1 + T2
			LOAD_BONE_PTR
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vmla.f32 q1, q12, d14[0]			\n\t" // q1 = q1 + M3_0 * TXXXX
			"vmla.f32 q6, q13, d14[1]			\n\t" // q6 = q2 + M3_1 * TYYYY
			
			// VERTEX 6
			
			STORE_BONE_OFS
			
			LOAD_BONE_PTR
			"vld1.32 {q0}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d15[0]			\n\t" // q1 = q1 + M3_2 * TZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vadd.f32 q6, q1, q6				\n\t" // q6 = T1 + T2 + T3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * V1XXXX
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load V2XYZW
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * V1YYYY
			"vmov s27, s31						\n\t" // T.WWWW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M1_2 * V1ZZZZ
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store TXYZW
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q15, d1[1]			\n\t" // q1 = M1_TRANSLATE * V1WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M2_0 * V2XXXX
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load V3XYZW
			"vadd.f32 q3, q2, q3				\n\t" // q3 = V1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * V2YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M2_2 * V2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1
			"vmla.f32 q6, q15, d9[1]			\n\t" // q6 = q6 + M2_TRANSLATE * V2WWWW
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N1XYZW
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M3_0 * V3XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vmla.f32 q3, q13, d0[1]			\n\t" // q3 = V1 + M3_1 * V3YYYY
			"vadd.f32 q6, q5, q6				\n\t" // q6 = V2
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M3_2 * V3ZZZZ
			"vmla.f32 q3, q15, d1[1]			\n\t" // q3 = q3 + M3_3 * V3WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M1_0 * N1XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vadd.f32 q1, q1, q6				\n\t" // q1 = V2 + V3
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load N2XYZW
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M1_1 * N1YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M1_2 * N1ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1 + V2 + V3
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q6, q12, d0[0]			\n\t" // q6 = N1 + M2_0 * N2XXXX
			LOAD_BONE_PTR
			"vst1.32 {q3}, [%[o], :128]!		\n\t" // store VXYZW
			"vmla.f32 q5, q13, d0[1]			\n\t" // q5 = N1 + M2_1 * N2YYYY
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N3XYZW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M2_3 * N2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vadd.f32 q6, q5, q6				\n\t" // q6 = N1 + N2
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load T1XYZW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M3_0 * N3XXXX
			// stall q6
			"vadd.f32 q3, q3, q6				\n\t" // q3 = N1 + N2
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M3_1 * N3YYYY
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			// stall q5
			"vadd.f32 q3, q3, q5				\n\t" // q3 = N1 + N2 + N3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vmla.f32 q6, q14, d9[0]			\n\t" // q6 = q6 + M3_3 * N3ZZZZ
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone1 matrix rows 2 + 3
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load T2XYZW
			LOAD_BONE_PTR
			"vadd.f32 q6, q3, q6				\n\t" // q6 = N1 + N2 + N3
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q2, q12, d8[0]			\n\t" // q2 = q2 + M2_0 * T2XXXX
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store NXYZW
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * T2YYYY
			"vld1.32 {q7}, [%[v], :128]!		\n\t" // load T3XYZW
			"vmla.f32 q1, q14, d9[0]			\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			"vadd.f32 q6, q2, q6				\n\t" // q6 = T1 + T2
			LOAD_BONE_PTR
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vmla.f32 q1, q12, d14[0]			\n\t" // q1 = q1 + M3_0 * TXXXX
			"vmla.f32 q6, q13, d14[1]			\n\t" // q6 = q2 + M3_1 * TYYYY
			
			// VERTEX 7
			
			STORE_BONE_OFS
			
			LOAD_BONE_PTR
			"vld1.32 {q0}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d15[0]			\n\t" // q1 = q1 + M3_2 * TZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vadd.f32 q6, q1, q6				\n\t" // q6 = T1 + T2 + T3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * V1XXXX
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load V2XYZW
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * V1YYYY
			"vmov s27, s31						\n\t" // T.WWWW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M1_2 * V1ZZZZ
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store TXYZW
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q15, d1[1]			\n\t" // q1 = M1_TRANSLATE * V1WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M2_0 * V2XXXX
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load V3XYZW
			"vadd.f32 q3, q2, q3				\n\t" // q3 = V1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * V2YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M2_2 * V2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1
			"vmla.f32 q6, q15, d9[1]			\n\t" // q6 = q6 + M2_TRANSLATE * V2WWWW
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N1XYZW
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M3_0 * V3XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vmla.f32 q3, q13, d0[1]			\n\t" // q3 = V1 + M3_1 * V3YYYY
			"vadd.f32 q6, q5, q6				\n\t" // q6 = V2
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M3_2 * V3ZZZZ
			"vmla.f32 q3, q15, d1[1]			\n\t" // q3 = q3 + M3_3 * V3WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M1_0 * N1XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vadd.f32 q1, q1, q6				\n\t" // q1 = V2 + V3
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load N2XYZW
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M1_1 * N1YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M1_2 * N1ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1 + V2 + V3
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q6, q12, d0[0]			\n\t" // q6 = N1 + M2_0 * N2XXXX
			LOAD_BONE_PTR
			"vst1.32 {q3}, [%[o], :128]!		\n\t" // store VXYZW
			"vmla.f32 q5, q13, d0[1]			\n\t" // q5 = N1 + M2_1 * N2YYYY
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N3XYZW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M2_3 * N2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vadd.f32 q6, q5, q6				\n\t" // q6 = N1 + N2
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load T1XYZW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M3_0 * N3XXXX
			// stall q6
			"vadd.f32 q3, q3, q6				\n\t" // q3 = N1 + N2
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M3_1 * N3YYYY
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			// stall q5
			"vadd.f32 q3, q3, q5				\n\t" // q3 = N1 + N2 + N3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vmla.f32 q6, q14, d9[0]			\n\t" // q6 = q6 + M3_3 * N3ZZZZ
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone1 matrix rows 2 + 3
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load T2XYZW
			LOAD_BONE_PTR
			"vadd.f32 q6, q3, q6				\n\t" // q6 = N1 + N2 + N3
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q2, q12, d8[0]			\n\t" // q2 = q2 + M2_0 * T2XXXX
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store NXYZW
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * T2YYYY
			"vld1.32 {q7}, [%[v], :128]!		\n\t" // load T3XYZW
			"vmla.f32 q1, q14, d9[0]			\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			"vadd.f32 q6, q2, q6				\n\t" // q6 = T1 + T2
			LOAD_BONE_PTR
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vmla.f32 q1, q12, d14[0]			\n\t" // q1 = q1 + M3_0 * TXXXX
			"vmla.f32 q6, q13, d14[1]			\n\t" // q6 = q2 + M3_1 * TYYYY
			
			// VERTEX 8
			
			STORE_BONE_OFS
			
			LOAD_BONE_PTR
			"vld1.32 {q0}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d15[0]			\n\t" // q1 = q1 + M3_2 * TZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vadd.f32 q6, q1, q6				\n\t" // q6 = T1 + T2 + T3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * V1XXXX
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load V2XYZW
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * V1YYYY
			"vmov s27, s31						\n\t" // T.WWWW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M1_2 * V1ZZZZ
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store TXYZW
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q15, d1[1]			\n\t" // q1 = M1_TRANSLATE * V1WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M2_0 * V2XXXX
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load V3XYZW
			"vadd.f32 q3, q2, q3				\n\t" // q3 = V1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * V2YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M2_2 * V2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1
			"vmla.f32 q6, q15, d9[1]			\n\t" // q6 = q6 + M2_TRANSLATE * V2WWWW
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N1XYZW
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M3_0 * V3XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vmla.f32 q3, q13, d0[1]			\n\t" // q3 = V1 + M3_1 * V3YYYY
			"vadd.f32 q6, q5, q6				\n\t" // q6 = V2
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M3_2 * V3ZZZZ
			"vmla.f32 q3, q15, d1[1]			\n\t" // q3 = q3 + M3_3 * V3WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M1_0 * N1XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vadd.f32 q1, q1, q6				\n\t" // q1 = V2 + V3
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load N2XYZW
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M1_1 * N1YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M1_2 * N1ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1 + V2 + V3
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q6, q12, d0[0]			\n\t" // q6 = N1 + M2_0 * N2XXXX
			LOAD_BONE_PTR
			"vst1.32 {q3}, [%[o], :128]!		\n\t" // store VXYZW
			"vmla.f32 q5, q13, d0[1]			\n\t" // q5 = N1 + M2_1 * N2YYYY
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N3XYZW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M2_3 * N2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vadd.f32 q6, q5, q6				\n\t" // q6 = N1 + N2
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load T1XYZW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M3_0 * N3XXXX
			// stall q6
			"vadd.f32 q3, q3, q6				\n\t" // q3 = N1 + N2
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M3_1 * N3YYYY
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			// stall q5
			"vadd.f32 q3, q3, q5				\n\t" // q3 = N1 + N2 + N3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vmla.f32 q6, q14, d9[0]			\n\t" // q6 = q6 + M3_3 * N3ZZZZ
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone1 matrix rows 2 + 3
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load T2XYZW
			LOAD_BONE_PTR
			"vadd.f32 q6, q3, q6				\n\t" // q6 = N1 + N2 + N3
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q2, q12, d8[0]			\n\t" // q2 = q2 + M2_0 * T2XXXX
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store NXYZW
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * T2YYYY
			"vld1.32 {q7}, [%[v], :128]!		\n\t" // load T3XYZW
			"vmla.f32 q1, q14, d9[0]			\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			"vadd.f32 q6, q2, q6				\n\t" // q6 = T1 + T2
			LOAD_BONE_PTR
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vmla.f32 q1, q12, d14[0]			\n\t" // q1 = q1 + M3_0 * TXXXX
			"vmla.f32 q6, q13, d14[1]			\n\t" // q6 = q2 + M3_1 * TYYYY
			
			// VERTEX 9
			
			STORE_BONE_OFS
			
			LOAD_BONE_PTR
			"vld1.32 {q0}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d15[0]			\n\t" // q1 = q1 + M3_2 * TZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vadd.f32 q6, q1, q6				\n\t" // q6 = T1 + T2 + T3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * V1XXXX
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load V2XYZW
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * V1YYYY
			"vmov s27, s31						\n\t" // T.WWWW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M1_2 * V1ZZZZ
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store TXYZW
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q15, d1[1]			\n\t" // q1 = M1_TRANSLATE * V1WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M2_0 * V2XXXX
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load V3XYZW
			"vadd.f32 q3, q2, q3				\n\t" // q3 = V1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * V2YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M2_2 * V2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1
			"vmla.f32 q6, q15, d9[1]			\n\t" // q6 = q6 + M2_TRANSLATE * V2WWWW
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N1XYZW
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M3_0 * V3XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vmla.f32 q3, q13, d0[1]			\n\t" // q3 = V1 + M3_1 * V3YYYY
			"vadd.f32 q6, q5, q6				\n\t" // q6 = V2
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M3_2 * V3ZZZZ
			"vmla.f32 q3, q15, d1[1]			\n\t" // q3 = q3 + M3_3 * V3WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M1_0 * N1XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vadd.f32 q1, q1, q6				\n\t" // q1 = V2 + V3
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load N2XYZW
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M1_1 * N1YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M1_2 * N1ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1 + V2 + V3
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q6, q12, d0[0]			\n\t" // q6 = N1 + M2_0 * N2XXXX
			LOAD_BONE_PTR
			"vst1.32 {q3}, [%[o], :128]!		\n\t" // store VXYZW
			"vmla.f32 q5, q13, d0[1]			\n\t" // q5 = N1 + M2_1 * N2YYYY
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N3XYZW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M2_3 * N2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vadd.f32 q6, q5, q6				\n\t" // q6 = N1 + N2
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load T1XYZW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M3_0 * N3XXXX
			// stall q6
			"vadd.f32 q3, q3, q6				\n\t" // q3 = N1 + N2
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M3_1 * N3YYYY
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			// stall q5
			"vadd.f32 q3, q3, q5				\n\t" // q3 = N1 + N2 + N3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vmla.f32 q6, q14, d9[0]			\n\t" // q6 = q6 + M3_3 * N3ZZZZ
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone1 matrix rows 2 + 3
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load T2XYZW
			LOAD_BONE_PTR
			"vadd.f32 q6, q3, q6				\n\t" // q6 = N1 + N2 + N3
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q2, q12, d8[0]			\n\t" // q2 = q2 + M2_0 * T2XXXX
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store NXYZW
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * T2YYYY
			"vld1.32 {q7}, [%[v], :128]!		\n\t" // load T3XYZW
			"vmla.f32 q1, q14, d9[0]			\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			"vadd.f32 q6, q2, q6				\n\t" // q6 = T1 + T2
			LOAD_BONE_PTR
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vmla.f32 q1, q12, d14[0]			\n\t" // q1 = q1 + M3_0 * TXXXX
			"vmla.f32 q6, q13, d14[1]			\n\t" // q6 = q2 + M3_1 * TYYYY
			
			// VERTEX 10
			
			STORE_BONE_OFS
			
			LOAD_BONE_PTR
			"vld1.32 {q0}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d15[0]			\n\t" // q1 = q1 + M3_2 * TZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vadd.f32 q6, q1, q6				\n\t" // q6 = T1 + T2 + T3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * V1XXXX
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load V2XYZW
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * V1YYYY
			"vmov s27, s31						\n\t" // T.WWWW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M1_2 * V1ZZZZ
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store TXYZW
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q15, d1[1]			\n\t" // q1 = M1_TRANSLATE * V1WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M2_0 * V2XXXX
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load V3XYZW
			"vadd.f32 q3, q2, q3				\n\t" // q3 = V1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * V2YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M2_2 * V2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1
			"vmla.f32 q6, q15, d9[1]			\n\t" // q6 = q6 + M2_TRANSLATE * V2WWWW
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N1XYZW
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M3_0 * V3XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vmla.f32 q3, q13, d0[1]			\n\t" // q3 = V1 + M3_1 * V3YYYY
			"vadd.f32 q6, q5, q6				\n\t" // q6 = V2
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M3_2 * V3ZZZZ
			"vmla.f32 q3, q15, d1[1]			\n\t" // q3 = q3 + M3_3 * V3WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M1_0 * N1XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vadd.f32 q1, q1, q6				\n\t" // q1 = V2 + V3
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load N2XYZW
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M1_1 * N1YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M1_2 * N1ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1 + V2 + V3
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q6, q12, d0[0]			\n\t" // q6 = N1 + M2_0 * N2XXXX
			LOAD_BONE_PTR
			"vst1.32 {q3}, [%[o], :128]!		\n\t" // store VXYZW
			"vmla.f32 q5, q13, d0[1]			\n\t" // q5 = N1 + M2_1 * N2YYYY
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N3XYZW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M2_3 * N2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vadd.f32 q6, q5, q6				\n\t" // q6 = N1 + N2
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load T1XYZW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M3_0 * N3XXXX
			// stall q6
			"vadd.f32 q3, q3, q6				\n\t" // q3 = N1 + N2
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M3_1 * N3YYYY
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			// stall q5
			"vadd.f32 q3, q3, q5				\n\t" // q3 = N1 + N2 + N3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vmla.f32 q6, q14, d9[0]			\n\t" // q6 = q6 + M3_3 * N3ZZZZ
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone1 matrix rows 2 + 3
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load T2XYZW
			LOAD_BONE_PTR
			"vadd.f32 q6, q3, q6				\n\t" // q6 = N1 + N2 + N3
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q2, q12, d8[0]			\n\t" // q2 = q2 + M2_0 * T2XXXX
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store NXYZW
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * T2YYYY
			"vld1.32 {q7}, [%[v], :128]!		\n\t" // load T3XYZW
			"vmla.f32 q1, q14, d9[0]			\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			"vadd.f32 q6, q2, q6				\n\t" // q6 = T1 + T2
			LOAD_BONE_PTR
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vmla.f32 q1, q12, d14[0]			\n\t" // q1 = q1 + M3_0 * TXXXX
			"vmla.f32 q6, q13, d14[1]			\n\t" // q6 = q2 + M3_1 * TYYYY
			
			// VERTEX 11
			
			STORE_BONE_OFS
			
			LOAD_BONE_PTR
			"vld1.32 {q0}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d15[0]			\n\t" // q1 = q1 + M3_2 * TZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vadd.f32 q6, q1, q6				\n\t" // q6 = T1 + T2 + T3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * V1XXXX
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load V2XYZW
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * V1YYYY
			"vmov s27, s31						\n\t" // T.WWWW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M1_2 * V1ZZZZ
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store TXYZW
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q15, d1[1]			\n\t" // q1 = M1_TRANSLATE * V1WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M2_0 * V2XXXX
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load V3XYZW
			"vadd.f32 q3, q2, q3				\n\t" // q3 = V1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * V2YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M2_2 * V2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1
			"vmla.f32 q6, q15, d9[1]			\n\t" // q6 = q6 + M2_TRANSLATE * V2WWWW
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N1XYZW
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M3_0 * V3XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vmla.f32 q3, q13, d0[1]			\n\t" // q3 = V1 + M3_1 * V3YYYY
			"vadd.f32 q6, q5, q6				\n\t" // q6 = V2
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M3_2 * V3ZZZZ
			"vmla.f32 q3, q15, d1[1]			\n\t" // q3 = q3 + M3_3 * V3WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M1_0 * N1XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vadd.f32 q1, q1, q6				\n\t" // q1 = V2 + V3
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load N2XYZW
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M1_1 * N1YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M1_2 * N1ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1 + V2 + V3
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q6, q12, d0[0]			\n\t" // q6 = N1 + M2_0 * N2XXXX
			LOAD_BONE_PTR
			"vst1.32 {q3}, [%[o], :128]!		\n\t" // store VXYZW
			"vmla.f32 q5, q13, d0[1]			\n\t" // q5 = N1 + M2_1 * N2YYYY
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N3XYZW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M2_3 * N2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vadd.f32 q6, q5, q6				\n\t" // q6 = N1 + N2
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load T1XYZW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M3_0 * N3XXXX
			// stall q6
			"vadd.f32 q3, q3, q6				\n\t" // q3 = N1 + N2
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M3_1 * N3YYYY
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			// stall q5
			"vadd.f32 q3, q3, q5				\n\t" // q3 = N1 + N2 + N3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vmla.f32 q6, q14, d9[0]			\n\t" // q6 = q6 + M3_3 * N3ZZZZ
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone1 matrix rows 2 + 3
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load T2XYZW
			LOAD_BONE_PTR
			"vadd.f32 q6, q3, q6				\n\t" // q6 = N1 + N2 + N3
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q2, q12, d8[0]			\n\t" // q2 = q2 + M2_0 * T2XXXX
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store NXYZW
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * T2YYYY
			"vld1.32 {q7}, [%[v], :128]!		\n\t" // load T3XYZW
			"vmla.f32 q1, q14, d9[0]			\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			"vadd.f32 q6, q2, q6				\n\t" // q6 = T1 + T2
			LOAD_BONE_PTR
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vmla.f32 q1, q12, d14[0]			\n\t" // q1 = q1 + M3_0 * TXXXX
			"vmla.f32 q6, q13, d14[1]			\n\t" // q6 = q2 + M3_1 * TYYYY
			
			// VERTEX 12
			
			STORE_BONE_OFS
			
			LOAD_BONE_PTR
			"vld1.32 {q0}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d15[0]			\n\t" // q1 = q1 + M3_2 * TZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vadd.f32 q6, q1, q6				\n\t" // q6 = T1 + T2 + T3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * V1XXXX
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load V2XYZW
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * V1YYYY
			"vmov s27, s31						\n\t" // T.WWWW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M1_2 * V1ZZZZ
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store TXYZW
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q15, d1[1]			\n\t" // q1 = M1_TRANSLATE * V1WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M2_0 * V2XXXX
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load V3XYZW
			"vadd.f32 q3, q2, q3				\n\t" // q3 = V1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * V2YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M2_2 * V2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1
			"vmla.f32 q6, q15, d9[1]			\n\t" // q6 = q6 + M2_TRANSLATE * V2WWWW
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N1XYZW
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M3_0 * V3XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vmla.f32 q3, q13, d0[1]			\n\t" // q3 = V1 + M3_1 * V3YYYY
			"vadd.f32 q6, q5, q6				\n\t" // q6 = V2
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M3_2 * V3ZZZZ
			"vmla.f32 q3, q15, d1[1]			\n\t" // q3 = q3 + M3_3 * V3WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M1_0 * N1XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vadd.f32 q1, q1, q6				\n\t" // q1 = V2 + V3
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load N2XYZW
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M1_1 * N1YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M1_2 * N1ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1 + V2 + V3
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q6, q12, d0[0]			\n\t" // q6 = N1 + M2_0 * N2XXXX
			LOAD_BONE_PTR
			"vst1.32 {q3}, [%[o], :128]!		\n\t" // store VXYZW
			"vmla.f32 q5, q13, d0[1]			\n\t" // q5 = N1 + M2_1 * N2YYYY
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N3XYZW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M2_3 * N2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vadd.f32 q6, q5, q6				\n\t" // q6 = N1 + N2
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load T1XYZW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M3_0 * N3XXXX
			// stall q6
			"vadd.f32 q3, q3, q6				\n\t" // q3 = N1 + N2
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M3_1 * N3YYYY
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			// stall q5
			"vadd.f32 q3, q3, q5				\n\t" // q3 = N1 + N2 + N3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vmla.f32 q6, q14, d9[0]			\n\t" // q6 = q6 + M3_3 * N3ZZZZ
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone1 matrix rows 2 + 3
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load T2XYZW
			LOAD_BONE_PTR
			"vadd.f32 q6, q3, q6				\n\t" // q6 = N1 + N2 + N3
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q2, q12, d8[0]			\n\t" // q2 = q2 + M2_0 * T2XXXX
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store NXYZW
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * T2YYYY
			"vld1.32 {q7}, [%[v], :128]!		\n\t" // load T3XYZW
			"vmla.f32 q1, q14, d9[0]			\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			"vadd.f32 q6, q2, q6				\n\t" // q6 = T1 + T2
			LOAD_BONE_PTR
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vmla.f32 q1, q12, d14[0]			\n\t" // q1 = q1 + M3_0 * TXXXX
			"vmla.f32 q6, q13, d14[1]			\n\t" // q6 = q2 + M3_1 * TYYYY
			
			// VERTEX 13
			
			STORE_BONE_OFS
			
			LOAD_BONE_PTR
			"vld1.32 {q0}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d15[0]			\n\t" // q1 = q1 + M3_2 * TZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vadd.f32 q6, q1, q6				\n\t" // q6 = T1 + T2 + T3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * V1XXXX
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load V2XYZW
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * V1YYYY
			"vmov s27, s31						\n\t" // T.WWWW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M1_2 * V1ZZZZ
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store TXYZW
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q15, d1[1]			\n\t" // q1 = M1_TRANSLATE * V1WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M2_0 * V2XXXX
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load V3XYZW
			"vadd.f32 q3, q2, q3				\n\t" // q3 = V1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * V2YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M2_2 * V2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1
			"vmla.f32 q6, q15, d9[1]			\n\t" // q6 = q6 + M2_TRANSLATE * V2WWWW
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N1XYZW
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M3_0 * V3XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vmla.f32 q3, q13, d0[1]			\n\t" // q3 = V1 + M3_1 * V3YYYY
			"vadd.f32 q6, q5, q6				\n\t" // q6 = V2
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M3_2 * V3ZZZZ
			"vmla.f32 q3, q15, d1[1]			\n\t" // q3 = q3 + M3_3 * V3WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M1_0 * N1XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vadd.f32 q1, q1, q6				\n\t" // q1 = V2 + V3
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load N2XYZW
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M1_1 * N1YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M1_2 * N1ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1 + V2 + V3
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q6, q12, d0[0]			\n\t" // q6 = N1 + M2_0 * N2XXXX
			LOAD_BONE_PTR
			"vst1.32 {q3}, [%[o], :128]!		\n\t" // store VXYZW
			"vmla.f32 q5, q13, d0[1]			\n\t" // q5 = N1 + M2_1 * N2YYYY
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N3XYZW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M2_3 * N2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vadd.f32 q6, q5, q6				\n\t" // q6 = N1 + N2
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load T1XYZW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M3_0 * N3XXXX
			// stall q6
			"vadd.f32 q3, q3, q6				\n\t" // q3 = N1 + N2
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M3_1 * N3YYYY
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			// stall q5
			"vadd.f32 q3, q3, q5				\n\t" // q3 = N1 + N2 + N3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vmla.f32 q6, q14, d9[0]			\n\t" // q6 = q6 + M3_3 * N3ZZZZ
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone1 matrix rows 2 + 3
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load T2XYZW
			LOAD_BONE_PTR
			"vadd.f32 q6, q3, q6				\n\t" // q6 = N1 + N2 + N3
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q2, q12, d8[0]			\n\t" // q2 = q2 + M2_0 * T2XXXX
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store NXYZW
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * T2YYYY
			"vld1.32 {q7}, [%[v], :128]!		\n\t" // load T3XYZW
			"vmla.f32 q1, q14, d9[0]			\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			"vadd.f32 q6, q2, q6				\n\t" // q6 = T1 + T2
			LOAD_BONE_PTR
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vmla.f32 q1, q12, d14[0]			\n\t" // q1 = q1 + M3_0 * TXXXX
			"vmla.f32 q6, q13, d14[1]			\n\t" // q6 = q2 + M3_1 * TYYYY
			
			// VERTEX 14
			
			STORE_BONE_OFS
			
			LOAD_BONE_PTR
			"vld1.32 {q0}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d15[0]			\n\t" // q1 = q1 + M3_2 * TZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vadd.f32 q6, q1, q6				\n\t" // q6 = T1 + T2 + T3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * V1XXXX
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load V2XYZW
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * V1YYYY
			"vmov s27, s31						\n\t" // T.WWWW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M1_2 * V1ZZZZ
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store TXYZW
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q15, d1[1]			\n\t" // q1 = M1_TRANSLATE * V1WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M2_0 * V2XXXX
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load V3XYZW
			"vadd.f32 q3, q2, q3				\n\t" // q3 = V1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * V2YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M2_2 * V2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1
			"vmla.f32 q6, q15, d9[1]			\n\t" // q6 = q6 + M2_TRANSLATE * V2WWWW
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N1XYZW
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M3_0 * V3XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vmla.f32 q3, q13, d0[1]			\n\t" // q3 = V1 + M3_1 * V3YYYY
			"vadd.f32 q6, q5, q6				\n\t" // q6 = V2
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M3_2 * V3ZZZZ
			"vmla.f32 q3, q15, d1[1]			\n\t" // q3 = q3 + M3_3 * V3WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M1_0 * N1XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vadd.f32 q1, q1, q6				\n\t" // q1 = V2 + V3
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load N2XYZW
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M1_1 * N1YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M1_2 * N1ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1 + V2 + V3
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q6, q12, d0[0]			\n\t" // q6 = N1 + M2_0 * N2XXXX
			LOAD_BONE_PTR
			"vst1.32 {q3}, [%[o], :128]!		\n\t" // store VXYZW
			"vmla.f32 q5, q13, d0[1]			\n\t" // q5 = N1 + M2_1 * N2YYYY
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N3XYZW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M2_3 * N2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vadd.f32 q6, q5, q6				\n\t" // q6 = N1 + N2
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load T1XYZW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M3_0 * N3XXXX
			// stall q6
			"vadd.f32 q3, q3, q6				\n\t" // q3 = N1 + N2
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M3_1 * N3YYYY
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			// stall q5
			"vadd.f32 q3, q3, q5				\n\t" // q3 = N1 + N2 + N3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vmla.f32 q6, q14, d9[0]			\n\t" // q6 = q6 + M3_3 * N3ZZZZ
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone1 matrix rows 2 + 3
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load T2XYZW
			LOAD_BONE_PTR
			"vadd.f32 q6, q3, q6				\n\t" // q6 = N1 + N2 + N3
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q2, q12, d8[0]			\n\t" // q2 = q2 + M2_0 * T2XXXX
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store NXYZW
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * T2YYYY
			"vld1.32 {q7}, [%[v], :128]!		\n\t" // load T3XYZW
			"vmla.f32 q1, q14, d9[0]			\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			"vadd.f32 q6, q2, q6				\n\t" // q6 = T1 + T2
			LOAD_BONE_PTR
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vmla.f32 q1, q12, d14[0]			\n\t" // q1 = q1 + M3_0 * TXXXX
			"vmla.f32 q6, q13, d14[1]			\n\t" // q6 = q2 + M3_1 * TYYYY
			
			// VERTEX 15
			
			STORE_BONE_OFS
			
			LOAD_BONE_PTR
			"vld1.32 {q0}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d15[0]			\n\t" // q1 = q1 + M3_2 * TZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vadd.f32 q6, q1, q6				\n\t" // q6 = T1 + T2 + T3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * V1XXXX
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load V2XYZW
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * V1YYYY
			"vmov s27, s31						\n\t" // T.WWWW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M1_2 * V1ZZZZ
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store TXYZW
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q15, d1[1]			\n\t" // q1 = M1_TRANSLATE * V1WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M2_0 * V2XXXX
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load V3XYZW
			"vadd.f32 q3, q2, q3				\n\t" // q3 = V1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * V2YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M2_2 * V2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1
			"vmla.f32 q6, q15, d9[1]			\n\t" // q6 = q6 + M2_TRANSLATE * V2WWWW
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N1XYZW
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M3_0 * V3XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vmla.f32 q3, q13, d0[1]			\n\t" // q3 = V1 + M3_1 * V3YYYY
			"vadd.f32 q6, q5, q6				\n\t" // q6 = V2
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M3_2 * V3ZZZZ
			"vmla.f32 q3, q15, d1[1]			\n\t" // q3 = q3 + M3_3 * V3WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M1_0 * N1XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vadd.f32 q1, q1, q6				\n\t" // q1 = V2 + V3
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load N2XYZW
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M1_1 * N1YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M1_2 * N1ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1 + V2 + V3
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q6, q12, d0[0]			\n\t" // q6 = N1 + M2_0 * N2XXXX
			LOAD_BONE_PTR
			"vst1.32 {q3}, [%[o], :128]!		\n\t" // store VXYZW
			"vmla.f32 q5, q13, d0[1]			\n\t" // q5 = N1 + M2_1 * N2YYYY
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N3XYZW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M2_3 * N2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vadd.f32 q6, q5, q6				\n\t" // q6 = N1 + N2
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load T1XYZW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M3_0 * N3XXXX
			// stall q6
			"vadd.f32 q3, q3, q6				\n\t" // q3 = N1 + N2
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M3_1 * N3YYYY
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			// stall q5
			"vadd.f32 q3, q3, q5				\n\t" // q3 = N1 + N2 + N3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vmla.f32 q6, q14, d9[0]			\n\t" // q6 = q6 + M3_3 * N3ZZZZ
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone1 matrix rows 2 + 3
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load T2XYZW
			LOAD_BONE_PTR
			"vadd.f32 q6, q3, q6				\n\t" // q6 = N1 + N2 + N3
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q2, q12, d8[0]			\n\t" // q2 = q2 + M2_0 * T2XXXX
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store NXYZW
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * T2YYYY
			"vld1.32 {q7}, [%[v], :128]!		\n\t" // load T3XYZW
			"vmla.f32 q1, q14, d9[0]			\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			"vadd.f32 q6, q2, q6				\n\t" // q6 = T1 + T2
			LOAD_BONE_PTR
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vmla.f32 q1, q12, d14[0]			\n\t" // q1 = q1 + M3_0 * TXXXX
			"vmla.f32 q6, q13, d14[1]			\n\t" // q6 = q2 + M3_1 * TYYYY
			
			// VERTEX 16
			
			STORE_BONE_OFS
			
			LOAD_BONE_PTR
			"vld1.32 {q0}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d15[0]			\n\t" // q1 = q1 + M3_2 * TZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vadd.f32 q6, q1, q6				\n\t" // q6 = T1 + T2 + T3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * V1XXXX
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load V2XYZW
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * V1YYYY
			"vmov s27, s31						\n\t" // T.WWWW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M1_2 * V1ZZZZ
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store TXYZW
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q15, d1[1]			\n\t" // q1 = M1_TRANSLATE * V1WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M2_0 * V2XXXX
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load V3XYZW
			"vadd.f32 q3, q2, q3				\n\t" // q3 = V1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * V2YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M2_2 * V2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1
			"vmla.f32 q6, q15, d9[1]			\n\t" // q6 = q6 + M2_TRANSLATE * V2WWWW
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N1XYZW
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M3_0 * V3XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vmla.f32 q3, q13, d0[1]			\n\t" // q3 = V1 + M3_1 * V3YYYY
			"vadd.f32 q6, q5, q6				\n\t" // q6 = V2
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M3_2 * V3ZZZZ
			"vmla.f32 q3, q15, d1[1]			\n\t" // q3 = q3 + M3_3 * V3WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M1_0 * N1XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vadd.f32 q1, q1, q6				\n\t" // q1 = V2 + V3
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load N2XYZW
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M1_1 * N1YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M1_2 * N1ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1 + V2 + V3
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q6, q12, d0[0]			\n\t" // q6 = N1 + M2_0 * N2XXXX
			LOAD_BONE_PTR
			"vst1.32 {q3}, [%[o], :128]!		\n\t" // store VXYZW
			"vmla.f32 q5, q13, d0[1]			\n\t" // q5 = N1 + M2_1 * N2YYYY
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N3XYZW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M2_3 * N2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vadd.f32 q6, q5, q6				\n\t" // q6 = N1 + N2
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load T1XYZW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M3_0 * N3XXXX
			// stall q6
			"vadd.f32 q3, q3, q6				\n\t" // q3 = N1 + N2
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M3_1 * N3YYYY
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			// stall q5
			"vadd.f32 q3, q3, q5				\n\t" // q3 = N1 + N2 + N3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vmla.f32 q6, q14, d9[0]			\n\t" // q6 = q6 + M3_3 * N3ZZZZ
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone1 matrix rows 2 + 3
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load T2XYZW
			LOAD_BONE_PTR
			"vadd.f32 q6, q3, q6				\n\t" // q6 = N1 + N2 + N3
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q2, q12, d8[0]			\n\t" // q2 = q2 + M2_0 * T2XXXX
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store NXYZW
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * T2YYYY
			"vld1.32 {q7}, [%[v], :128]!		\n\t" // load T3XYZW
			"vmla.f32 q1, q14, d9[0]			\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			"vadd.f32 q6, q2, q6				\n\t" // q6 = T1 + T2
			LOAD_BONE_PTR
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vmla.f32 q1, q12, d14[0]			\n\t" // q1 = q1 + M3_0 * TXXXX
			"vmla.f32 q6, q13, d14[1]			\n\t" // q6 = q2 + M3_1 * TYYYY
			
			// stall q1
			"vmla.f32 q1, q14, d15[0]			\n\t" // q1 = q1 + M3_2 * TZZZZ
			// stall q1
			"vadd.f32 q6, q1, q6				\n\t" // q6 = T1 + T2 + T3
			// stall q1
			"vmov s27, s31						\n\t" // T.WWWW
			// stall q1
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store TXYZW
			
			STORE_BONE_OFS
								
			// loop
			"sub %[nv], %[nv], #16			\n\t"
			"cmp %[nv], #16					\n\t"
			"bge Lskin3_16					\n\t"
			
		: [o] "+r" (outVerts), [v] "+r" (vertices), [bi] "+r" (boneIndices), [nv] "+r" (numVerts)
		: [bones] "r" (bones)
		: "cc", "r9", "r10", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
		);
	}
	
	if (numVerts >= 8) {
		
		asm volatile (
		
			LOAD_BONE_OFS
			
			"Lskin3_8:						\n\t"
						
			LOAD_BONE_PTR
			"vld1.32 {q0}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * V1XXXX
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load V2XYZW
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * V1YYYY
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M1_2 * V1ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q15, d1[1]			\n\t" // q1 = M1_TRANSLATE * V1WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M2_0 * V2XXXX
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load V3XYZW
			"vadd.f32 q3, q2, q3				\n\t" // q3 = V1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * V2YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M2_2 * V2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1
			"vmla.f32 q6, q15, d9[1]			\n\t" // q6 = q6 + M2_TRANSLATE * V2WWWW
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N1XYZW
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M3_0 * V3XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vmla.f32 q3, q13, d0[1]			\n\t" // q3 = V1 + M3_1 * V3YYYY
			"vadd.f32 q6, q5, q6				\n\t" // q6 = V2
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M3_2 * V3ZZZZ
			"vmla.f32 q3, q15, d1[1]			\n\t" // q3 = q3 + M3_3 * V3WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M1_0 * N1XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vadd.f32 q1, q1, q6				\n\t" // q1 = V2 + V3
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load N2XYZW
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M1_1 * N1YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M1_2 * N1ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1 + V2 + V3
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q6, q12, d0[0]			\n\t" // q6 = N1 + M2_0 * N2XXXX
			LOAD_BONE_PTR
			"vst1.32 {q3}, [%[o], :128]!		\n\t" // store VXYZW
			"vmla.f32 q5, q13, d0[1]			\n\t" // q5 = N1 + M2_1 * N2YYYY
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N3XYZW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M2_3 * N2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vadd.f32 q6, q5, q6				\n\t" // q6 = N1 + N2
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load T1XYZW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M3_0 * N3XXXX
			// stall q6
			"vadd.f32 q3, q3, q6				\n\t" // q3 = N1 + N2
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M3_1 * N3YYYY
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			// stall q5
			"vadd.f32 q3, q3, q5				\n\t" // q3 = N1 + N2 + N3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vmla.f32 q6, q14, d9[0]			\n\t" // q6 = q6 + M3_3 * N3ZZZZ
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone1 matrix rows 2 + 3
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load T2XYZW
			LOAD_BONE_PTR
			"vadd.f32 q6, q3, q6				\n\t" // q6 = N1 + N2 + N3
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q2, q12, d8[0]			\n\t" // q2 = q2 + M2_0 * T2XXXX
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store NXYZW
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * T2YYYY
			"vld1.32 {q7}, [%[v], :128]!		\n\t" // load T3XYZW
			"vmla.f32 q1, q14, d9[0]			\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			"vadd.f32 q6, q2, q6				\n\t" // q6 = T1 + T2
			LOAD_BONE_PTR
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vmla.f32 q1, q12, d14[0]			\n\t" // q1 = q1 + M3_0 * TXXXX
			"vmla.f32 q6, q13, d14[1]			\n\t" // q6 = q2 + M3_1 * TYYYY
			
			// VERTEX 2
			
			STORE_BONE_OFS
			
			LOAD_BONE_PTR
			"vld1.32 {q0}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d15[0]			\n\t" // q1 = q1 + M3_2 * TZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vadd.f32 q6, q1, q6				\n\t" // q6 = T1 + T2 + T3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * V1XXXX
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load V2XYZW
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * V1YYYY
			"vmov s27, s31						\n\t" // T.WWWW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M1_2 * V1ZZZZ
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store TXYZW
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q15, d1[1]			\n\t" // q1 = M1_TRANSLATE * V1WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M2_0 * V2XXXX
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load V3XYZW
			"vadd.f32 q3, q2, q3				\n\t" // q3 = V1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * V2YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M2_2 * V2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1
			"vmla.f32 q6, q15, d9[1]			\n\t" // q6 = q6 + M2_TRANSLATE * V2WWWW
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N1XYZW
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M3_0 * V3XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vmla.f32 q3, q13, d0[1]			\n\t" // q3 = V1 + M3_1 * V3YYYY
			"vadd.f32 q6, q5, q6				\n\t" // q6 = V2
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M3_2 * V3ZZZZ
			"vmla.f32 q3, q15, d1[1]			\n\t" // q3 = q3 + M3_3 * V3WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M1_0 * N1XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vadd.f32 q1, q1, q6				\n\t" // q1 = V2 + V3
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load N2XYZW
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M1_1 * N1YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M1_2 * N1ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1 + V2 + V3
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q6, q12, d0[0]			\n\t" // q6 = N1 + M2_0 * N2XXXX
			LOAD_BONE_PTR
			"vst1.32 {q3}, [%[o], :128]!		\n\t" // store VXYZW
			"vmla.f32 q5, q13, d0[1]			\n\t" // q5 = N1 + M2_1 * N2YYYY
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N3XYZW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M2_3 * N2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vadd.f32 q6, q5, q6				\n\t" // q6 = N1 + N2
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load T1XYZW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M3_0 * N3XXXX
			// stall q6
			"vadd.f32 q3, q3, q6				\n\t" // q3 = N1 + N2
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M3_1 * N3YYYY
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			// stall q5
			"vadd.f32 q3, q3, q5				\n\t" // q3 = N1 + N2 + N3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vmla.f32 q6, q14, d9[0]			\n\t" // q6 = q6 + M3_3 * N3ZZZZ
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone1 matrix rows 2 + 3
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load T2XYZW
			LOAD_BONE_PTR
			"vadd.f32 q6, q3, q6				\n\t" // q6 = N1 + N2 + N3
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q2, q12, d8[0]			\n\t" // q2 = q2 + M2_0 * T2XXXX
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store NXYZW
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * T2YYYY
			"vld1.32 {q7}, [%[v], :128]!		\n\t" // load T3XYZW
			"vmla.f32 q1, q14, d9[0]			\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			"vadd.f32 q6, q2, q6				\n\t" // q6 = T1 + T2
			LOAD_BONE_PTR
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vmla.f32 q1, q12, d14[0]			\n\t" // q1 = q1 + M3_0 * TXXXX
			"vmla.f32 q6, q13, d14[1]			\n\t" // q6 = q2 + M3_1 * TYYYY
			
			// VERTEX 3
			
			STORE_BONE_OFS
			
			LOAD_BONE_PTR
			"vld1.32 {q0}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d15[0]			\n\t" // q1 = q1 + M3_2 * TZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vadd.f32 q6, q1, q6				\n\t" // q6 = T1 + T2 + T3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * V1XXXX
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load V2XYZW
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * V1YYYY
			"vmov s27, s31						\n\t" // T.WWWW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M1_2 * V1ZZZZ
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store TXYZW
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q15, d1[1]			\n\t" // q1 = M1_TRANSLATE * V1WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M2_0 * V2XXXX
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load V3XYZW
			"vadd.f32 q3, q2, q3				\n\t" // q3 = V1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * V2YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M2_2 * V2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1
			"vmla.f32 q6, q15, d9[1]			\n\t" // q6 = q6 + M2_TRANSLATE * V2WWWW
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N1XYZW
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M3_0 * V3XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vmla.f32 q3, q13, d0[1]			\n\t" // q3 = V1 + M3_1 * V3YYYY
			"vadd.f32 q6, q5, q6				\n\t" // q6 = V2
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M3_2 * V3ZZZZ
			"vmla.f32 q3, q15, d1[1]			\n\t" // q3 = q3 + M3_3 * V3WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M1_0 * N1XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vadd.f32 q1, q1, q6				\n\t" // q1 = V2 + V3
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load N2XYZW
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M1_1 * N1YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M1_2 * N1ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1 + V2 + V3
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q6, q12, d0[0]			\n\t" // q6 = N1 + M2_0 * N2XXXX
			LOAD_BONE_PTR
			"vst1.32 {q3}, [%[o], :128]!		\n\t" // store VXYZW
			"vmla.f32 q5, q13, d0[1]			\n\t" // q5 = N1 + M2_1 * N2YYYY
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N3XYZW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M2_3 * N2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vadd.f32 q6, q5, q6				\n\t" // q6 = N1 + N2
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load T1XYZW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M3_0 * N3XXXX
			// stall q6
			"vadd.f32 q3, q3, q6				\n\t" // q3 = N1 + N2
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M3_1 * N3YYYY
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			// stall q5
			"vadd.f32 q3, q3, q5				\n\t" // q3 = N1 + N2 + N3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vmla.f32 q6, q14, d9[0]			\n\t" // q6 = q6 + M3_3 * N3ZZZZ
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone1 matrix rows 2 + 3
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load T2XYZW
			LOAD_BONE_PTR
			"vadd.f32 q6, q3, q6				\n\t" // q6 = N1 + N2 + N3
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q2, q12, d8[0]			\n\t" // q2 = q2 + M2_0 * T2XXXX
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store NXYZW
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * T2YYYY
			"vld1.32 {q7}, [%[v], :128]!		\n\t" // load T3XYZW
			"vmla.f32 q1, q14, d9[0]			\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			"vadd.f32 q6, q2, q6				\n\t" // q6 = T1 + T2
			LOAD_BONE_PTR
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vmla.f32 q1, q12, d14[0]			\n\t" // q1 = q1 + M3_0 * TXXXX
			"vmla.f32 q6, q13, d14[1]			\n\t" // q6 = q2 + M3_1 * TYYYY
			
			// VERTEX 4
			
			STORE_BONE_OFS
			
			LOAD_BONE_PTR
			"vld1.32 {q0}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d15[0]			\n\t" // q1 = q1 + M3_2 * TZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vadd.f32 q6, q1, q6				\n\t" // q6 = T1 + T2 + T3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * V1XXXX
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load V2XYZW
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * V1YYYY
			"vmov s27, s31						\n\t" // T.WWWW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M1_2 * V1ZZZZ
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store TXYZW
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q15, d1[1]			\n\t" // q1 = M1_TRANSLATE * V1WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M2_0 * V2XXXX
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load V3XYZW
			"vadd.f32 q3, q2, q3				\n\t" // q3 = V1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * V2YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M2_2 * V2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1
			"vmla.f32 q6, q15, d9[1]			\n\t" // q6 = q6 + M2_TRANSLATE * V2WWWW
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N1XYZW
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M3_0 * V3XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vmla.f32 q3, q13, d0[1]			\n\t" // q3 = V1 + M3_1 * V3YYYY
			"vadd.f32 q6, q5, q6				\n\t" // q6 = V2
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M3_2 * V3ZZZZ
			"vmla.f32 q3, q15, d1[1]			\n\t" // q3 = q3 + M3_3 * V3WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M1_0 * N1XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vadd.f32 q1, q1, q6				\n\t" // q1 = V2 + V3
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load N2XYZW
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M1_1 * N1YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M1_2 * N1ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1 + V2 + V3
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q6, q12, d0[0]			\n\t" // q6 = N1 + M2_0 * N2XXXX
			LOAD_BONE_PTR
			"vst1.32 {q3}, [%[o], :128]!		\n\t" // store VXYZW
			"vmla.f32 q5, q13, d0[1]			\n\t" // q5 = N1 + M2_1 * N2YYYY
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N3XYZW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M2_3 * N2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vadd.f32 q6, q5, q6				\n\t" // q6 = N1 + N2
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load T1XYZW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M3_0 * N3XXXX
			// stall q6
			"vadd.f32 q3, q3, q6				\n\t" // q3 = N1 + N2
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M3_1 * N3YYYY
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			// stall q5
			"vadd.f32 q3, q3, q5				\n\t" // q3 = N1 + N2 + N3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vmla.f32 q6, q14, d9[0]			\n\t" // q6 = q6 + M3_3 * N3ZZZZ
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone1 matrix rows 2 + 3
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load T2XYZW
			LOAD_BONE_PTR
			"vadd.f32 q6, q3, q6				\n\t" // q6 = N1 + N2 + N3
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q2, q12, d8[0]			\n\t" // q2 = q2 + M2_0 * T2XXXX
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store NXYZW
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * T2YYYY
			"vld1.32 {q7}, [%[v], :128]!		\n\t" // load T3XYZW
			"vmla.f32 q1, q14, d9[0]			\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			"vadd.f32 q6, q2, q6				\n\t" // q6 = T1 + T2
			LOAD_BONE_PTR
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vmla.f32 q1, q12, d14[0]			\n\t" // q1 = q1 + M3_0 * TXXXX
			"vmla.f32 q6, q13, d14[1]			\n\t" // q6 = q2 + M3_1 * TYYYY
			
			// VERTEX 5
			
			STORE_BONE_OFS
			
			LOAD_BONE_PTR
			"vld1.32 {q0}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d15[0]			\n\t" // q1 = q1 + M3_2 * TZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vadd.f32 q6, q1, q6				\n\t" // q6 = T1 + T2 + T3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * V1XXXX
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load V2XYZW
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * V1YYYY
			"vmov s27, s31						\n\t" // T.WWWW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M1_2 * V1ZZZZ
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store TXYZW
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q15, d1[1]			\n\t" // q1 = M1_TRANSLATE * V1WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M2_0 * V2XXXX
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load V3XYZW
			"vadd.f32 q3, q2, q3				\n\t" // q3 = V1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * V2YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M2_2 * V2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1
			"vmla.f32 q6, q15, d9[1]			\n\t" // q6 = q6 + M2_TRANSLATE * V2WWWW
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N1XYZW
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M3_0 * V3XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vmla.f32 q3, q13, d0[1]			\n\t" // q3 = V1 + M3_1 * V3YYYY
			"vadd.f32 q6, q5, q6				\n\t" // q6 = V2
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M3_2 * V3ZZZZ
			"vmla.f32 q3, q15, d1[1]			\n\t" // q3 = q3 + M3_3 * V3WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M1_0 * N1XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vadd.f32 q1, q1, q6				\n\t" // q1 = V2 + V3
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load N2XYZW
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M1_1 * N1YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M1_2 * N1ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1 + V2 + V3
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q6, q12, d0[0]			\n\t" // q6 = N1 + M2_0 * N2XXXX
			LOAD_BONE_PTR
			"vst1.32 {q3}, [%[o], :128]!		\n\t" // store VXYZW
			"vmla.f32 q5, q13, d0[1]			\n\t" // q5 = N1 + M2_1 * N2YYYY
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N3XYZW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M2_3 * N2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vadd.f32 q6, q5, q6				\n\t" // q6 = N1 + N2
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load T1XYZW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M3_0 * N3XXXX
			// stall q6
			"vadd.f32 q3, q3, q6				\n\t" // q3 = N1 + N2
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M3_1 * N3YYYY
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			// stall q5
			"vadd.f32 q3, q3, q5				\n\t" // q3 = N1 + N2 + N3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vmla.f32 q6, q14, d9[0]			\n\t" // q6 = q6 + M3_3 * N3ZZZZ
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone1 matrix rows 2 + 3
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load T2XYZW
			LOAD_BONE_PTR
			"vadd.f32 q6, q3, q6				\n\t" // q6 = N1 + N2 + N3
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q2, q12, d8[0]			\n\t" // q2 = q2 + M2_0 * T2XXXX
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store NXYZW
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * T2YYYY
			"vld1.32 {q7}, [%[v], :128]!		\n\t" // load T3XYZW
			"vmla.f32 q1, q14, d9[0]			\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			"vadd.f32 q6, q2, q6				\n\t" // q6 = T1 + T2
			LOAD_BONE_PTR
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vmla.f32 q1, q12, d14[0]			\n\t" // q1 = q1 + M3_0 * TXXXX
			"vmla.f32 q6, q13, d14[1]			\n\t" // q6 = q2 + M3_1 * TYYYY
			
			// VERTEX 6
			
			STORE_BONE_OFS
			
			LOAD_BONE_PTR
			"vld1.32 {q0}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d15[0]			\n\t" // q1 = q1 + M3_2 * TZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vadd.f32 q6, q1, q6				\n\t" // q6 = T1 + T2 + T3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * V1XXXX
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load V2XYZW
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * V1YYYY
			"vmov s27, s31						\n\t" // T.WWWW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M1_2 * V1ZZZZ
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store TXYZW
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q15, d1[1]			\n\t" // q1 = M1_TRANSLATE * V1WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M2_0 * V2XXXX
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load V3XYZW
			"vadd.f32 q3, q2, q3				\n\t" // q3 = V1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * V2YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M2_2 * V2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1
			"vmla.f32 q6, q15, d9[1]			\n\t" // q6 = q6 + M2_TRANSLATE * V2WWWW
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N1XYZW
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M3_0 * V3XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vmla.f32 q3, q13, d0[1]			\n\t" // q3 = V1 + M3_1 * V3YYYY
			"vadd.f32 q6, q5, q6				\n\t" // q6 = V2
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M3_2 * V3ZZZZ
			"vmla.f32 q3, q15, d1[1]			\n\t" // q3 = q3 + M3_3 * V3WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M1_0 * N1XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vadd.f32 q1, q1, q6				\n\t" // q1 = V2 + V3
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load N2XYZW
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M1_1 * N1YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M1_2 * N1ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1 + V2 + V3
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q6, q12, d0[0]			\n\t" // q6 = N1 + M2_0 * N2XXXX
			LOAD_BONE_PTR
			"vst1.32 {q3}, [%[o], :128]!		\n\t" // store VXYZW
			"vmla.f32 q5, q13, d0[1]			\n\t" // q5 = N1 + M2_1 * N2YYYY
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N3XYZW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M2_3 * N2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vadd.f32 q6, q5, q6				\n\t" // q6 = N1 + N2
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load T1XYZW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M3_0 * N3XXXX
			// stall q6
			"vadd.f32 q3, q3, q6				\n\t" // q3 = N1 + N2
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M3_1 * N3YYYY
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			// stall q5
			"vadd.f32 q3, q3, q5				\n\t" // q3 = N1 + N2 + N3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vmla.f32 q6, q14, d9[0]			\n\t" // q6 = q6 + M3_3 * N3ZZZZ
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone1 matrix rows 2 + 3
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load T2XYZW
			LOAD_BONE_PTR
			"vadd.f32 q6, q3, q6				\n\t" // q6 = N1 + N2 + N3
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q2, q12, d8[0]			\n\t" // q2 = q2 + M2_0 * T2XXXX
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store NXYZW
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * T2YYYY
			"vld1.32 {q7}, [%[v], :128]!		\n\t" // load T3XYZW
			"vmla.f32 q1, q14, d9[0]			\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			"vadd.f32 q6, q2, q6				\n\t" // q6 = T1 + T2
			LOAD_BONE_PTR
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vmla.f32 q1, q12, d14[0]			\n\t" // q1 = q1 + M3_0 * TXXXX
			"vmla.f32 q6, q13, d14[1]			\n\t" // q6 = q2 + M3_1 * TYYYY
			
			// VERTEX 7
			
			STORE_BONE_OFS
			
			LOAD_BONE_PTR
			"vld1.32 {q0}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d15[0]			\n\t" // q1 = q1 + M3_2 * TZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vadd.f32 q6, q1, q6				\n\t" // q6 = T1 + T2 + T3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * V1XXXX
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load V2XYZW
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * V1YYYY
			"vmov s27, s31						\n\t" // T.WWWW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M1_2 * V1ZZZZ
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store TXYZW
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q15, d1[1]			\n\t" // q1 = M1_TRANSLATE * V1WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M2_0 * V2XXXX
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load V3XYZW
			"vadd.f32 q3, q2, q3				\n\t" // q3 = V1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * V2YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M2_2 * V2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1
			"vmla.f32 q6, q15, d9[1]			\n\t" // q6 = q6 + M2_TRANSLATE * V2WWWW
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N1XYZW
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M3_0 * V3XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vmla.f32 q3, q13, d0[1]			\n\t" // q3 = V1 + M3_1 * V3YYYY
			"vadd.f32 q6, q5, q6				\n\t" // q6 = V2
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M3_2 * V3ZZZZ
			"vmla.f32 q3, q15, d1[1]			\n\t" // q3 = q3 + M3_3 * V3WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M1_0 * N1XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vadd.f32 q1, q1, q6				\n\t" // q1 = V2 + V3
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load N2XYZW
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M1_1 * N1YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M1_2 * N1ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1 + V2 + V3
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q6, q12, d0[0]			\n\t" // q6 = N1 + M2_0 * N2XXXX
			LOAD_BONE_PTR
			"vst1.32 {q3}, [%[o], :128]!		\n\t" // store VXYZW
			"vmla.f32 q5, q13, d0[1]			\n\t" // q5 = N1 + M2_1 * N2YYYY
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N3XYZW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M2_3 * N2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vadd.f32 q6, q5, q6				\n\t" // q6 = N1 + N2
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load T1XYZW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M3_0 * N3XXXX
			// stall q6
			"vadd.f32 q3, q3, q6				\n\t" // q3 = N1 + N2
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M3_1 * N3YYYY
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			// stall q5
			"vadd.f32 q3, q3, q5				\n\t" // q3 = N1 + N2 + N3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vmla.f32 q6, q14, d9[0]			\n\t" // q6 = q6 + M3_3 * N3ZZZZ
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone1 matrix rows 2 + 3
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load T2XYZW
			LOAD_BONE_PTR
			"vadd.f32 q6, q3, q6				\n\t" // q6 = N1 + N2 + N3
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q2, q12, d8[0]			\n\t" // q2 = q2 + M2_0 * T2XXXX
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store NXYZW
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * T2YYYY
			"vld1.32 {q7}, [%[v], :128]!		\n\t" // load T3XYZW
			"vmla.f32 q1, q14, d9[0]			\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			"vadd.f32 q6, q2, q6				\n\t" // q6 = T1 + T2
			LOAD_BONE_PTR
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vmla.f32 q1, q12, d14[0]			\n\t" // q1 = q1 + M3_0 * TXXXX
			"vmla.f32 q6, q13, d14[1]			\n\t" // q6 = q2 + M3_1 * TYYYY
			
			// VERTEX 8
			
			STORE_BONE_OFS
			
			LOAD_BONE_PTR
			"vld1.32 {q0}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d15[0]			\n\t" // q1 = q1 + M3_2 * TZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vadd.f32 q6, q1, q6				\n\t" // q6 = T1 + T2 + T3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * V1XXXX
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load V2XYZW
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * V1YYYY
			"vmov s27, s31						\n\t" // T.WWWW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M1_2 * V1ZZZZ
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store TXYZW
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q15, d1[1]			\n\t" // q1 = M1_TRANSLATE * V1WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M2_0 * V2XXXX
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load V3XYZW
			"vadd.f32 q3, q2, q3				\n\t" // q3 = V1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * V2YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M2_2 * V2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1
			"vmla.f32 q6, q15, d9[1]			\n\t" // q6 = q6 + M2_TRANSLATE * V2WWWW
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N1XYZW
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M3_0 * V3XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vmla.f32 q3, q13, d0[1]			\n\t" // q3 = V1 + M3_1 * V3YYYY
			"vadd.f32 q6, q5, q6				\n\t" // q6 = V2
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M3_2 * V3ZZZZ
			"vmla.f32 q3, q15, d1[1]			\n\t" // q3 = q3 + M3_3 * V3WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M1_0 * N1XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vadd.f32 q1, q1, q6				\n\t" // q1 = V2 + V3
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load N2XYZW
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M1_1 * N1YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M1_2 * N1ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1 + V2 + V3
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q6, q12, d0[0]			\n\t" // q6 = N1 + M2_0 * N2XXXX
			LOAD_BONE_PTR
			"vst1.32 {q3}, [%[o], :128]!		\n\t" // store VXYZW
			"vmla.f32 q5, q13, d0[1]			\n\t" // q5 = N1 + M2_1 * N2YYYY
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N3XYZW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M2_3 * N2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vadd.f32 q6, q5, q6				\n\t" // q6 = N1 + N2
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load T1XYZW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M3_0 * N3XXXX
			// stall q6
			"vadd.f32 q3, q3, q6				\n\t" // q3 = N1 + N2
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M3_1 * N3YYYY
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			// stall q5
			"vadd.f32 q3, q3, q5				\n\t" // q3 = N1 + N2 + N3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vmla.f32 q6, q14, d9[0]			\n\t" // q6 = q6 + M3_3 * N3ZZZZ
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone1 matrix rows 2 + 3
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load T2XYZW
			LOAD_BONE_PTR
			"vadd.f32 q6, q3, q6				\n\t" // q6 = N1 + N2 + N3
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q2, q12, d8[0]			\n\t" // q2 = q2 + M2_0 * T2XXXX
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store NXYZW
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * T2YYYY
			"vld1.32 {q7}, [%[v], :128]!		\n\t" // load T3XYZW
			"vmla.f32 q1, q14, d9[0]			\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			"vadd.f32 q6, q2, q6				\n\t" // q6 = T1 + T2
			LOAD_BONE_PTR
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vmla.f32 q1, q12, d14[0]			\n\t" // q1 = q1 + M3_0 * TXXXX
			"vmla.f32 q6, q13, d14[1]			\n\t" // q6 = q2 + M3_1 * TYYYY
			
			// stall q1
			"vmla.f32 q1, q14, d15[0]			\n\t" // q1 = q1 + M3_2 * TZZZZ
			// stall q1
			"vadd.f32 q6, q1, q6				\n\t" // q6 = T1 + T2 + T3
			// stall q1
			"vmov s27, s31						\n\t" // T.WWWW
			// stall q1
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store TXYZW
			
			STORE_BONE_OFS
								
			// loop
			"sub %[nv], %[nv], #8			\n\t"
			"cmp %[nv], #8					\n\t"
			"bge Lskin3_8					\n\t"
			
		: [o] "+r" (outVerts), [v] "+r" (vertices), [bi] "+r" (boneIndices), [nv] "+r" (numVerts)
		: [bones] "r" (bones)
		: "cc", "r9", "r10", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
		);
	}
	
	if (numVerts >= 4) {
		
		asm volatile (
		
			LOAD_BONE_OFS
			
			"Lskin3_4:						\n\t"
						
			LOAD_BONE_PTR
			"vld1.32 {q0}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * V1XXXX
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load V2XYZW
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * V1YYYY
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M1_2 * V1ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q15, d1[1]			\n\t" // q1 = M1_TRANSLATE * V1WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M2_0 * V2XXXX
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load V3XYZW
			"vadd.f32 q3, q2, q3				\n\t" // q3 = V1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * V2YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M2_2 * V2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1
			"vmla.f32 q6, q15, d9[1]			\n\t" // q6 = q6 + M2_TRANSLATE * V2WWWW
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N1XYZW
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M3_0 * V3XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vmla.f32 q3, q13, d0[1]			\n\t" // q3 = V1 + M3_1 * V3YYYY
			"vadd.f32 q6, q5, q6				\n\t" // q6 = V2
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M3_2 * V3ZZZZ
			"vmla.f32 q3, q15, d1[1]			\n\t" // q3 = q3 + M3_3 * V3WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M1_0 * N1XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vadd.f32 q1, q1, q6				\n\t" // q1 = V2 + V3
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load N2XYZW
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M1_1 * N1YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M1_2 * N1ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1 + V2 + V3
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q6, q12, d0[0]			\n\t" // q6 = N1 + M2_0 * N2XXXX
			LOAD_BONE_PTR
			"vst1.32 {q3}, [%[o], :128]!		\n\t" // store VXYZW
			"vmla.f32 q5, q13, d0[1]			\n\t" // q5 = N1 + M2_1 * N2YYYY
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N3XYZW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M2_3 * N2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vadd.f32 q6, q5, q6				\n\t" // q6 = N1 + N2
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load T1XYZW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M3_0 * N3XXXX
			// stall q6
			"vadd.f32 q3, q3, q6				\n\t" // q3 = N1 + N2
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M3_1 * N3YYYY
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			// stall q5
			"vadd.f32 q3, q3, q5				\n\t" // q3 = N1 + N2 + N3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vmla.f32 q6, q14, d9[0]			\n\t" // q6 = q6 + M3_3 * N3ZZZZ
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone1 matrix rows 2 + 3
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load T2XYZW
			LOAD_BONE_PTR
			"vadd.f32 q6, q3, q6				\n\t" // q6 = N1 + N2 + N3
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q2, q12, d8[0]			\n\t" // q2 = q2 + M2_0 * T2XXXX
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store NXYZW
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * T2YYYY
			"vld1.32 {q7}, [%[v], :128]!		\n\t" // load T3XYZW
			"vmla.f32 q1, q14, d9[0]			\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			"vadd.f32 q6, q2, q6				\n\t" // q6 = T1 + T2
			LOAD_BONE_PTR
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vmla.f32 q1, q12, d14[0]			\n\t" // q1 = q1 + M3_0 * TXXXX
			"vmla.f32 q6, q13, d14[1]			\n\t" // q6 = q2 + M3_1 * TYYYY
			
			// VERTEX 2
			
			STORE_BONE_OFS
			
			LOAD_BONE_PTR
			"vld1.32 {q0}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d15[0]			\n\t" // q1 = q1 + M3_2 * TZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vadd.f32 q6, q1, q6				\n\t" // q6 = T1 + T2 + T3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * V1XXXX
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load V2XYZW
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * V1YYYY
			"vmov s27, s31						\n\t" // T.WWWW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M1_2 * V1ZZZZ
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store TXYZW
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q15, d1[1]			\n\t" // q1 = M1_TRANSLATE * V1WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M2_0 * V2XXXX
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load V3XYZW
			"vadd.f32 q3, q2, q3				\n\t" // q3 = V1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * V2YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M2_2 * V2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1
			"vmla.f32 q6, q15, d9[1]			\n\t" // q6 = q6 + M2_TRANSLATE * V2WWWW
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N1XYZW
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M3_0 * V3XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vmla.f32 q3, q13, d0[1]			\n\t" // q3 = V1 + M3_1 * V3YYYY
			"vadd.f32 q6, q5, q6				\n\t" // q6 = V2
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M3_2 * V3ZZZZ
			"vmla.f32 q3, q15, d1[1]			\n\t" // q3 = q3 + M3_3 * V3WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M1_0 * N1XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vadd.f32 q1, q1, q6				\n\t" // q1 = V2 + V3
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load N2XYZW
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M1_1 * N1YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M1_2 * N1ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1 + V2 + V3
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q6, q12, d0[0]			\n\t" // q6 = N1 + M2_0 * N2XXXX
			LOAD_BONE_PTR
			"vst1.32 {q3}, [%[o], :128]!		\n\t" // store VXYZW
			"vmla.f32 q5, q13, d0[1]			\n\t" // q5 = N1 + M2_1 * N2YYYY
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N3XYZW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M2_3 * N2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vadd.f32 q6, q5, q6				\n\t" // q6 = N1 + N2
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load T1XYZW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M3_0 * N3XXXX
			// stall q6
			"vadd.f32 q3, q3, q6				\n\t" // q3 = N1 + N2
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M3_1 * N3YYYY
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			// stall q5
			"vadd.f32 q3, q3, q5				\n\t" // q3 = N1 + N2 + N3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vmla.f32 q6, q14, d9[0]			\n\t" // q6 = q6 + M3_3 * N3ZZZZ
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone1 matrix rows 2 + 3
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load T2XYZW
			LOAD_BONE_PTR
			"vadd.f32 q6, q3, q6				\n\t" // q6 = N1 + N2 + N3
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q2, q12, d8[0]			\n\t" // q2 = q2 + M2_0 * T2XXXX
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store NXYZW
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * T2YYYY
			"vld1.32 {q7}, [%[v], :128]!		\n\t" // load T3XYZW
			"vmla.f32 q1, q14, d9[0]			\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			"vadd.f32 q6, q2, q6				\n\t" // q6 = T1 + T2
			LOAD_BONE_PTR
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vmla.f32 q1, q12, d14[0]			\n\t" // q1 = q1 + M3_0 * TXXXX
			"vmla.f32 q6, q13, d14[1]			\n\t" // q6 = q2 + M3_1 * TYYYY
			
			// VERTEX 3
			
			STORE_BONE_OFS
			
			LOAD_BONE_PTR
			"vld1.32 {q0}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d15[0]			\n\t" // q1 = q1 + M3_2 * TZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vadd.f32 q6, q1, q6				\n\t" // q6 = T1 + T2 + T3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * V1XXXX
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load V2XYZW
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * V1YYYY
			"vmov s27, s31						\n\t" // T.WWWW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M1_2 * V1ZZZZ
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store TXYZW
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q15, d1[1]			\n\t" // q1 = M1_TRANSLATE * V1WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M2_0 * V2XXXX
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load V3XYZW
			"vadd.f32 q3, q2, q3				\n\t" // q3 = V1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * V2YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M2_2 * V2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1
			"vmla.f32 q6, q15, d9[1]			\n\t" // q6 = q6 + M2_TRANSLATE * V2WWWW
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N1XYZW
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M3_0 * V3XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vmla.f32 q3, q13, d0[1]			\n\t" // q3 = V1 + M3_1 * V3YYYY
			"vadd.f32 q6, q5, q6				\n\t" // q6 = V2
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M3_2 * V3ZZZZ
			"vmla.f32 q3, q15, d1[1]			\n\t" // q3 = q3 + M3_3 * V3WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M1_0 * N1XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vadd.f32 q1, q1, q6				\n\t" // q1 = V2 + V3
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load N2XYZW
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M1_1 * N1YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M1_2 * N1ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1 + V2 + V3
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q6, q12, d0[0]			\n\t" // q6 = N1 + M2_0 * N2XXXX
			LOAD_BONE_PTR
			"vst1.32 {q3}, [%[o], :128]!		\n\t" // store VXYZW
			"vmla.f32 q5, q13, d0[1]			\n\t" // q5 = N1 + M2_1 * N2YYYY
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N3XYZW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M2_3 * N2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vadd.f32 q6, q5, q6				\n\t" // q6 = N1 + N2
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load T1XYZW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M3_0 * N3XXXX
			// stall q6
			"vadd.f32 q3, q3, q6				\n\t" // q3 = N1 + N2
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M3_1 * N3YYYY
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			// stall q5
			"vadd.f32 q3, q3, q5				\n\t" // q3 = N1 + N2 + N3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vmla.f32 q6, q14, d9[0]			\n\t" // q6 = q6 + M3_3 * N3ZZZZ
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone1 matrix rows 2 + 3
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load T2XYZW
			LOAD_BONE_PTR
			"vadd.f32 q6, q3, q6				\n\t" // q6 = N1 + N2 + N3
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q2, q12, d8[0]			\n\t" // q2 = q2 + M2_0 * T2XXXX
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store NXYZW
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * T2YYYY
			"vld1.32 {q7}, [%[v], :128]!		\n\t" // load T3XYZW
			"vmla.f32 q1, q14, d9[0]			\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			"vadd.f32 q6, q2, q6				\n\t" // q6 = T1 + T2
			LOAD_BONE_PTR
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vmla.f32 q1, q12, d14[0]			\n\t" // q1 = q1 + M3_0 * TXXXX
			"vmla.f32 q6, q13, d14[1]			\n\t" // q6 = q2 + M3_1 * TYYYY
			
			// VERTEX 4
			
			STORE_BONE_OFS
			
			LOAD_BONE_PTR
			"vld1.32 {q0}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d15[0]			\n\t" // q1 = q1 + M3_2 * TZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vadd.f32 q6, q1, q6				\n\t" // q6 = T1 + T2 + T3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * V1XXXX
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load V2XYZW
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * V1YYYY
			"vmov s27, s31						\n\t" // T.WWWW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M1_2 * V1ZZZZ
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store TXYZW
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q15, d1[1]			\n\t" // q1 = M1_TRANSLATE * V1WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M2_0 * V2XXXX
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load V3XYZW
			"vadd.f32 q3, q2, q3				\n\t" // q3 = V1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * V2YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M2_2 * V2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1
			"vmla.f32 q6, q15, d9[1]			\n\t" // q6 = q6 + M2_TRANSLATE * V2WWWW
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N1XYZW
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M3_0 * V3XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vmla.f32 q3, q13, d0[1]			\n\t" // q3 = V1 + M3_1 * V3YYYY
			"vadd.f32 q6, q5, q6				\n\t" // q6 = V2
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M3_2 * V3ZZZZ
			"vmla.f32 q3, q15, d1[1]			\n\t" // q3 = q3 + M3_3 * V3WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M1_0 * N1XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vadd.f32 q1, q1, q6				\n\t" // q1 = V2 + V3
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load N2XYZW
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M1_1 * N1YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M1_2 * N1ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1 + V2 + V3
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q6, q12, d0[0]			\n\t" // q6 = N1 + M2_0 * N2XXXX
			LOAD_BONE_PTR
			"vst1.32 {q3}, [%[o], :128]!		\n\t" // store VXYZW
			"vmla.f32 q5, q13, d0[1]			\n\t" // q5 = N1 + M2_1 * N2YYYY
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N3XYZW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M2_3 * N2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vadd.f32 q6, q5, q6				\n\t" // q6 = N1 + N2
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load T1XYZW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M3_0 * N3XXXX
			// stall q6
			"vadd.f32 q3, q3, q6				\n\t" // q3 = N1 + N2
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M3_1 * N3YYYY
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			// stall q5
			"vadd.f32 q3, q3, q5				\n\t" // q3 = N1 + N2 + N3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vmla.f32 q6, q14, d9[0]			\n\t" // q6 = q6 + M3_3 * N3ZZZZ
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone1 matrix rows 2 + 3
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load T2XYZW
			LOAD_BONE_PTR
			"vadd.f32 q6, q3, q6				\n\t" // q6 = N1 + N2 + N3
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q2, q12, d8[0]			\n\t" // q2 = q2 + M2_0 * T2XXXX
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store NXYZW
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * T2YYYY
			"vld1.32 {q7}, [%[v], :128]!		\n\t" // load T3XYZW
			"vmla.f32 q1, q14, d9[0]			\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			"vadd.f32 q6, q2, q6				\n\t" // q6 = T1 + T2
			LOAD_BONE_PTR
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vmla.f32 q1, q12, d14[0]			\n\t" // q1 = q1 + M3_0 * TXXXX
			"vmla.f32 q6, q13, d14[1]			\n\t" // q6 = q2 + M3_1 * TYYYY
			
			// stall q1
			"vmla.f32 q1, q14, d15[0]			\n\t" // q1 = q1 + M3_2 * TZZZZ
			// stall q1
			"vadd.f32 q6, q1, q6				\n\t" // q6 = T1 + T2 + T3
			// stall q1
			"vmov s27, s31						\n\t" // T.WWWW
			// stall q1
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store TXYZW
			
			STORE_BONE_OFS
								
			// loop
			"sub %[nv], %[nv], #4			\n\t"
			"cmp %[nv], #4					\n\t"
			"bge Lskin3_4					\n\t"
			
		: [o] "+r" (outVerts), [v] "+r" (vertices), [bi] "+r" (boneIndices), [nv] "+r" (numVerts)
		: [bones] "r" (bones)
		: "cc", "r9", "r10", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
		);
	}
	
	if (numVerts > 0) {
		
		asm volatile (
		
			LOAD_BONE_OFS
			
			"Lskin3_1:						\n\t"
						
			LOAD_BONE_PTR
			"vld1.32 {q0}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * V1XXXX
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load V2XYZW
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * V1YYYY
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M1_2 * V1ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q15, d1[1]			\n\t" // q1 = M1_TRANSLATE * V1WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M2_0 * V2XXXX
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load V3XYZW
			"vadd.f32 q3, q2, q3				\n\t" // q3 = V1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * V2YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M2_2 * V2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1
			"vmla.f32 q6, q15, d9[1]			\n\t" // q6 = q6 + M2_TRANSLATE * V2WWWW
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N1XYZW
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M3_0 * V3XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vmla.f32 q3, q13, d0[1]			\n\t" // q3 = V1 + M3_1 * V3YYYY
			"vadd.f32 q6, q5, q6				\n\t" // q6 = V2
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M3_2 * V3ZZZZ
			"vmla.f32 q3, q15, d1[1]			\n\t" // q3 = q3 + M3_3 * V3WWWW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M1_0 * N1XXXX
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vadd.f32 q1, q1, q6				\n\t" // q1 = V2 + V3
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load N2XYZW
			LOAD_BONE_PTR
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M1_1 * N1YYYY
			"vmla.f32 q5, q14, d9[0]			\n\t" // q5 = q5 + M1_2 * N1ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vadd.f32 q3, q1, q3				\n\t" // q3 = V1 + V2 + V3
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q6, q12, d0[0]			\n\t" // q6 = N1 + M2_0 * N2XXXX
			LOAD_BONE_PTR
			"vst1.32 {q3}, [%[o], :128]!		\n\t" // store VXYZW
			"vmla.f32 q5, q13, d0[1]			\n\t" // q5 = N1 + M2_1 * N2YYYY
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load N3XYZW
			"vmul.f32 q3, q14, d1[0]			\n\t" // q3 = M2_3 * N2ZZZZ
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			LOAD_BONE_OFS
			LOAD_BONE_PTR
			"vadd.f32 q6, q5, q6				\n\t" // q6 = N1 + N2
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load T1XYZW
			"vmul.f32 q5, q12, d8[0]			\n\t" // q5 = M3_0 * N3XXXX
			// stall q6
			"vadd.f32 q3, q3, q6				\n\t" // q3 = N1 + N2
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M3_1 * N3YYYY
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone1 matrix rows 0 + 1
			// stall q5
			"vadd.f32 q3, q3, q5				\n\t" // q3 = N1 + N2 + N3
			"vmul.f32 q1, q12, d0[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vmla.f32 q6, q14, d9[0]			\n\t" // q6 = q6 + M3_3 * N3ZZZZ
			"vmul.f32 q2, q13, d0[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone1 matrix rows 2 + 3
			"vld1.32 {q4}, [%[v], :128]!		\n\t" // load T2XYZW
			LOAD_BONE_PTR
			"vadd.f32 q6, q3, q6				\n\t" // q6 = N1 + N2 + N3
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone2 matrix rows 2 + 3
			"vmla.f32 q2, q12, d8[0]			\n\t" // q2 = q2 + M2_0 * T2XXXX
			"vst1.32 {q6}, [%[o], :128]!		\n\t" // store NXYZW
			"vmul.f32 q6, q13, d8[1]			\n\t" // q6 = M2_1 * T2YYYY
			"vld1.32 {q0}, [%[v], :128]!		\n\t" // load T3XYZW
			"vmla.f32 q1, q14, d9[0]			\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			"vadd.f32 q2, q2, q6				\n\t" // q2 = T1 + T2
			LOAD_BONE_PTR
			"vld1.32 {q12-q13}, [r9, :128]!		\n\t" // load bone3 matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!		\n\t" // load bone3 matrix rows 2 + 3
			"vmla.f32 q1, q12, d0[0]			\n\t" // q1 = q1 + M3_0 * TXXXX
			"vmla.f32 q2, q13, d0[1]			\n\t" // q2 = q2 + M3_1 * TYYYY
			// stall q1
			"vmla.f32 q1, q14, d1[0]			\n\t" // q1 = q1 + M3_2 * TZZZZ
			// stall q1
			"vadd.f32 q1, q1, q2				\n\t" // q1 = T1 + T2 + T3
			// stall q1
			"vmov s7, s3						\n\t" // T.WWWW
			// stall q1
			"vst1.32 {q1}, [%[o], :128]!		\n\t" // store TXYZW
			
			STORE_BONE_OFS
			
			// loop
			"subs %[nv], %[nv], #1			\n\t"
			"bne Lskin3_1					\n\t"
			
		: [o] "+r" (outVerts), [v] "+r" (vertices), [bi] "+r" (boneIndices), [nv] "+r" (numVerts)
		: [bones] "r" (bones)
		: "cc", "r9", "r10", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
		);
	}
}

void SkinVerts2B(
	float *outVerts, 
	const float *bones, 
	const float *vertices,
	const U16 *boneIndices,
	int numVerts
)
{
	RAD_ASSERT(IsAligned(outVerts, SIMDDriver::kAlignment));
	RAD_ASSERT(IsAligned(bones, SIMDDriver::kAlignment));
	RAD_ASSERT(IsAligned(vertices, SIMDDriver::kAlignment));
	RAD_ASSERT(IsAligned(boneIndices, SIMDDriver::kAlignment));
	
	if (numVerts >= 16) {
		
		asm volatile (
		
			LOAD_BONE_OFS
			
			"Lskin2_16:						\n\t"
						
			LOAD_BONE_PTR
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q8-q9}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vld1.32 {q10-q11}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			
			LOAD_BONE_PTR
			
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * V1XXXX
			"vmul.f32 q1, q11, d9[1]		\n\t" // DI q1 = M1_TRANSLATION * V1.WWWW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * V1YYYY
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load V2XYZW
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * V1ZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone2 matrix rows 2 + 3
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M2_0 * V2XXXX
			"vadd.f32 q6, q0, q1			\n\t" // q6 = V1
			"vmul.f32 q3, q15, d11[1]		\n\t" // DI q3 = M2_TRANSLATION * V2.WWWW
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load N1XYZW
			"vmla.f32 q2, q13, d10[1]		\n\t" // q2 = q2 + M2_3 * V2YYYY
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * N1XXXX
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_3 * V2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load N2XYZW
			"vadd.f32 q6, q6, q3			\n\t" // q6 = V1 + V2
			"vmul.f32 q3, q12, d10[0]		\n\t" // q3 = M2_0 * N2XXXX
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * N1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = V1 + V2
			"vmla.f32 q0, q10, d9[0]		\n\t" // q0 = q0 + M1_2 * N1ZZZZ
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load T1XYZW
			"vmla.f32 q3, q13, d10[1]		\n\t" // q3 = q3 + M2_1 * N2YYYY
			"vmul.f32 q1, q8, d8[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store VXYZW (q6 free)
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_2 * N2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load T2XYZW
			"vmul.f32 q2, q9, d8[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vadd.f32 q0, q0, q3			\n\t" // q0 = N1 + N2
			"vmla.f32 q2, q12, d10[0]		\n\t" // q2 = q3 + M2_0 * T2XXXX
			"vmul.f32 q6, q13, d10[1]		\n\t" // q6 = M2_1 * T2YYYY
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store NXYZW
			"vmla.f32 q1, q14, d11[0]		\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			
			// VERTEX 2
			
			LOAD_BONE_PTR
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q8-q9}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vld1.32 {q10-q11}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			
			LOAD_BONE_PTR
			
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * V1XXXX
			"vadd.f32 q6, q6, q1			\n\t" // q6 = q6 + q1
			"vmul.f32 q1, q11, d9[1]		\n\t" // DI q1 = M1_TRANSLATION * V1.WWWW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * V1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = q6 + q2
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * V1ZZZZ
			// stall q6
			"vmov s27, s23					\n\t" // store T.W
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load V2XYZW
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone2 matrix rows 2 + 3
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M2_0 * V2XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store TXYZW
			"vadd.f32 q6, q0, q1			\n\t" // q6 = V1
			"vmul.f32 q3, q15, d11[1]		\n\t" // DI q3 = M2_TRANSLATION * V2.WWWW
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load N1XYZW
			"vmla.f32 q2, q13, d10[1]		\n\t" // q2 = q2 + M2_3 * V2YYYY
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * N1XXXX
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_3 * V2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load N2XYZW
			"vadd.f32 q6, q6, q3			\n\t" // q6 = V1 + V2
			"vmul.f32 q3, q12, d10[0]		\n\t" // q3 = M2_0 * N2XXXX
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * N1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = V1 + V2
			"vmla.f32 q0, q10, d9[0]		\n\t" // q0 = q0 + M1_2 * N1ZZZZ
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load T1XYZW
			"vmla.f32 q3, q13, d10[1]		\n\t" // q3 = q3 + M2_1 * N2YYYY
			"vmul.f32 q1, q8, d8[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store VXYZW (q6 free)
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_2 * N2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load T2XYZW
			"vmul.f32 q2, q9, d8[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vadd.f32 q0, q0, q3			\n\t" // q0 = N1 + N2
			"vmla.f32 q2, q12, d10[0]		\n\t" // q2 = q3 + M2_0 * T2XXXX
			"vmul.f32 q6, q13, d10[1]		\n\t" // q6 = M2_1 * T2YYYY
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store NXYZW
			"vmla.f32 q1, q14, d11[0]		\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			
			// VERTEX 3
			
			LOAD_BONE_PTR
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q8-q9}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vld1.32 {q10-q11}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			
			LOAD_BONE_PTR
			
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * V1XXXX
			"vadd.f32 q6, q6, q1			\n\t" // q6 = q6 + q1
			"vmul.f32 q1, q11, d9[1]		\n\t" // DI q1 = M1_TRANSLATION * V1.WWWW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * V1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = q6 + q2
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * V1ZZZZ
			// stall q6
			"vmov s27, s23					\n\t" // store T.W
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load V2XYZW
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone2 matrix rows 2 + 3
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M2_0 * V2XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store TXYZW
			"vadd.f32 q6, q0, q1			\n\t" // q6 = V1
			"vmul.f32 q3, q15, d11[1]		\n\t" // DI q3 = M2_TRANSLATION * V2.WWWW
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load N1XYZW
			"vmla.f32 q2, q13, d10[1]		\n\t" // q2 = q2 + M2_3 * V2YYYY
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * N1XXXX
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_3 * V2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load N2XYZW
			"vadd.f32 q6, q6, q3			\n\t" // q6 = V1 + V2
			"vmul.f32 q3, q12, d10[0]		\n\t" // q3 = M2_0 * N2XXXX
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * N1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = V1 + V2
			"vmla.f32 q0, q10, d9[0]		\n\t" // q0 = q0 + M1_2 * N1ZZZZ
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load T1XYZW
			"vmla.f32 q3, q13, d10[1]		\n\t" // q3 = q3 + M2_1 * N2YYYY
			"vmul.f32 q1, q8, d8[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store VXYZW (q6 free)
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_2 * N2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load T2XYZW
			"vmul.f32 q2, q9, d8[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vadd.f32 q0, q0, q3			\n\t" // q0 = N1 + N2
			"vmla.f32 q2, q12, d10[0]		\n\t" // q2 = q3 + M2_0 * T2XXXX
			"vmul.f32 q6, q13, d10[1]		\n\t" // q6 = M2_1 * T2YYYY
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store NXYZW
			"vmla.f32 q1, q14, d11[0]		\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			
			// VERTEX 4
			
			LOAD_BONE_PTR
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q8-q9}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vld1.32 {q10-q11}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			
			LOAD_BONE_PTR
			
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * V1XXXX
			"vadd.f32 q6, q6, q1			\n\t" // q6 = q6 + q1
			"vmul.f32 q1, q11, d9[1]		\n\t" // DI q1 = M1_TRANSLATION * V1.WWWW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * V1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = q6 + q2
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * V1ZZZZ
			// stall q6
			"vmov s27, s23					\n\t" // store T.W
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load V2XYZW
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone2 matrix rows 2 + 3
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M2_0 * V2XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store TXYZW
			"vadd.f32 q6, q0, q1			\n\t" // q6 = V1
			"vmul.f32 q3, q15, d11[1]		\n\t" // DI q3 = M2_TRANSLATION * V2.WWWW
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load N1XYZW
			"vmla.f32 q2, q13, d10[1]		\n\t" // q2 = q2 + M2_3 * V2YYYY
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * N1XXXX
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_3 * V2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load N2XYZW
			"vadd.f32 q6, q6, q3			\n\t" // q6 = V1 + V2
			"vmul.f32 q3, q12, d10[0]		\n\t" // q3 = M2_0 * N2XXXX
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * N1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = V1 + V2
			"vmla.f32 q0, q10, d9[0]		\n\t" // q0 = q0 + M1_2 * N1ZZZZ
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load T1XYZW
			"vmla.f32 q3, q13, d10[1]		\n\t" // q3 = q3 + M2_1 * N2YYYY
			"vmul.f32 q1, q8, d8[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store VXYZW (q6 free)
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_2 * N2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load T2XYZW
			"vmul.f32 q2, q9, d8[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vadd.f32 q0, q0, q3			\n\t" // q0 = N1 + N2
			"vmla.f32 q2, q12, d10[0]		\n\t" // q2 = q3 + M2_0 * T2XXXX
			"vmul.f32 q6, q13, d10[1]		\n\t" // q6 = M2_1 * T2YYYY
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store NXYZW
			"vmla.f32 q1, q14, d11[0]		\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			
			// VERTEX 5
			
			LOAD_BONE_PTR
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q8-q9}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vld1.32 {q10-q11}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			
			LOAD_BONE_PTR
			
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * V1XXXX
			"vadd.f32 q6, q6, q1			\n\t" // q6 = q6 + q1
			"vmul.f32 q1, q11, d9[1]		\n\t" // DI q1 = M1_TRANSLATION * V1.WWWW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * V1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = q6 + q2
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * V1ZZZZ
			// stall q6
			"vmov s27, s23					\n\t" // store T.W
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load V2XYZW
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone2 matrix rows 2 + 3
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M2_0 * V2XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store TXYZW
			"vadd.f32 q6, q0, q1			\n\t" // q6 = V1
			"vmul.f32 q3, q15, d11[1]		\n\t" // DI q3 = M2_TRANSLATION * V2.WWWW
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load N1XYZW
			"vmla.f32 q2, q13, d10[1]		\n\t" // q2 = q2 + M2_3 * V2YYYY
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * N1XXXX
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_3 * V2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load N2XYZW
			"vadd.f32 q6, q6, q3			\n\t" // q6 = V1 + V2
			"vmul.f32 q3, q12, d10[0]		\n\t" // q3 = M2_0 * N2XXXX
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * N1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = V1 + V2
			"vmla.f32 q0, q10, d9[0]		\n\t" // q0 = q0 + M1_2 * N1ZZZZ
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load T1XYZW
			"vmla.f32 q3, q13, d10[1]		\n\t" // q3 = q3 + M2_1 * N2YYYY
			"vmul.f32 q1, q8, d8[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store VXYZW (q6 free)
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_2 * N2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load T2XYZW
			"vmul.f32 q2, q9, d8[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vadd.f32 q0, q0, q3			\n\t" // q0 = N1 + N2
			"vmla.f32 q2, q12, d10[0]		\n\t" // q2 = q3 + M2_0 * T2XXXX
			"vmul.f32 q6, q13, d10[1]		\n\t" // q6 = M2_1 * T2YYYY
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store NXYZW
			"vmla.f32 q1, q14, d11[0]		\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			
			// VERTEX 6
			
			LOAD_BONE_PTR
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q8-q9}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vld1.32 {q10-q11}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			
			LOAD_BONE_PTR
			
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * V1XXXX
			"vadd.f32 q6, q6, q1			\n\t" // q6 = q6 + q1
			"vmul.f32 q1, q11, d9[1]		\n\t" // DI q1 = M1_TRANSLATION * V1.WWWW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * V1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = q6 + q2
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * V1ZZZZ
			// stall q6
			"vmov s27, s23					\n\t" // store T.W
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load V2XYZW
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone2 matrix rows 2 + 3
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M2_0 * V2XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store TXYZW
			"vadd.f32 q6, q0, q1			\n\t" // q6 = V1
			"vmul.f32 q3, q15, d11[1]		\n\t" // DI q3 = M2_TRANSLATION * V2.WWWW
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load N1XYZW
			"vmla.f32 q2, q13, d10[1]		\n\t" // q2 = q2 + M2_3 * V2YYYY
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * N1XXXX
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_3 * V2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load N2XYZW
			"vadd.f32 q6, q6, q3			\n\t" // q6 = V1 + V2
			"vmul.f32 q3, q12, d10[0]		\n\t" // q3 = M2_0 * N2XXXX
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * N1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = V1 + V2
			"vmla.f32 q0, q10, d9[0]		\n\t" // q0 = q0 + M1_2 * N1ZZZZ
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load T1XYZW
			"vmla.f32 q3, q13, d10[1]		\n\t" // q3 = q3 + M2_1 * N2YYYY
			"vmul.f32 q1, q8, d8[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store VXYZW (q6 free)
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_2 * N2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load T2XYZW
			"vmul.f32 q2, q9, d8[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vadd.f32 q0, q0, q3			\n\t" // q0 = N1 + N2
			"vmla.f32 q2, q12, d10[0]		\n\t" // q2 = q3 + M2_0 * T2XXXX
			"vmul.f32 q6, q13, d10[1]		\n\t" // q6 = M2_1 * T2YYYY
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store NXYZW
			"vmla.f32 q1, q14, d11[0]		\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			
			// VERTEX 7
			
			LOAD_BONE_PTR
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q8-q9}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vld1.32 {q10-q11}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			
			LOAD_BONE_PTR
			
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * V1XXXX
			"vadd.f32 q6, q6, q1			\n\t" // q6 = q6 + q1
			"vmul.f32 q1, q11, d9[1]		\n\t" // DI q1 = M1_TRANSLATION * V1.WWWW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * V1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = q6 + q2
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * V1ZZZZ
			// stall q6
			"vmov s27, s23					\n\t" // store T.W
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load V2XYZW
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone2 matrix rows 2 + 3
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M2_0 * V2XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store TXYZW
			"vadd.f32 q6, q0, q1			\n\t" // q6 = V1
			"vmul.f32 q3, q15, d11[1]		\n\t" // DI q3 = M2_TRANSLATION * V2.WWWW
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load N1XYZW
			"vmla.f32 q2, q13, d10[1]		\n\t" // q2 = q2 + M2_3 * V2YYYY
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * N1XXXX
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_3 * V2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load N2XYZW
			"vadd.f32 q6, q6, q3			\n\t" // q6 = V1 + V2
			"vmul.f32 q3, q12, d10[0]		\n\t" // q3 = M2_0 * N2XXXX
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * N1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = V1 + V2
			"vmla.f32 q0, q10, d9[0]		\n\t" // q0 = q0 + M1_2 * N1ZZZZ
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load T1XYZW
			"vmla.f32 q3, q13, d10[1]		\n\t" // q3 = q3 + M2_1 * N2YYYY
			"vmul.f32 q1, q8, d8[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store VXYZW (q6 free)
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_2 * N2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load T2XYZW
			"vmul.f32 q2, q9, d8[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vadd.f32 q0, q0, q3			\n\t" // q0 = N1 + N2
			"vmla.f32 q2, q12, d10[0]		\n\t" // q2 = q3 + M2_0 * T2XXXX
			"vmul.f32 q6, q13, d10[1]		\n\t" // q6 = M2_1 * T2YYYY
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store NXYZW
			"vmla.f32 q1, q14, d11[0]		\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			
			// VERTEX 8
			
			LOAD_BONE_PTR
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q8-q9}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vld1.32 {q10-q11}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			
			LOAD_BONE_PTR
			
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * V1XXXX
			"vadd.f32 q6, q6, q1			\n\t" // q6 = q6 + q1
			"vmul.f32 q1, q11, d9[1]		\n\t" // DI q1 = M1_TRANSLATION * V1.WWWW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * V1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = q6 + q2
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * V1ZZZZ
			// stall q6
			"vmov s27, s23					\n\t" // store T.W
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load V2XYZW
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone2 matrix rows 2 + 3
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M2_0 * V2XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store TXYZW
			"vadd.f32 q6, q0, q1			\n\t" // q6 = V1
			"vmul.f32 q3, q15, d11[1]		\n\t" // DI q3 = M2_TRANSLATION * V2.WWWW
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load N1XYZW
			"vmla.f32 q2, q13, d10[1]		\n\t" // q2 = q2 + M2_3 * V2YYYY
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * N1XXXX
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_3 * V2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load N2XYZW
			"vadd.f32 q6, q6, q3			\n\t" // q6 = V1 + V2
			"vmul.f32 q3, q12, d10[0]		\n\t" // q3 = M2_0 * N2XXXX
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * N1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = V1 + V2
			"vmla.f32 q0, q10, d9[0]		\n\t" // q0 = q0 + M1_2 * N1ZZZZ
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load T1XYZW
			"vmla.f32 q3, q13, d10[1]		\n\t" // q3 = q3 + M2_1 * N2YYYY
			"vmul.f32 q1, q8, d8[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store VXYZW (q6 free)
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_2 * N2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load T2XYZW
			"vmul.f32 q2, q9, d8[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vadd.f32 q0, q0, q3			\n\t" // q0 = N1 + N2
			"vmla.f32 q2, q12, d10[0]		\n\t" // q2 = q3 + M2_0 * T2XXXX
			"vmul.f32 q6, q13, d10[1]		\n\t" // q6 = M2_1 * T2YYYY
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store NXYZW
			"vmla.f32 q1, q14, d11[0]		\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			
			// VERTEX 9
			
			LOAD_BONE_PTR
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q8-q9}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vld1.32 {q10-q11}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			
			LOAD_BONE_PTR
			
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * V1XXXX
			"vadd.f32 q6, q6, q1			\n\t" // q6 = q6 + q1
			"vmul.f32 q1, q11, d9[1]		\n\t" // DI q1 = M1_TRANSLATION * V1.WWWW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * V1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = q6 + q2
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * V1ZZZZ
			// stall q6
			"vmov s27, s23					\n\t" // store T.W
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load V2XYZW
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone2 matrix rows 2 + 3
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M2_0 * V2XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store TXYZW
			"vadd.f32 q6, q0, q1			\n\t" // q6 = V1
			"vmul.f32 q3, q15, d11[1]		\n\t" // DI q3 = M2_TRANSLATION * V2.WWWW
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load N1XYZW
			"vmla.f32 q2, q13, d10[1]		\n\t" // q2 = q2 + M2_3 * V2YYYY
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * N1XXXX
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_3 * V2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load N2XYZW
			"vadd.f32 q6, q6, q3			\n\t" // q6 = V1 + V2
			"vmul.f32 q3, q12, d10[0]		\n\t" // q3 = M2_0 * N2XXXX
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * N1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = V1 + V2
			"vmla.f32 q0, q10, d9[0]		\n\t" // q0 = q0 + M1_2 * N1ZZZZ
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load T1XYZW
			"vmla.f32 q3, q13, d10[1]		\n\t" // q3 = q3 + M2_1 * N2YYYY
			"vmul.f32 q1, q8, d8[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store VXYZW (q6 free)
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_2 * N2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load T2XYZW
			"vmul.f32 q2, q9, d8[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vadd.f32 q0, q0, q3			\n\t" // q0 = N1 + N2
			"vmla.f32 q2, q12, d10[0]		\n\t" // q2 = q3 + M2_0 * T2XXXX
			"vmul.f32 q6, q13, d10[1]		\n\t" // q6 = M2_1 * T2YYYY
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store NXYZW
			"vmla.f32 q1, q14, d11[0]		\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			
			// VERTEX 10
			
			LOAD_BONE_PTR
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q8-q9}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vld1.32 {q10-q11}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			
			LOAD_BONE_PTR
			
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * V1XXXX
			"vadd.f32 q6, q6, q1			\n\t" // q6 = q6 + q1
			"vmul.f32 q1, q11, d9[1]		\n\t" // DI q1 = M1_TRANSLATION * V1.WWWW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * V1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = q6 + q2
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * V1ZZZZ
			// stall q6
			"vmov s27, s23					\n\t" // store T.W
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load V2XYZW
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone2 matrix rows 2 + 3
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M2_0 * V2XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store TXYZW
			"vadd.f32 q6, q0, q1			\n\t" // q6 = V1
			"vmul.f32 q3, q15, d11[1]		\n\t" // DI q3 = M2_TRANSLATION * V2.WWWW
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load N1XYZW
			"vmla.f32 q2, q13, d10[1]		\n\t" // q2 = q2 + M2_3 * V2YYYY
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * N1XXXX
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_3 * V2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load N2XYZW
			"vadd.f32 q6, q6, q3			\n\t" // q6 = V1 + V2
			"vmul.f32 q3, q12, d10[0]		\n\t" // q3 = M2_0 * N2XXXX
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * N1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = V1 + V2
			"vmla.f32 q0, q10, d9[0]		\n\t" // q0 = q0 + M1_2 * N1ZZZZ
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load T1XYZW
			"vmla.f32 q3, q13, d10[1]		\n\t" // q3 = q3 + M2_1 * N2YYYY
			"vmul.f32 q1, q8, d8[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store VXYZW (q6 free)
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_2 * N2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load T2XYZW
			"vmul.f32 q2, q9, d8[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vadd.f32 q0, q0, q3			\n\t" // q0 = N1 + N2
			"vmla.f32 q2, q12, d10[0]		\n\t" // q2 = q3 + M2_0 * T2XXXX
			"vmul.f32 q6, q13, d10[1]		\n\t" // q6 = M2_1 * T2YYYY
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store NXYZW
			"vmla.f32 q1, q14, d11[0]		\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			
			// VERTEX 11
			
			LOAD_BONE_PTR
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q8-q9}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vld1.32 {q10-q11}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			
			LOAD_BONE_PTR
			
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * V1XXXX
			"vadd.f32 q6, q6, q1			\n\t" // q6 = q6 + q1
			"vmul.f32 q1, q11, d9[1]		\n\t" // DI q1 = M1_TRANSLATION * V1.WWWW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * V1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = q6 + q2
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * V1ZZZZ
			// stall q6
			"vmov s27, s23					\n\t" // store T.W
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load V2XYZW
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone2 matrix rows 2 + 3
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M2_0 * V2XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store TXYZW
			"vadd.f32 q6, q0, q1			\n\t" // q6 = V1
			"vmul.f32 q3, q15, d11[1]		\n\t" // DI q3 = M2_TRANSLATION * V2.WWWW
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load N1XYZW
			"vmla.f32 q2, q13, d10[1]		\n\t" // q2 = q2 + M2_3 * V2YYYY
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * N1XXXX
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_3 * V2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load N2XYZW
			"vadd.f32 q6, q6, q3			\n\t" // q6 = V1 + V2
			"vmul.f32 q3, q12, d10[0]		\n\t" // q3 = M2_0 * N2XXXX
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * N1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = V1 + V2
			"vmla.f32 q0, q10, d9[0]		\n\t" // q0 = q0 + M1_2 * N1ZZZZ
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load T1XYZW
			"vmla.f32 q3, q13, d10[1]		\n\t" // q3 = q3 + M2_1 * N2YYYY
			"vmul.f32 q1, q8, d8[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store VXYZW (q6 free)
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_2 * N2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load T2XYZW
			"vmul.f32 q2, q9, d8[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vadd.f32 q0, q0, q3			\n\t" // q0 = N1 + N2
			"vmla.f32 q2, q12, d10[0]		\n\t" // q2 = q3 + M2_0 * T2XXXX
			"vmul.f32 q6, q13, d10[1]		\n\t" // q6 = M2_1 * T2YYYY
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store NXYZW
			"vmla.f32 q1, q14, d11[0]		\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			
			// VERTEX 12
			
			LOAD_BONE_PTR
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q8-q9}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vld1.32 {q10-q11}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			
			LOAD_BONE_PTR
			
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * V1XXXX
			"vadd.f32 q6, q6, q1			\n\t" // q6 = q6 + q1
			"vmul.f32 q1, q11, d9[1]		\n\t" // DI q1 = M1_TRANSLATION * V1.WWWW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * V1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = q6 + q2
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * V1ZZZZ
			// stall q6
			"vmov s27, s23					\n\t" // store T.W
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load V2XYZW
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone2 matrix rows 2 + 3
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M2_0 * V2XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store TXYZW
			"vadd.f32 q6, q0, q1			\n\t" // q6 = V1
			"vmul.f32 q3, q15, d11[1]		\n\t" // DI q3 = M2_TRANSLATION * V2.WWWW
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load N1XYZW
			"vmla.f32 q2, q13, d10[1]		\n\t" // q2 = q2 + M2_3 * V2YYYY
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * N1XXXX
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_3 * V2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load N2XYZW
			"vadd.f32 q6, q6, q3			\n\t" // q6 = V1 + V2
			"vmul.f32 q3, q12, d10[0]		\n\t" // q3 = M2_0 * N2XXXX
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * N1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = V1 + V2
			"vmla.f32 q0, q10, d9[0]		\n\t" // q0 = q0 + M1_2 * N1ZZZZ
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load T1XYZW
			"vmla.f32 q3, q13, d10[1]		\n\t" // q3 = q3 + M2_1 * N2YYYY
			"vmul.f32 q1, q8, d8[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store VXYZW (q6 free)
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_2 * N2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load T2XYZW
			"vmul.f32 q2, q9, d8[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vadd.f32 q0, q0, q3			\n\t" // q0 = N1 + N2
			"vmla.f32 q2, q12, d10[0]		\n\t" // q2 = q3 + M2_0 * T2XXXX
			"vmul.f32 q6, q13, d10[1]		\n\t" // q6 = M2_1 * T2YYYY
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store NXYZW
			"vmla.f32 q1, q14, d11[0]		\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			
			// VERTEX 13
			
			LOAD_BONE_PTR
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q8-q9}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vld1.32 {q10-q11}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			
			LOAD_BONE_PTR
			
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * V1XXXX
			"vadd.f32 q6, q6, q1			\n\t" // q6 = q6 + q1
			"vmul.f32 q1, q11, d9[1]		\n\t" // DI q1 = M1_TRANSLATION * V1.WWWW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * V1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = q6 + q2
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * V1ZZZZ
			// stall q6
			"vmov s27, s23					\n\t" // store T.W
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load V2XYZW
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone2 matrix rows 2 + 3
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M2_0 * V2XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store TXYZW
			"vadd.f32 q6, q0, q1			\n\t" // q6 = V1
			"vmul.f32 q3, q15, d11[1]		\n\t" // DI q3 = M2_TRANSLATION * V2.WWWW
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load N1XYZW
			"vmla.f32 q2, q13, d10[1]		\n\t" // q2 = q2 + M2_3 * V2YYYY
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * N1XXXX
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_3 * V2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load N2XYZW
			"vadd.f32 q6, q6, q3			\n\t" // q6 = V1 + V2
			"vmul.f32 q3, q12, d10[0]		\n\t" // q3 = M2_0 * N2XXXX
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * N1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = V1 + V2
			"vmla.f32 q0, q10, d9[0]		\n\t" // q0 = q0 + M1_2 * N1ZZZZ
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load T1XYZW
			"vmla.f32 q3, q13, d10[1]		\n\t" // q3 = q3 + M2_1 * N2YYYY
			"vmul.f32 q1, q8, d8[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store VXYZW (q6 free)
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_2 * N2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load T2XYZW
			"vmul.f32 q2, q9, d8[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vadd.f32 q0, q0, q3			\n\t" // q0 = N1 + N2
			"vmla.f32 q2, q12, d10[0]		\n\t" // q2 = q3 + M2_0 * T2XXXX
			"vmul.f32 q6, q13, d10[1]		\n\t" // q6 = M2_1 * T2YYYY
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store NXYZW
			"vmla.f32 q1, q14, d11[0]		\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			
			// VERTEX 14
			
			LOAD_BONE_PTR
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q8-q9}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vld1.32 {q10-q11}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			
			LOAD_BONE_PTR
			
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * V1XXXX
			"vadd.f32 q6, q6, q1			\n\t" // q6 = q6 + q1
			"vmul.f32 q1, q11, d9[1]		\n\t" // DI q1 = M1_TRANSLATION * V1.WWWW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * V1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = q6 + q2
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * V1ZZZZ
			// stall q6
			"vmov s27, s23					\n\t" // store T.W
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load V2XYZW
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone2 matrix rows 2 + 3
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M2_0 * V2XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store TXYZW
			"vadd.f32 q6, q0, q1			\n\t" // q6 = V1
			"vmul.f32 q3, q15, d11[1]		\n\t" // DI q3 = M2_TRANSLATION * V2.WWWW
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load N1XYZW
			"vmla.f32 q2, q13, d10[1]		\n\t" // q2 = q2 + M2_3 * V2YYYY
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * N1XXXX
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_3 * V2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load N2XYZW
			"vadd.f32 q6, q6, q3			\n\t" // q6 = V1 + V2
			"vmul.f32 q3, q12, d10[0]		\n\t" // q3 = M2_0 * N2XXXX
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * N1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = V1 + V2
			"vmla.f32 q0, q10, d9[0]		\n\t" // q0 = q0 + M1_2 * N1ZZZZ
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load T1XYZW
			"vmla.f32 q3, q13, d10[1]		\n\t" // q3 = q3 + M2_1 * N2YYYY
			"vmul.f32 q1, q8, d8[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store VXYZW (q6 free)
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_2 * N2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load T2XYZW
			"vmul.f32 q2, q9, d8[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vadd.f32 q0, q0, q3			\n\t" // q0 = N1 + N2
			"vmla.f32 q2, q12, d10[0]		\n\t" // q2 = q3 + M2_0 * T2XXXX
			"vmul.f32 q6, q13, d10[1]		\n\t" // q6 = M2_1 * T2YYYY
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store NXYZW
			"vmla.f32 q1, q14, d11[0]		\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			
			// VERTEX 15
			
			LOAD_BONE_PTR
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q8-q9}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vld1.32 {q10-q11}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			
			LOAD_BONE_PTR
			
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * V1XXXX
			"vadd.f32 q6, q6, q1			\n\t" // q6 = q6 + q1
			"vmul.f32 q1, q11, d9[1]		\n\t" // DI q1 = M1_TRANSLATION * V1.WWWW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * V1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = q6 + q2
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * V1ZZZZ
			// stall q6
			"vmov s27, s23					\n\t" // store T.W
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load V2XYZW
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone2 matrix rows 2 + 3
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M2_0 * V2XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store TXYZW
			"vadd.f32 q6, q0, q1			\n\t" // q6 = V1
			"vmul.f32 q3, q15, d11[1]		\n\t" // DI q3 = M2_TRANSLATION * V2.WWWW
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load N1XYZW
			"vmla.f32 q2, q13, d10[1]		\n\t" // q2 = q2 + M2_3 * V2YYYY
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * N1XXXX
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_3 * V2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load N2XYZW
			"vadd.f32 q6, q6, q3			\n\t" // q6 = V1 + V2
			"vmul.f32 q3, q12, d10[0]		\n\t" // q3 = M2_0 * N2XXXX
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * N1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = V1 + V2
			"vmla.f32 q0, q10, d9[0]		\n\t" // q0 = q0 + M1_2 * N1ZZZZ
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load T1XYZW
			"vmla.f32 q3, q13, d10[1]		\n\t" // q3 = q3 + M2_1 * N2YYYY
			"vmul.f32 q1, q8, d8[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store VXYZW (q6 free)
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_2 * N2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load T2XYZW
			"vmul.f32 q2, q9, d8[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vadd.f32 q0, q0, q3			\n\t" // q0 = N1 + N2
			"vmla.f32 q2, q12, d10[0]		\n\t" // q2 = q3 + M2_0 * T2XXXX
			"vmul.f32 q6, q13, d10[1]		\n\t" // q6 = M2_1 * T2YYYY
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store NXYZW
			"vmla.f32 q1, q14, d11[0]		\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			
			// VERTEX 16
			
			LOAD_BONE_PTR
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q8-q9}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vld1.32 {q10-q11}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			
			LOAD_BONE_PTR
			
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * V1XXXX
			"vadd.f32 q6, q6, q1			\n\t" // q6 = q6 + q1
			"vmul.f32 q1, q11, d9[1]		\n\t" // DI q1 = M1_TRANSLATION * V1.WWWW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * V1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = q6 + q2
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * V1ZZZZ
			// stall q6
			"vmov s27, s23					\n\t" // store T.W
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load V2XYZW
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone2 matrix rows 2 + 3
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M2_0 * V2XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store TXYZW
			"vadd.f32 q6, q0, q1			\n\t" // q6 = V1
			"vmul.f32 q3, q15, d11[1]		\n\t" // DI q3 = M2_TRANSLATION * V2.WWWW
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load N1XYZW
			"vmla.f32 q2, q13, d10[1]		\n\t" // q2 = q2 + M2_3 * V2YYYY
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * N1XXXX
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_3 * V2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load N2XYZW
			"vadd.f32 q6, q6, q3			\n\t" // q6 = V1 + V2
			"vmul.f32 q3, q12, d10[0]		\n\t" // q3 = M2_0 * N2XXXX
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * N1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = V1 + V2
			"vmla.f32 q0, q10, d9[0]		\n\t" // q0 = q0 + M1_2 * N1ZZZZ
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load T1XYZW
			"vmla.f32 q3, q13, d10[1]		\n\t" // q3 = q3 + M2_1 * N2YYYY
			"vmul.f32 q1, q8, d8[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store VXYZW (q6 free)
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_2 * N2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load T2XYZW
			"vmul.f32 q2, q9, d8[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vadd.f32 q0, q0, q3			\n\t" // q0 = N1 + N2
			"vmla.f32 q2, q12, d10[0]		\n\t" // q2 = q3 + M2_0 * T2XXXX
			"vmul.f32 q6, q13, d10[1]		\n\t" // q6 = M2_1 * T2YYYY
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store NXYZW
			"vmla.f32 q1, q14, d11[0]		\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			
			// stall q6
			"vadd.f32 q6, q6, q2			\n\t" // q6 = q6 + q2
			// stall q6
			"vadd.f32 q6, q6, q1			\n\t" // q6 = q6 + q1
			// stall q6
			"vmov s27, s23					\n\t" // store T.W
			// stall q6
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store TXYZW
			
			// loop
			"sub %[nv], %[nv], #16			\n\t"
			"cmp %[nv], #16					\n\t"
			"bge Lskin2_16					\n\t"
			STORE_BONE_OFS
			
		: [o] "+r" (outVerts), [v] "+r" (vertices), [bi] "+r" (boneIndices), [nv] "+r" (numVerts)
		: [bones] "r" (bones)
		: "cc", "r9", "r10", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
		);
	}
	
	if (numVerts >= 8) {
		
		asm volatile (
		
			LOAD_BONE_OFS
			
			"Lskin2_8:						\n\t"
						
			LOAD_BONE_PTR
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q8-q9}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vld1.32 {q10-q11}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			
			LOAD_BONE_PTR
			
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * V1XXXX
			"vmul.f32 q1, q11, d9[1]		\n\t" // DI q1 = M1_TRANSLATION * V1.WWWW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * V1YYYY
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load V2XYZW
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * V1ZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone2 matrix rows 2 + 3
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M2_0 * V2XXXX
			"vadd.f32 q6, q0, q1			\n\t" // q6 = V1
			"vmul.f32 q3, q15, d11[1]		\n\t" // DI q3 = M2_TRANSLATION * V2.WWWW
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load N1XYZW
			"vmla.f32 q2, q13, d10[1]		\n\t" // q2 = q2 + M2_3 * V2YYYY
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * N1XXXX
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_3 * V2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load N2XYZW
			"vadd.f32 q6, q6, q3			\n\t" // q6 = V1 + V2
			"vmul.f32 q3, q12, d10[0]		\n\t" // q3 = M2_0 * N2XXXX
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * N1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = V1 + V2
			"vmla.f32 q0, q10, d9[0]		\n\t" // q0 = q0 + M1_2 * N1ZZZZ
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load T1XYZW
			"vmla.f32 q3, q13, d10[1]		\n\t" // q3 = q3 + M2_1 * N2YYYY
			"vmul.f32 q1, q8, d8[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store VXYZW (q6 free)
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_2 * N2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load T2XYZW
			"vmul.f32 q2, q9, d8[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vadd.f32 q0, q0, q3			\n\t" // q0 = N1 + N2
			"vmla.f32 q2, q12, d10[0]		\n\t" // q2 = q3 + M2_0 * T2XXXX
			"vmul.f32 q6, q13, d10[1]		\n\t" // q6 = M2_1 * T2YYYY
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store NXYZW
			"vmla.f32 q1, q14, d11[0]		\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			
			// VERTEX 2
			
			LOAD_BONE_PTR
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q8-q9}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vld1.32 {q10-q11}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			
			LOAD_BONE_PTR
			
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * V1XXXX
			"vadd.f32 q6, q6, q1			\n\t" // q6 = q6 + q1
			"vmul.f32 q1, q11, d9[1]		\n\t" // DI q1 = M1_TRANSLATION * V1.WWWW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * V1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = q6 + q2
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * V1ZZZZ
			// stall q6
			"vmov s27, s23					\n\t" // store T.W
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load V2XYZW
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone2 matrix rows 2 + 3
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M2_0 * V2XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store TXYZW
			"vadd.f32 q6, q0, q1			\n\t" // q6 = V1
			"vmul.f32 q3, q15, d11[1]		\n\t" // DI q3 = M2_TRANSLATION * V2.WWWW
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load N1XYZW
			"vmla.f32 q2, q13, d10[1]		\n\t" // q2 = q2 + M2_3 * V2YYYY
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * N1XXXX
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_3 * V2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load N2XYZW
			"vadd.f32 q6, q6, q3			\n\t" // q6 = V1 + V2
			"vmul.f32 q3, q12, d10[0]		\n\t" // q3 = M2_0 * N2XXXX
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * N1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = V1 + V2
			"vmla.f32 q0, q10, d9[0]		\n\t" // q0 = q0 + M1_2 * N1ZZZZ
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load T1XYZW
			"vmla.f32 q3, q13, d10[1]		\n\t" // q3 = q3 + M2_1 * N2YYYY
			"vmul.f32 q1, q8, d8[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store VXYZW (q6 free)
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_2 * N2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load T2XYZW
			"vmul.f32 q2, q9, d8[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vadd.f32 q0, q0, q3			\n\t" // q0 = N1 + N2
			"vmla.f32 q2, q12, d10[0]		\n\t" // q2 = q3 + M2_0 * T2XXXX
			"vmul.f32 q6, q13, d10[1]		\n\t" // q6 = M2_1 * T2YYYY
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store NXYZW
			"vmla.f32 q1, q14, d11[0]		\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			
			// VERTEX 3
			
			LOAD_BONE_PTR
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q8-q9}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vld1.32 {q10-q11}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			
			LOAD_BONE_PTR
			
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * V1XXXX
			"vadd.f32 q6, q6, q1			\n\t" // q6 = q6 + q1
			"vmul.f32 q1, q11, d9[1]		\n\t" // DI q1 = M1_TRANSLATION * V1.WWWW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * V1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = q6 + q2
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * V1ZZZZ
			// stall q6
			"vmov s27, s23					\n\t" // store T.W
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load V2XYZW
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone2 matrix rows 2 + 3
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M2_0 * V2XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store TXYZW
			"vadd.f32 q6, q0, q1			\n\t" // q6 = V1
			"vmul.f32 q3, q15, d11[1]		\n\t" // DI q3 = M2_TRANSLATION * V2.WWWW
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load N1XYZW
			"vmla.f32 q2, q13, d10[1]		\n\t" // q2 = q2 + M2_3 * V2YYYY
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * N1XXXX
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_3 * V2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load N2XYZW
			"vadd.f32 q6, q6, q3			\n\t" // q6 = V1 + V2
			"vmul.f32 q3, q12, d10[0]		\n\t" // q3 = M2_0 * N2XXXX
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * N1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = V1 + V2
			"vmla.f32 q0, q10, d9[0]		\n\t" // q0 = q0 + M1_2 * N1ZZZZ
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load T1XYZW
			"vmla.f32 q3, q13, d10[1]		\n\t" // q3 = q3 + M2_1 * N2YYYY
			"vmul.f32 q1, q8, d8[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store VXYZW (q6 free)
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_2 * N2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load T2XYZW
			"vmul.f32 q2, q9, d8[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vadd.f32 q0, q0, q3			\n\t" // q0 = N1 + N2
			"vmla.f32 q2, q12, d10[0]		\n\t" // q2 = q3 + M2_0 * T2XXXX
			"vmul.f32 q6, q13, d10[1]		\n\t" // q6 = M2_1 * T2YYYY
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store NXYZW
			"vmla.f32 q1, q14, d11[0]		\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			
			// VERTEX 4
			
			LOAD_BONE_PTR
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q8-q9}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vld1.32 {q10-q11}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			
			LOAD_BONE_PTR
			
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * V1XXXX
			"vadd.f32 q6, q6, q1			\n\t" // q6 = q6 + q1
			"vmul.f32 q1, q11, d9[1]		\n\t" // DI q1 = M1_TRANSLATION * V1.WWWW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * V1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = q6 + q2
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * V1ZZZZ
			// stall q6
			"vmov s27, s23					\n\t" // store T.W
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load V2XYZW
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone2 matrix rows 2 + 3
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M2_0 * V2XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store TXYZW
			"vadd.f32 q6, q0, q1			\n\t" // q6 = V1
			"vmul.f32 q3, q15, d11[1]		\n\t" // DI q3 = M2_TRANSLATION * V2.WWWW
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load N1XYZW
			"vmla.f32 q2, q13, d10[1]		\n\t" // q2 = q2 + M2_3 * V2YYYY
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * N1XXXX
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_3 * V2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load N2XYZW
			"vadd.f32 q6, q6, q3			\n\t" // q6 = V1 + V2
			"vmul.f32 q3, q12, d10[0]		\n\t" // q3 = M2_0 * N2XXXX
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * N1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = V1 + V2
			"vmla.f32 q0, q10, d9[0]		\n\t" // q0 = q0 + M1_2 * N1ZZZZ
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load T1XYZW
			"vmla.f32 q3, q13, d10[1]		\n\t" // q3 = q3 + M2_1 * N2YYYY
			"vmul.f32 q1, q8, d8[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store VXYZW (q6 free)
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_2 * N2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load T2XYZW
			"vmul.f32 q2, q9, d8[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vadd.f32 q0, q0, q3			\n\t" // q0 = N1 + N2
			"vmla.f32 q2, q12, d10[0]		\n\t" // q2 = q3 + M2_0 * T2XXXX
			"vmul.f32 q6, q13, d10[1]		\n\t" // q6 = M2_1 * T2YYYY
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store NXYZW
			"vmla.f32 q1, q14, d11[0]		\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			
			// VERTEX 5
			
			LOAD_BONE_PTR
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q8-q9}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vld1.32 {q10-q11}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			
			LOAD_BONE_PTR
			
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * V1XXXX
			"vadd.f32 q6, q6, q1			\n\t" // q6 = q6 + q1
			"vmul.f32 q1, q11, d9[1]		\n\t" // DI q1 = M1_TRANSLATION * V1.WWWW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * V1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = q6 + q2
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * V1ZZZZ
			// stall q6
			"vmov s27, s23					\n\t" // store T.W
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load V2XYZW
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone2 matrix rows 2 + 3
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M2_0 * V2XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store TXYZW
			"vadd.f32 q6, q0, q1			\n\t" // q6 = V1
			"vmul.f32 q3, q15, d11[1]		\n\t" // DI q3 = M2_TRANSLATION * V2.WWWW
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load N1XYZW
			"vmla.f32 q2, q13, d10[1]		\n\t" // q2 = q2 + M2_3 * V2YYYY
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * N1XXXX
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_3 * V2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load N2XYZW
			"vadd.f32 q6, q6, q3			\n\t" // q6 = V1 + V2
			"vmul.f32 q3, q12, d10[0]		\n\t" // q3 = M2_0 * N2XXXX
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * N1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = V1 + V2
			"vmla.f32 q0, q10, d9[0]		\n\t" // q0 = q0 + M1_2 * N1ZZZZ
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load T1XYZW
			"vmla.f32 q3, q13, d10[1]		\n\t" // q3 = q3 + M2_1 * N2YYYY
			"vmul.f32 q1, q8, d8[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store VXYZW (q6 free)
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_2 * N2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load T2XYZW
			"vmul.f32 q2, q9, d8[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vadd.f32 q0, q0, q3			\n\t" // q0 = N1 + N2
			"vmla.f32 q2, q12, d10[0]		\n\t" // q2 = q3 + M2_0 * T2XXXX
			"vmul.f32 q6, q13, d10[1]		\n\t" // q6 = M2_1 * T2YYYY
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store NXYZW
			"vmla.f32 q1, q14, d11[0]		\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			
			// VERTEX 6
			
			LOAD_BONE_PTR
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q8-q9}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vld1.32 {q10-q11}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			
			LOAD_BONE_PTR
			
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * V1XXXX
			"vadd.f32 q6, q6, q1			\n\t" // q6 = q6 + q1
			"vmul.f32 q1, q11, d9[1]		\n\t" // DI q1 = M1_TRANSLATION * V1.WWWW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * V1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = q6 + q2
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * V1ZZZZ
			// stall q6
			"vmov s27, s23					\n\t" // store T.W
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load V2XYZW
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone2 matrix rows 2 + 3
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M2_0 * V2XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store TXYZW
			"vadd.f32 q6, q0, q1			\n\t" // q6 = V1
			"vmul.f32 q3, q15, d11[1]		\n\t" // DI q3 = M2_TRANSLATION * V2.WWWW
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load N1XYZW
			"vmla.f32 q2, q13, d10[1]		\n\t" // q2 = q2 + M2_3 * V2YYYY
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * N1XXXX
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_3 * V2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load N2XYZW
			"vadd.f32 q6, q6, q3			\n\t" // q6 = V1 + V2
			"vmul.f32 q3, q12, d10[0]		\n\t" // q3 = M2_0 * N2XXXX
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * N1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = V1 + V2
			"vmla.f32 q0, q10, d9[0]		\n\t" // q0 = q0 + M1_2 * N1ZZZZ
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load T1XYZW
			"vmla.f32 q3, q13, d10[1]		\n\t" // q3 = q3 + M2_1 * N2YYYY
			"vmul.f32 q1, q8, d8[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store VXYZW (q6 free)
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_2 * N2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load T2XYZW
			"vmul.f32 q2, q9, d8[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vadd.f32 q0, q0, q3			\n\t" // q0 = N1 + N2
			"vmla.f32 q2, q12, d10[0]		\n\t" // q2 = q3 + M2_0 * T2XXXX
			"vmul.f32 q6, q13, d10[1]		\n\t" // q6 = M2_1 * T2YYYY
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store NXYZW
			"vmla.f32 q1, q14, d11[0]		\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			
			// VERTEX 7
			
			LOAD_BONE_PTR
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q8-q9}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vld1.32 {q10-q11}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			
			LOAD_BONE_PTR
			
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * V1XXXX
			"vadd.f32 q6, q6, q1			\n\t" // q6 = q6 + q1
			"vmul.f32 q1, q11, d9[1]		\n\t" // DI q1 = M1_TRANSLATION * V1.WWWW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * V1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = q6 + q2
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * V1ZZZZ
			// stall q6
			"vmov s27, s23					\n\t" // store T.W
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load V2XYZW
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone2 matrix rows 2 + 3
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M2_0 * V2XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store TXYZW
			"vadd.f32 q6, q0, q1			\n\t" // q6 = V1
			"vmul.f32 q3, q15, d11[1]		\n\t" // DI q3 = M2_TRANSLATION * V2.WWWW
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load N1XYZW
			"vmla.f32 q2, q13, d10[1]		\n\t" // q2 = q2 + M2_3 * V2YYYY
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * N1XXXX
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_3 * V2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load N2XYZW
			"vadd.f32 q6, q6, q3			\n\t" // q6 = V1 + V2
			"vmul.f32 q3, q12, d10[0]		\n\t" // q3 = M2_0 * N2XXXX
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * N1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = V1 + V2
			"vmla.f32 q0, q10, d9[0]		\n\t" // q0 = q0 + M1_2 * N1ZZZZ
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load T1XYZW
			"vmla.f32 q3, q13, d10[1]		\n\t" // q3 = q3 + M2_1 * N2YYYY
			"vmul.f32 q1, q8, d8[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store VXYZW (q6 free)
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_2 * N2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load T2XYZW
			"vmul.f32 q2, q9, d8[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vadd.f32 q0, q0, q3			\n\t" // q0 = N1 + N2
			"vmla.f32 q2, q12, d10[0]		\n\t" // q2 = q3 + M2_0 * T2XXXX
			"vmul.f32 q6, q13, d10[1]		\n\t" // q6 = M2_1 * T2YYYY
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store NXYZW
			"vmla.f32 q1, q14, d11[0]		\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			
			// VERTEX 8
			
			LOAD_BONE_PTR
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q8-q9}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vld1.32 {q10-q11}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			
			LOAD_BONE_PTR
			
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * V1XXXX
			"vadd.f32 q6, q6, q1			\n\t" // q6 = q6 + q1
			"vmul.f32 q1, q11, d9[1]		\n\t" // DI q1 = M1_TRANSLATION * V1.WWWW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * V1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = q6 + q2
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * V1ZZZZ
			// stall q6
			"vmov s27, s23					\n\t" // store T.W
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load V2XYZW
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone2 matrix rows 2 + 3
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M2_0 * V2XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store TXYZW
			"vadd.f32 q6, q0, q1			\n\t" // q6 = V1
			"vmul.f32 q3, q15, d11[1]		\n\t" // DI q3 = M2_TRANSLATION * V2.WWWW
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load N1XYZW
			"vmla.f32 q2, q13, d10[1]		\n\t" // q2 = q2 + M2_3 * V2YYYY
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * N1XXXX
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_3 * V2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load N2XYZW
			"vadd.f32 q6, q6, q3			\n\t" // q6 = V1 + V2
			"vmul.f32 q3, q12, d10[0]		\n\t" // q3 = M2_0 * N2XXXX
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * N1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = V1 + V2
			"vmla.f32 q0, q10, d9[0]		\n\t" // q0 = q0 + M1_2 * N1ZZZZ
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load T1XYZW
			"vmla.f32 q3, q13, d10[1]		\n\t" // q3 = q3 + M2_1 * N2YYYY
			"vmul.f32 q1, q8, d8[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store VXYZW (q6 free)
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_2 * N2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load T2XYZW
			"vmul.f32 q2, q9, d8[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vadd.f32 q0, q0, q3			\n\t" // q0 = N1 + N2
			"vmla.f32 q2, q12, d10[0]		\n\t" // q2 = q3 + M2_0 * T2XXXX
			"vmul.f32 q6, q13, d10[1]		\n\t" // q6 = M2_1 * T2YYYY
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store NXYZW
			"vmla.f32 q1, q14, d11[0]		\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			
			// stall q6
			"vadd.f32 q6, q6, q2			\n\t" // q6 = q6 + q2
			// stall q6
			"vadd.f32 q6, q6, q1			\n\t" // q6 = q6 + q1
			// stall q6
			"vmov s27, s23					\n\t" // store T.W
			// stall q6
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store TXYZW
			
			// loop
			"sub %[nv], %[nv], #8			\n\t"
			"cmp %[nv], #8					\n\t"
			"bge Lskin2_8					\n\t"
			STORE_BONE_OFS
			
		: [o] "+r" (outVerts), [v] "+r" (vertices), [bi] "+r" (boneIndices), [nv] "+r" (numVerts)
		: [bones] "r" (bones)
		: "cc", "r9", "r10", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
		);
	}
	
	if (numVerts >= 4) {
		
		asm volatile (
		
			LOAD_BONE_OFS
			
			"Lskin2_4:						\n\t"
						
			LOAD_BONE_PTR
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q8-q9}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vld1.32 {q10-q11}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			
			LOAD_BONE_PTR
			
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * V1XXXX
			"vmul.f32 q1, q11, d9[1]		\n\t" // DI q1 = M1_TRANSLATION * V1.WWWW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * V1YYYY
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load V2XYZW
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * V1ZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone2 matrix rows 2 + 3
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M2_0 * V2XXXX
			"vadd.f32 q6, q0, q1			\n\t" // q6 = V1
			"vmul.f32 q3, q15, d11[1]		\n\t" // DI q3 = M2_TRANSLATION * V2.WWWW
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load N1XYZW
			"vmla.f32 q2, q13, d10[1]		\n\t" // q2 = q2 + M2_3 * V2YYYY
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * N1XXXX
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_3 * V2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load N2XYZW
			"vadd.f32 q6, q6, q3			\n\t" // q6 = V1 + V2
			"vmul.f32 q3, q12, d10[0]		\n\t" // q3 = M2_0 * N2XXXX
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * N1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = V1 + V2
			"vmla.f32 q0, q10, d9[0]		\n\t" // q0 = q0 + M1_2 * N1ZZZZ
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load T1XYZW
			"vmla.f32 q3, q13, d10[1]		\n\t" // q3 = q3 + M2_1 * N2YYYY
			"vmul.f32 q1, q8, d8[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store VXYZW (q6 free)
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_2 * N2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load T2XYZW
			"vmul.f32 q2, q9, d8[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vadd.f32 q0, q0, q3			\n\t" // q0 = N1 + N2
			"vmla.f32 q2, q12, d10[0]		\n\t" // q2 = q3 + M2_0 * T2XXXX
			"vmul.f32 q6, q13, d10[1]		\n\t" // q6 = M2_1 * T2YYYY
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store NXYZW
			"vmla.f32 q1, q14, d11[0]		\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			
			// VERTEX 2
			
			LOAD_BONE_PTR
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q8-q9}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vld1.32 {q10-q11}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			
			LOAD_BONE_PTR
			
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * V1XXXX
			"vadd.f32 q6, q6, q1			\n\t" // q6 = q6 + q1
			"vmul.f32 q1, q11, d9[1]		\n\t" // DI q1 = M1_TRANSLATION * V1.WWWW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * V1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = q6 + q2
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * V1ZZZZ
			// stall q6
			"vmov s27, s23					\n\t" // store T.W
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load V2XYZW
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone2 matrix rows 2 + 3
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M2_0 * V2XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store TXYZW
			"vadd.f32 q6, q0, q1			\n\t" // q6 = V1
			"vmul.f32 q3, q15, d11[1]		\n\t" // DI q3 = M2_TRANSLATION * V2.WWWW
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load N1XYZW
			"vmla.f32 q2, q13, d10[1]		\n\t" // q2 = q2 + M2_3 * V2YYYY
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * N1XXXX
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_3 * V2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load N2XYZW
			"vadd.f32 q6, q6, q3			\n\t" // q6 = V1 + V2
			"vmul.f32 q3, q12, d10[0]		\n\t" // q3 = M2_0 * N2XXXX
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * N1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = V1 + V2
			"vmla.f32 q0, q10, d9[0]		\n\t" // q0 = q0 + M1_2 * N1ZZZZ
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load T1XYZW
			"vmla.f32 q3, q13, d10[1]		\n\t" // q3 = q3 + M2_1 * N2YYYY
			"vmul.f32 q1, q8, d8[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store VXYZW (q6 free)
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_2 * N2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load T2XYZW
			"vmul.f32 q2, q9, d8[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vadd.f32 q0, q0, q3			\n\t" // q0 = N1 + N2
			"vmla.f32 q2, q12, d10[0]		\n\t" // q2 = q3 + M2_0 * T2XXXX
			"vmul.f32 q6, q13, d10[1]		\n\t" // q6 = M2_1 * T2YYYY
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store NXYZW
			"vmla.f32 q1, q14, d11[0]		\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			
			// VERTEX 3
			
			LOAD_BONE_PTR
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q8-q9}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vld1.32 {q10-q11}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			
			LOAD_BONE_PTR
			
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * V1XXXX
			"vadd.f32 q6, q6, q1			\n\t" // q6 = q6 + q1
			"vmul.f32 q1, q11, d9[1]		\n\t" // DI q1 = M1_TRANSLATION * V1.WWWW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * V1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = q6 + q2
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * V1ZZZZ
			// stall q6
			"vmov s27, s23					\n\t" // store T.W
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load V2XYZW
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone2 matrix rows 2 + 3
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M2_0 * V2XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store TXYZW
			"vadd.f32 q6, q0, q1			\n\t" // q6 = V1
			"vmul.f32 q3, q15, d11[1]		\n\t" // DI q3 = M2_TRANSLATION * V2.WWWW
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load N1XYZW
			"vmla.f32 q2, q13, d10[1]		\n\t" // q2 = q2 + M2_3 * V2YYYY
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * N1XXXX
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_3 * V2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load N2XYZW
			"vadd.f32 q6, q6, q3			\n\t" // q6 = V1 + V2
			"vmul.f32 q3, q12, d10[0]		\n\t" // q3 = M2_0 * N2XXXX
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * N1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = V1 + V2
			"vmla.f32 q0, q10, d9[0]		\n\t" // q0 = q0 + M1_2 * N1ZZZZ
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load T1XYZW
			"vmla.f32 q3, q13, d10[1]		\n\t" // q3 = q3 + M2_1 * N2YYYY
			"vmul.f32 q1, q8, d8[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store VXYZW (q6 free)
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_2 * N2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load T2XYZW
			"vmul.f32 q2, q9, d8[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vadd.f32 q0, q0, q3			\n\t" // q0 = N1 + N2
			"vmla.f32 q2, q12, d10[0]		\n\t" // q2 = q3 + M2_0 * T2XXXX
			"vmul.f32 q6, q13, d10[1]		\n\t" // q6 = M2_1 * T2YYYY
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store NXYZW
			"vmla.f32 q1, q14, d11[0]		\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			
			// VERTEX 4
			
			LOAD_BONE_PTR
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q8-q9}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vld1.32 {q10-q11}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			
			LOAD_BONE_PTR
			
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * V1XXXX
			"vadd.f32 q6, q6, q1			\n\t" // q6 = q6 + q1
			"vmul.f32 q1, q11, d9[1]		\n\t" // DI q1 = M1_TRANSLATION * V1.WWWW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * V1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = q6 + q2
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * V1ZZZZ
			// stall q6
			"vmov s27, s23					\n\t" // store T.W
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load V2XYZW
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone2 matrix rows 2 + 3
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M2_0 * V2XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store TXYZW
			"vadd.f32 q6, q0, q1			\n\t" // q6 = V1
			"vmul.f32 q3, q15, d11[1]		\n\t" // DI q3 = M2_TRANSLATION * V2.WWWW
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load N1XYZW
			"vmla.f32 q2, q13, d10[1]		\n\t" // q2 = q2 + M2_3 * V2YYYY
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * N1XXXX
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_3 * V2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load N2XYZW
			"vadd.f32 q6, q6, q3			\n\t" // q6 = V1 + V2
			"vmul.f32 q3, q12, d10[0]		\n\t" // q3 = M2_0 * N2XXXX
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * N1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = V1 + V2
			"vmla.f32 q0, q10, d9[0]		\n\t" // q0 = q0 + M1_2 * N1ZZZZ
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load T1XYZW
			"vmla.f32 q3, q13, d10[1]		\n\t" // q3 = q3 + M2_1 * N2YYYY
			"vmul.f32 q1, q8, d8[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store VXYZW (q6 free)
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_2 * N2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load T2XYZW
			"vmul.f32 q2, q9, d8[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vadd.f32 q0, q0, q3			\n\t" // q0 = N1 + N2
			"vmla.f32 q2, q12, d10[0]		\n\t" // q2 = q3 + M2_0 * T2XXXX
			"vmul.f32 q6, q13, d10[1]		\n\t" // q6 = M2_1 * T2YYYY
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store NXYZW
			"vmla.f32 q1, q14, d11[0]		\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			
			// stall q6
			"vadd.f32 q6, q6, q2			\n\t" // q6 = q6 + q2
			// stall q6
			"vadd.f32 q6, q6, q1			\n\t" // q6 = q6 + q1
			// stall q6
			"vmov s27, s23					\n\t" // store T.W
			// stall q6
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store TXYZW
			
			// loop
			"sub %[nv], %[nv], #4			\n\t"
			"cmp %[nv], #4					\n\t"
			"bge Lskin2_4					\n\t"
			STORE_BONE_OFS
			
		: [o] "+r" (outVerts), [v] "+r" (vertices), [bi] "+r" (boneIndices), [nv] "+r" (numVerts)
		: [bones] "r" (bones)
		: "cc", "r9", "r10", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
		);
	}

	if (numVerts > 0) {
		
		asm volatile (
		
			LOAD_BONE_OFS
			
			"Lskin2_1:						\n\t"
						
			LOAD_BONE_PTR
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load V1XYZW
			"vld1.32 {q8-q9}, [r9, :128]!	\n\t" // load bone1 matrix rows 0 + 1
			"vld1.32 {q10-q11}, [r9, :128]!	\n\t" // load bone1 matrix rows 2 + 3
			
			LOAD_BONE_PTR
			
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * V1XXXX
			"vmul.f32 q1, q11, d9[1]		\n\t" // DI q1 = M1_TRANSLATION * V1.WWWW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone2 matrix rows 0 + 1
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * V1YYYY
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load V2XYZW
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * V1ZZZZ
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone2 matrix rows 2 + 3
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M2_0 * V2XXXX
			"vadd.f32 q6, q0, q1			\n\t" // q6 = V1
			"vmul.f32 q3, q15, d11[1]		\n\t" // DI q3 = M2_TRANSLATION * V2.WWWW
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load N1XYZW
			"vmla.f32 q2, q13, d10[1]		\n\t" // q2 = q2 + M2_3 * V2YYYY
			"vmul.f32 q0, q8, d8[0]			\n\t" // q0 = M1_0 * N1XXXX
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_3 * V2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load N2XYZW
			"vadd.f32 q6, q6, q3			\n\t" // q6 = V1 + V2
			"vmul.f32 q3, q12, d10[0]		\n\t" // q3 = M2_0 * N2XXXX
			"vmla.f32 q0, q9, d8[1]			\n\t" // q0 = q0 + M1_1 * N1YYYY
			"vadd.f32 q6, q6, q2			\n\t" // q6 = V1 + V2
			"vmla.f32 q0, q10, d9[0]		\n\t" // q0 = q0 + M1_2 * N1ZZZZ
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load T1XYZW
			"vmla.f32 q3, q13, d10[1]		\n\t" // q3 = q3 + M2_1 * N2YYYY
			"vmul.f32 q1, q8, d8[0]			\n\t" // q1 = M1_0 * T1XXXX
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store VXYZW (q6 free)
			"vmla.f32 q3, q14, d11[0]		\n\t" // q3 = q3 + M2_2 * N2ZZZZ
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load T2XYZW
			"vmul.f32 q2, q9, d8[1]			\n\t" // q2 = M1_1 * T1YYYY
			"vmla.f32 q1, q10, d9[0]		\n\t" // q1 = q1 + M1_2 * T1ZZZZ
			"vadd.f32 q0, q0, q3			\n\t" // q0 = N1 + N2
			"vmla.f32 q2, q12, d10[0]		\n\t" // q2 = q3 + M2_0 * T2XXXX
			"vmul.f32 q6, q13, d10[1]		\n\t" // q6 = M2_1 * T2YYYY
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store NXYZW
			"vmla.f32 q1, q14, d11[0]		\n\t" // q1 = q1 + M2_2 * T2ZZZZ
			// stall q6
			"vadd.f32 q6, q6, q2			\n\t" // q6 = q6 + q2
			// stall q6
			"vadd.f32 q6, q6, q1			\n\t" // q6 = q6 + q1
			// stall q6
			"vmov s27, s23					\n\t" // store T.W
			// stall q6
			"vst1.32 {q6}, [%[o], :128]!	\n\t" // store TXYZW
			
			// loop
			"subs %[nv], %[nv], #1			\n\t"
			"bne Lskin2_1					\n\t"
			STORE_BONE_OFS
			
		: [o] "+r" (outVerts), [v] "+r" (vertices), [bi] "+r" (boneIndices), [nv] "+r" (numVerts)
		: [bones] "r" (bones)
		: "cc", "r9", "r10", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
		);
	}
}

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
	
	if (numVerts >= 16) {
		
		asm volatile (
			
			LOAD_BONE_OFS
			
			"Lskin1_16:						\n\t"
			
			// VERTEX 1
			
			LOAD_BONE_PTR
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load VXYZW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone matrix rows 2 + 3
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load NXYZW
			"vmul.f32 q0, q12, d8[0]		\n\t" // q0 = M0 * VXXXX
			"vmov q1, q15					\n\t" // DI q1 = M TRANSLATION
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M0 * NXXXX
			"vmla.f32 q1, q13, d8[1]		\n\t" // q1 = q1 + M1 * VYYYY
			"vmul.f32 q3, q13, d10[1]		\n\t" // q3 = M1 * NYYYY
			"vmla.f32 q0, q14, d9[0]		\n\t" // q0 = q0 + M2 * VZZZZ
			"vmla.f32 q2, q14, d11[0]		\n\t" // q2 = q2 + M2 * NZZZZ
			"vld1.32 {q6}, [%[v], :128]!	\n\t" // load TXYZW
			
			LOAD_BONE_PTR
			
			"vadd.f32 q0, q0, q1			\n\t" // q0 = q0 + q1
			"vmul.f32 q1, q13, d12[1]		\n\t" // q1 = M1 * TYYYY
			"vadd.f32 q2, q2, q3			\n\t" // q2 = q2 + q3
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store VXYZW
			"vmla.f32 q1, q14, d13[0]		\n\t" // q1 = q1 + M2 * TZZZZ
			"vmul.f32 q3, q12, d12[0]		\n\t" // q3 = M0 * TXXXX
			"vst1.32 {q2}, [%[o], :128]!	\n\t" // store NXYZW
			
			// VERTEX 2 (q3, q6 still busy)
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load VXYZW
			"vadd.f32 q3, q3, q1			\n\t" // q3 = q3 + q1
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone matrix rows 2 + 3
			"vmov s15, s27					\n\t" // store T.W
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load NXYZW
			"vst1.32 {q3}, [%[o], :128]!	\n\t" // store TXYZW
			// q3, q6 done
			"vmul.f32 q0, q12, d8[0]		\n\t" // q0 = M0 * VXXXX
			"vmov q1, q15					\n\t" // DI q1 = M TRANSLATION
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M0 * NXXXX
			"vmla.f32 q1, q13, d8[1]		\n\t" // q1 = q1 + M1 * VYYYY
			"vmul.f32 q3, q13, d10[1]		\n\t" // q3 = M1 * NYYYY
			"vmla.f32 q0, q14, d9[0]		\n\t" // q0 = q0 + M2 * VZZZZ
			"vmla.f32 q2, q14, d11[0]		\n\t" // q2 = q2 + M2 * NZZZZ
			"vld1.32 {q6}, [%[v], :128]!	\n\t" // load TXYZW
			
			LOAD_BONE_PTR
			
			"vadd.f32 q0, q0, q1			\n\t" // q0 = q0 + q1
			"vmul.f32 q1, q13, d12[1]		\n\t" // q1 = M1 * TYYYY
			"vadd.f32 q2, q2, q3			\n\t" // q2 = q2 + q3
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store VXYZW
			"vmla.f32 q1, q14, d13[0]		\n\t" // q1 = q1 + M2 * TZZZZ
			"vmul.f32 q3, q12, d12[0]		\n\t" // q3 = M0 * TXXXX
			"vst1.32 {q2}, [%[o], :128]!	\n\t" // store NXYZW
			
			// VERTEX 3 (q3, q6 still busy)
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load VXYZW
			"vadd.f32 q3, q3, q1			\n\t" // q3 = q3 + q1
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone matrix rows 2 + 3
			"vmov s15, s27					\n\t" // store T.W
			
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load NXYZW
			"vst1.32 {q3}, [%[o], :128]!	\n\t" // store TXYZW
			// q3, q6 done
			"vmul.f32 q0, q12, d8[0]		\n\t" // q0 = M0 * VXXXX
			"vmov q1, q15					\n\t" // DI q1 = M TRANSLATION
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M0 * NXXXX
			"vmla.f32 q1, q13, d8[1]		\n\t" // q1 = q1 + M1 * VYYYY
			"vmul.f32 q3, q13, d10[1]		\n\t" // q3 = M1 * NYYYY
			"vmla.f32 q0, q14, d9[0]		\n\t" // q0 = q0 + M2 * VZZZZ
			"vmla.f32 q2, q14, d11[0]		\n\t" // q2 = q2 + M2 * NZZZZ
			"vld1.32 {q6}, [%[v], :128]!	\n\t" // load TXYZW
			
			LOAD_BONE_PTR
			
			"vadd.f32 q0, q0, q1			\n\t" // q0 = q0 + q1
			"vmul.f32 q1, q13, d12[1]		\n\t" // q1 = M1 * TYYYY
			"vadd.f32 q2, q2, q3			\n\t" // q2 = q2 + q3
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store VXYZW
			"vmla.f32 q1, q14, d13[0]		\n\t" // q1 = q1 + M2 * TZZZZ
			"vmul.f32 q3, q12, d12[0]		\n\t" // q3 = M0 * TXXXX
			"vst1.32 {q2}, [%[o], :128]!	\n\t" // store NXYZW
			
			// VERTEX 4 (q3, q6 still busy)
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load VXYZW
			"vadd.f32 q3, q3, q1			\n\t" // q3 = q3 + q1
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone matrix rows 2 + 3
			"vmov s15, s27					\n\t" // store T.W
			
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load NXYZW
			"vst1.32 {q3}, [%[o], :128]!	\n\t" // store TXYZW
			// q3, q6 done
			"vmul.f32 q0, q12, d8[0]		\n\t" // q0 = M0 * VXXXX
			"vmov q1, q15					\n\t" // DI q1 = M TRANSLATION
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M0 * NXXXX
			"vmla.f32 q1, q13, d8[1]		\n\t" // q1 = q1 + M1 * VYYYY
			"vmul.f32 q3, q13, d10[1]		\n\t" // q3 = M1 * NYYYY
			"vmla.f32 q0, q14, d9[0]		\n\t" // q0 = q0 + M2 * VZZZZ
			"vmla.f32 q2, q14, d11[0]		\n\t" // q2 = q2 + M2 * NZZZZ
			"vld1.32 {q6}, [%[v], :128]!	\n\t" // load TXYZW
			
			LOAD_BONE_PTR
			
			"vadd.f32 q0, q0, q1			\n\t" // q0 = q0 + q1
			"vmul.f32 q1, q13, d12[1]		\n\t" // q1 = M1 * TYYYY
			"vadd.f32 q2, q2, q3			\n\t" // q2 = q2 + q3
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store VXYZW
			"vmla.f32 q1, q14, d13[0]		\n\t" // q1 = q1 + M2 * TZZZZ
			"vmul.f32 q3, q12, d12[0]		\n\t" // q3 = M0 * TXXXX
			"vst1.32 {q2}, [%[o], :128]!	\n\t" // store NXYZW
			
			// VERTEX 5 (q3, q6 still busy)
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load VXYZW
			"vadd.f32 q3, q3, q1			\n\t" // q3 = q3 + q1
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone matrix rows 2 + 3
			"vmov s15, s27					\n\t" // store T.W
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load NXYZW
			"vst1.32 {q3}, [%[o], :128]!	\n\t" // store TXYZW
			// q3, q6 done
			"vmul.f32 q0, q12, d8[0]		\n\t" // q0 = M0 * VXXXX
			"vmov q1, q15					\n\t" // DI q1 = M TRANSLATION
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M0 * NXXXX
			"vmla.f32 q1, q13, d8[1]		\n\t" // q1 = q1 + M1 * VYYYY
			"vmul.f32 q3, q13, d10[1]		\n\t" // q3 = M1 * NYYYY
			"vmla.f32 q0, q14, d9[0]		\n\t" // q0 = q0 + M2 * VZZZZ
			"vmla.f32 q2, q14, d11[0]		\n\t" // q2 = q2 + M2 * NZZZZ
			"vld1.32 {q6}, [%[v], :128]!	\n\t" // load TXYZW
			
			LOAD_BONE_PTR
			
			"vadd.f32 q0, q0, q1			\n\t" // q0 = q0 + q1
			"vmul.f32 q1, q13, d12[1]		\n\t" // q1 = M1 * TYYYY
			"vadd.f32 q2, q2, q3			\n\t" // q2 = q2 + q3
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store VXYZW
			"vmla.f32 q1, q14, d13[0]		\n\t" // q1 = q1 + M2 * TZZZZ
			"vmul.f32 q3, q12, d12[0]		\n\t" // q3 = M0 * TXXXX
			"vst1.32 {q2}, [%[o], :128]!	\n\t" // store NXYZW
			
			// VERTEX 6 (q3, q6 still busy)
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load VXYZW
			"vadd.f32 q3, q3, q1			\n\t" // q3 = q3 + q1
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone matrix rows 2 + 3
			"vmov s15, s27					\n\t" // store T.W
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load NXYZW
			"vst1.32 {q3}, [%[o], :128]!	\n\t" // store TXYZW
			// q3, q6 done
			"vmul.f32 q0, q12, d8[0]		\n\t" // q0 = M0 * VXXXX
			"vmov q1, q15					\n\t" // DI q1 = M TRANSLATION
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M0 * NXXXX
			"vmla.f32 q1, q13, d8[1]		\n\t" // q1 = q1 + M1 * VYYYY
			"vmul.f32 q3, q13, d10[1]		\n\t" // q3 = M1 * NYYYY
			"vmla.f32 q0, q14, d9[0]		\n\t" // q0 = q0 + M2 * VZZZZ
			"vmla.f32 q2, q14, d11[0]		\n\t" // q2 = q2 + M2 * NZZZZ
			"vld1.32 {q6}, [%[v], :128]!	\n\t" // load TXYZW
			
			LOAD_BONE_PTR
			
			"vadd.f32 q0, q0, q1			\n\t" // q0 = q0 + q1
			"vmul.f32 q1, q13, d12[1]		\n\t" // q1 = M1 * TYYYY
			"vadd.f32 q2, q2, q3			\n\t" // q2 = q2 + q3
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store VXYZW
			"vmla.f32 q1, q14, d13[0]		\n\t" // q1 = q1 + M2 * TZZZZ
			"vmul.f32 q3, q12, d12[0]		\n\t" // q3 = M0 * TXXXX
			"vst1.32 {q2}, [%[o], :128]!	\n\t" // store NXYZW
			
			// VERTEX 7 (q3, q6 still busy)
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load VXYZW
			"vadd.f32 q3, q3, q1			\n\t" // q3 = q3 + q1
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone matrix rows 2 + 3
			"vmov s15, s27					\n\t" // store T.W
			
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load NXYZW
			"vst1.32 {q3}, [%[o], :128]!	\n\t" // store TXYZW
			// q3, q6 done
			"vmul.f32 q0, q12, d8[0]		\n\t" // q0 = M0 * VXXXX
			"vmov q1, q15					\n\t" // DI q1 = M TRANSLATION
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M0 * NXXXX
			"vmla.f32 q1, q13, d8[1]		\n\t" // q1 = q1 + M1 * VYYYY
			"vmul.f32 q3, q13, d10[1]		\n\t" // q3 = M1 * NYYYY
			"vmla.f32 q0, q14, d9[0]		\n\t" // q0 = q0 + M2 * VZZZZ
			"vmla.f32 q2, q14, d11[0]		\n\t" // q2 = q2 + M2 * NZZZZ
			"vld1.32 {q6}, [%[v], :128]!	\n\t" // load TXYZW
			
			LOAD_BONE_PTR
			
			"vadd.f32 q0, q0, q1			\n\t" // q0 = q0 + q1
			"vmul.f32 q1, q13, d12[1]		\n\t" // q1 = M1 * TYYYY
			"vadd.f32 q2, q2, q3			\n\t" // q2 = q2 + q3
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store VXYZW
			"vmla.f32 q1, q14, d13[0]		\n\t" // q1 = q1 + M2 * TZZZZ
			"vmul.f32 q3, q12, d12[0]		\n\t" // q3 = M0 * TXXXX
			"vst1.32 {q2}, [%[o], :128]!	\n\t" // store NXYZW
			
			// VERTEX 8 (q3, q6 still busy)
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load VXYZW
			"vadd.f32 q3, q3, q1			\n\t" // q3 = q3 + q1
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone matrix rows 2 + 3
			"vmov s15, s27					\n\t" // store T.W
			
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load NXYZW
			"vst1.32 {q3}, [%[o], :128]!	\n\t" // store TXYZW
			// q3, q6 done
			"vmul.f32 q0, q12, d8[0]		\n\t" // q0 = M0 * VXXXX
			"vmov q1, q15					\n\t" // DI q1 = M TRANSLATION
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M0 * NXXXX
			"vmla.f32 q1, q13, d8[1]		\n\t" // q1 = q1 + M1 * VYYYY
			"vmul.f32 q3, q13, d10[1]		\n\t" // q3 = M1 * NYYYY
			"vmla.f32 q0, q14, d9[0]		\n\t" // q0 = q0 + M2 * VZZZZ
			"vmla.f32 q2, q14, d11[0]		\n\t" // q2 = q2 + M2 * NZZZZ
			"vld1.32 {q6}, [%[v], :128]!	\n\t" // load TXYZW
			
			LOAD_BONE_PTR
			
			"vadd.f32 q0, q0, q1			\n\t" // q0 = q0 + q1
			"vmul.f32 q1, q13, d12[1]		\n\t" // q1 = M1 * TYYYY
			"vadd.f32 q2, q2, q3			\n\t" // q2 = q2 + q3
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store VXYZW
			"vmla.f32 q1, q14, d13[0]		\n\t" // q1 = q1 + M2 * TZZZZ
			"vmul.f32 q3, q12, d12[0]		\n\t" // q3 = M0 * TXXXX
			"vst1.32 {q2}, [%[o], :128]!	\n\t" // store NXYZW
			
			// VERTEX 9 (q3, q6 still busy)
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load VXYZW
			"vadd.f32 q3, q3, q1			\n\t" // q3 = q3 + q1
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone matrix rows 2 + 3
			"vmov s15, s27					\n\t" // store T.W
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load NXYZW
			"vst1.32 {q3}, [%[o], :128]!	\n\t" // store TXYZW
			// q3, q6 done
			"vmul.f32 q0, q12, d8[0]		\n\t" // q0 = M0 * VXXXX
			"vmov q1, q15					\n\t" // DI q1 = M TRANSLATION
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M0 * NXXXX
			"vmla.f32 q1, q13, d8[1]		\n\t" // q1 = q1 + M1 * VYYYY
			"vmul.f32 q3, q13, d10[1]		\n\t" // q3 = M1 * NYYYY
			"vmla.f32 q0, q14, d9[0]		\n\t" // q0 = q0 + M2 * VZZZZ
			"vmla.f32 q2, q14, d11[0]		\n\t" // q2 = q2 + M2 * NZZZZ
			"vld1.32 {q6}, [%[v], :128]!	\n\t" // load TXYZW
			
			LOAD_BONE_PTR
			
			"vadd.f32 q0, q0, q1			\n\t" // q0 = q0 + q1
			"vmul.f32 q1, q13, d12[1]		\n\t" // q1 = M1 * TYYYY
			"vadd.f32 q2, q2, q3			\n\t" // q2 = q2 + q3
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store VXYZW
			"vmla.f32 q1, q14, d13[0]		\n\t" // q1 = q1 + M2 * TZZZZ
			"vmul.f32 q3, q12, d12[0]		\n\t" // q3 = M0 * TXXXX
			"vst1.32 {q2}, [%[o], :128]!	\n\t" // store NXYZW
			
			// VERTEX 10 (q3, q6 still busy)
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load VXYZW
			"vadd.f32 q3, q3, q1			\n\t" // q3 = q3 + q1
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone matrix rows 2 + 3
			"vmov s15, s27					\n\t" // store T.W
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load NXYZW
			"vst1.32 {q3}, [%[o], :128]!	\n\t" // store TXYZW
			// q3, q6 done
			"vmul.f32 q0, q12, d8[0]		\n\t" // q0 = M0 * VXXXX
			"vmov q1, q15					\n\t" // DI q1 = M TRANSLATION
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M0 * NXXXX
			"vmla.f32 q1, q13, d8[1]		\n\t" // q1 = q1 + M1 * VYYYY
			"vmul.f32 q3, q13, d10[1]		\n\t" // q3 = M1 * NYYYY
			"vmla.f32 q0, q14, d9[0]		\n\t" // q0 = q0 + M2 * VZZZZ
			"vmla.f32 q2, q14, d11[0]		\n\t" // q2 = q2 + M2 * NZZZZ
			"vld1.32 {q6}, [%[v], :128]!	\n\t" // load TXYZW
			
			LOAD_BONE_PTR
			
			"vadd.f32 q0, q0, q1			\n\t" // q0 = q0 + q1
			"vmul.f32 q1, q13, d12[1]		\n\t" // q1 = M1 * TYYYY
			"vadd.f32 q2, q2, q3			\n\t" // q2 = q2 + q3
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store VXYZW
			"vmla.f32 q1, q14, d13[0]		\n\t" // q1 = q1 + M2 * TZZZZ
			"vmul.f32 q3, q12, d12[0]		\n\t" // q3 = M0 * TXXXX
			"vst1.32 {q2}, [%[o], :128]!	\n\t" // store NXYZW
			
			// VERTEX 11 (q3, q6 still busy)
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load VXYZW
			"vadd.f32 q3, q3, q1			\n\t" // q3 = q3 + q1
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone matrix rows 2 + 3
			"vmov s15, s27					\n\t" // store T.W
			
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load NXYZW
			"vst1.32 {q3}, [%[o], :128]!	\n\t" // store TXYZW
			// q3, q6 done
			"vmul.f32 q0, q12, d8[0]		\n\t" // q0 = M0 * VXXXX
			"vmov q1, q15					\n\t" // DI q1 = M TRANSLATION
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M0 * NXXXX
			"vmla.f32 q1, q13, d8[1]		\n\t" // q1 = q1 + M1 * VYYYY
			"vmul.f32 q3, q13, d10[1]		\n\t" // q3 = M1 * NYYYY
			"vmla.f32 q0, q14, d9[0]		\n\t" // q0 = q0 + M2 * VZZZZ
			"vmla.f32 q2, q14, d11[0]		\n\t" // q2 = q2 + M2 * NZZZZ
			"vld1.32 {q6}, [%[v], :128]!	\n\t" // load TXYZW
			
			LOAD_BONE_PTR
			
			"vadd.f32 q0, q0, q1			\n\t" // q0 = q0 + q1
			"vmul.f32 q1, q13, d12[1]		\n\t" // q1 = M1 * TYYYY
			"vadd.f32 q2, q2, q3			\n\t" // q2 = q2 + q3
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store VXYZW
			"vmla.f32 q1, q14, d13[0]		\n\t" // q1 = q1 + M2 * TZZZZ
			"vmul.f32 q3, q12, d12[0]		\n\t" // q3 = M0 * TXXXX
			"vst1.32 {q2}, [%[o], :128]!	\n\t" // store NXYZW
			
			// VERTEX 12 (q3, q6 still busy)
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load VXYZW
			"vadd.f32 q3, q3, q1			\n\t" // q3 = q3 + q1
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone matrix rows 2 + 3
			"vmov s15, s27					\n\t" // store T.W
			
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load NXYZW
			"vst1.32 {q3}, [%[o], :128]!	\n\t" // store TXYZW
			// q3, q6 done
			"vmul.f32 q0, q12, d8[0]		\n\t" // q0 = M0 * VXXXX
			"vmov q1, q15					\n\t" // DI q1 = M TRANSLATION
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M0 * NXXXX
			"vmla.f32 q1, q13, d8[1]		\n\t" // q1 = q1 + M1 * VYYYY
			"vmul.f32 q3, q13, d10[1]		\n\t" // q3 = M1 * NYYYY
			"vmla.f32 q0, q14, d9[0]		\n\t" // q0 = q0 + M2 * VZZZZ
			"vmla.f32 q2, q14, d11[0]		\n\t" // q2 = q2 + M2 * NZZZZ
			"vld1.32 {q6}, [%[v], :128]!	\n\t" // load TXYZW
			
			LOAD_BONE_PTR
			
			"vadd.f32 q0, q0, q1			\n\t" // q0 = q0 + q1
			"vmul.f32 q1, q13, d12[1]		\n\t" // q1 = M1 * TYYYY
			"vadd.f32 q2, q2, q3			\n\t" // q2 = q2 + q3
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store VXYZW
			"vmla.f32 q1, q14, d13[0]		\n\t" // q1 = q1 + M2 * TZZZZ
			"vmul.f32 q3, q12, d12[0]		\n\t" // q3 = M0 * TXXXX
			"vst1.32 {q2}, [%[o], :128]!	\n\t" // store NXYZW
			
			// VERTEX 13 (q3, q6 still busy)
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load VXYZW
			"vadd.f32 q3, q3, q1			\n\t" // q3 = q3 + q1
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone matrix rows 2 + 3
			"vmov s15, s27					\n\t" // store T.W
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load NXYZW
			"vst1.32 {q3}, [%[o], :128]!	\n\t" // store TXYZW
			// q3, q6 done
			"vmul.f32 q0, q12, d8[0]		\n\t" // q0 = M0 * VXXXX
			"vmov q1, q15					\n\t" // DI q1 = M TRANSLATION
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M0 * NXXXX
			"vmla.f32 q1, q13, d8[1]		\n\t" // q1 = q1 + M1 * VYYYY
			"vmul.f32 q3, q13, d10[1]		\n\t" // q3 = M1 * NYYYY
			"vmla.f32 q0, q14, d9[0]		\n\t" // q0 = q0 + M2 * VZZZZ
			"vmla.f32 q2, q14, d11[0]		\n\t" // q2 = q2 + M2 * NZZZZ
			"vld1.32 {q6}, [%[v], :128]!	\n\t" // load TXYZW
			
			LOAD_BONE_PTR
			
			"vadd.f32 q0, q0, q1			\n\t" // q0 = q0 + q1
			"vmul.f32 q1, q13, d12[1]		\n\t" // q1 = M1 * TYYYY
			"vadd.f32 q2, q2, q3			\n\t" // q2 = q2 + q3
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store VXYZW
			"vmla.f32 q1, q14, d13[0]		\n\t" // q1 = q1 + M2 * TZZZZ
			"vmul.f32 q3, q12, d12[0]		\n\t" // q3 = M0 * TXXXX
			"vst1.32 {q2}, [%[o], :128]!	\n\t" // store NXYZW
			
			// VERTEX 14 (q3, q6 still busy)
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load VXYZW
			"vadd.f32 q3, q3, q1			\n\t" // q3 = q3 + q1
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone matrix rows 2 + 3
			"vmov s15, s27					\n\t" // store T.W
			
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load NXYZW
			"vst1.32 {q3}, [%[o], :128]!	\n\t" // store TXYZW
			// q3, q6 done
			"vmul.f32 q0, q12, d8[0]		\n\t" // q0 = M0 * VXXXX
			"vmov q1, q15					\n\t" // DI q1 = M TRANSLATION
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M0 * NXXXX
			"vmla.f32 q1, q13, d8[1]		\n\t" // q1 = q1 + M1 * VYYYY
			"vmul.f32 q3, q13, d10[1]		\n\t" // q3 = M1 * NYYYY
			"vmla.f32 q0, q14, d9[0]		\n\t" // q0 = q0 + M2 * VZZZZ
			"vmla.f32 q2, q14, d11[0]		\n\t" // q2 = q2 + M2 * NZZZZ
			"vld1.32 {q6}, [%[v], :128]!	\n\t" // load TXYZW
			
			LOAD_BONE_PTR
			
			"vadd.f32 q0, q0, q1			\n\t" // q0 = q0 + q1
			"vmul.f32 q1, q13, d12[1]		\n\t" // q1 = M1 * TYYYY
			"vadd.f32 q2, q2, q3			\n\t" // q2 = q2 + q3
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store VXYZW
			"vmla.f32 q1, q14, d13[0]		\n\t" // q1 = q1 + M2 * TZZZZ
			"vmul.f32 q3, q12, d12[0]		\n\t" // q3 = M0 * TXXXX
			"vst1.32 {q2}, [%[o], :128]!	\n\t" // store NXYZW
			
			// VERTEX 15 (q3, q6 still busy)
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load VXYZW
			"vadd.f32 q3, q3, q1			\n\t" // q3 = q3 + q1
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone matrix rows 2 + 3
			"vmov s15, s27					\n\t" // store T.W
			
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load NXYZW
			"vst1.32 {q3}, [%[o], :128]!	\n\t" // store TXYZW
			// q3, q6 done
			"vmul.f32 q0, q12, d8[0]		\n\t" // q0 = M0 * VXXXX
			"vmov q1, q15					\n\t" // DI q1 = M TRANSLATION
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M0 * NXXXX
			"vmla.f32 q1, q13, d8[1]		\n\t" // q1 = q1 + M1 * VYYYY
			"vmul.f32 q3, q13, d10[1]		\n\t" // q3 = M1 * NYYYY
			"vmla.f32 q0, q14, d9[0]		\n\t" // q0 = q0 + M2 * VZZZZ
			"vmla.f32 q2, q14, d11[0]		\n\t" // q2 = q2 + M2 * NZZZZ
			"vld1.32 {q6}, [%[v], :128]!	\n\t" // load TXYZW
			
			LOAD_BONE_PTR
			
			"vadd.f32 q0, q0, q1			\n\t" // q0 = q0 + q1
			"vmul.f32 q1, q13, d12[1]		\n\t" // q1 = M1 * TYYYY
			"vadd.f32 q2, q2, q3			\n\t" // q2 = q2 + q3
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store VXYZW
			"vmla.f32 q1, q14, d13[0]		\n\t" // q1 = q1 + M2 * TZZZZ
			"vmul.f32 q3, q12, d12[0]		\n\t" // q3 = M0 * TXXXX
			"vst1.32 {q2}, [%[o], :128]!	\n\t" // store NXYZW
			
			// VERTEX 16 (q3, q6 still busy)
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load VXYZW
			"vadd.f32 q3, q3, q1			\n\t" // q3 = q3 + q1
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone matrix rows 2 + 3
			"vmov s15, s27					\n\t" // store T.W
			
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load NXYZW
			"vst1.32 {q3}, [%[o], :128]!	\n\t" // store TXYZW
			// q3, q6 done
			"vmul.f32 q0, q12, d8[0]		\n\t" // q0 = M0 * VXXXX
			"vmov q1, q15					\n\t" // DI q1 = M TRANSLATION
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M0 * NXXXX
			"vmla.f32 q1, q13, d8[1]		\n\t" // q1 = q1 + M1 * VYYYY
			"vmul.f32 q3, q13, d10[1]		\n\t" // q3 = M1 * NYYYY
			"vmla.f32 q0, q14, d9[0]		\n\t" // q0 = q0 + M2 * VZZZZ
			"vmla.f32 q2, q14, d11[0]		\n\t" // q2 = q2 + M2 * NZZZZ
			"vld1.32 {q6}, [%[v], :128]!	\n\t" // load TXYZW
			"vadd.f32 q0, q0, q1			\n\t" // q0 = q0 + q1
			"vmul.f32 q1, q13, d12[1]		\n\t" // q1 = M1 * TYYYY
			"vadd.f32 q2, q2, q3			\n\t" // q2 = q2 + q3
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store VXYZW
			"vmla.f32 q1, q14, d13[0]		\n\t" // q1 = q1 + M2 * TZZZZ
			"vmul.f32 q3, q12, d12[0]		\n\t" // q3 = M0 * TXXXX
			"vst1.32 {q2}, [%[o], :128]!	\n\t" // store NXYZW
						
			// stall q3
			"vadd.f32 q3, q3, q1			\n\t" // q3 = q3 + q1
			// stall q3
			"vmov s15, s27					\n\t" // store T.W
			// stall q3
			"vst1.32 {q3}, [%[o], :128]!	\n\t" // store TXYZW
			
			// loop
			"sub %[nv], %[nv], #16			\n\t"
			"cmp %[nv], #16					\n\t"
			"bge Lskin1_16					\n\t"
			STORE_BONE_OFS
			
		: [o] "+r" (outVerts), [v] "+r" (vertices), [bi] "+r" (boneIndices), [nv] "+r" (numVerts)
		: [bones] "r" (bones)
		: "cc", "r9", "r10", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q12", "q13", "q14", "q15"
		);
	}

	if (numVerts >= 8) {
		
		asm volatile (
			
			LOAD_BONE_OFS
			
			"Lskin1_8:						\n\t"
			
			// VERTEX 1
			
			LOAD_BONE_PTR
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load VXYZW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone matrix rows 2 + 3
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load NXYZW
			"vmul.f32 q0, q12, d8[0]		\n\t" // q0 = M0 * VXXXX
			"vmov q1, q15					\n\t" // DI q1 = M TRANSLATION
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M0 * NXXXX
			"vmla.f32 q1, q13, d8[1]		\n\t" // q1 = q1 + M1 * VYYYY
			"vmul.f32 q3, q13, d10[1]		\n\t" // q3 = M1 * NYYYY
			"vmla.f32 q0, q14, d9[0]		\n\t" // q0 = q0 + M2 * VZZZZ
			"vmla.f32 q2, q14, d11[0]		\n\t" // q2 = q2 + M2 * NZZZZ
			"vld1.32 {q6}, [%[v], :128]!	\n\t" // load TXYZW
			
			LOAD_BONE_PTR
			
			"vadd.f32 q0, q0, q1			\n\t" // q0 = q0 + q1
			"vmul.f32 q1, q13, d12[1]		\n\t" // q1 = M1 * TYYYY
			"vadd.f32 q2, q2, q3			\n\t" // q2 = q2 + q3
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store VXYZW
			"vmla.f32 q1, q14, d13[0]		\n\t" // q1 = q1 + M2 * TZZZZ
			"vmul.f32 q3, q12, d12[0]		\n\t" // q3 = M0 * TXXXX
			"vst1.32 {q2}, [%[o], :128]!	\n\t" // store NXYZW
			
			// VERTEX 2 (q3, q6 still busy)
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load VXYZW
			"vadd.f32 q3, q3, q1			\n\t" // q3 = q3 + q1
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone matrix rows 2 + 3
			"vmov s15, s27					\n\t" // store T.W
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load NXYZW
			"vst1.32 {q3}, [%[o], :128]!	\n\t" // store TXYZW
			// q3, q6 done
			"vmul.f32 q0, q12, d8[0]		\n\t" // q0 = M0 * VXXXX
			"vmov q1, q15					\n\t" // DI q1 = M TRANSLATION
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M0 * NXXXX
			"vmla.f32 q1, q13, d8[1]		\n\t" // q1 = q1 + M1 * VYYYY
			"vmul.f32 q3, q13, d10[1]		\n\t" // q3 = M1 * NYYYY
			"vmla.f32 q0, q14, d9[0]		\n\t" // q0 = q0 + M2 * VZZZZ
			"vmla.f32 q2, q14, d11[0]		\n\t" // q2 = q2 + M2 * NZZZZ
			"vld1.32 {q6}, [%[v], :128]!	\n\t" // load TXYZW
			
			LOAD_BONE_PTR
			
			"vadd.f32 q0, q0, q1			\n\t" // q0 = q0 + q1
			"vmul.f32 q1, q13, d12[1]		\n\t" // q1 = M1 * TYYYY
			"vadd.f32 q2, q2, q3			\n\t" // q2 = q2 + q3
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store VXYZW
			"vmla.f32 q1, q14, d13[0]		\n\t" // q1 = q1 + M2 * TZZZZ
			"vmul.f32 q3, q12, d12[0]		\n\t" // q3 = M0 * TXXXX
			"vst1.32 {q2}, [%[o], :128]!	\n\t" // store NXYZW
			
			// VERTEX 3 (q3, q6 still busy)
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load VXYZW
			"vadd.f32 q3, q3, q1			\n\t" // q3 = q3 + q1
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone matrix rows 2 + 3
			"vmov s15, s27					\n\t" // store T.W
			
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load NXYZW
			"vst1.32 {q3}, [%[o], :128]!	\n\t" // store TXYZW
			// q3, q6 done
			"vmul.f32 q0, q12, d8[0]		\n\t" // q0 = M0 * VXXXX
			"vmov q1, q15					\n\t" // DI q1 = M TRANSLATION
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M0 * NXXXX
			"vmla.f32 q1, q13, d8[1]		\n\t" // q1 = q1 + M1 * VYYYY
			"vmul.f32 q3, q13, d10[1]		\n\t" // q3 = M1 * NYYYY
			"vmla.f32 q0, q14, d9[0]		\n\t" // q0 = q0 + M2 * VZZZZ
			"vmla.f32 q2, q14, d11[0]		\n\t" // q2 = q2 + M2 * NZZZZ
			"vld1.32 {q6}, [%[v], :128]!	\n\t" // load TXYZW
			
			LOAD_BONE_PTR
			
			"vadd.f32 q0, q0, q1			\n\t" // q0 = q0 + q1
			"vmul.f32 q1, q13, d12[1]		\n\t" // q1 = M1 * TYYYY
			"vadd.f32 q2, q2, q3			\n\t" // q2 = q2 + q3
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store VXYZW
			"vmla.f32 q1, q14, d13[0]		\n\t" // q1 = q1 + M2 * TZZZZ
			"vmul.f32 q3, q12, d12[0]		\n\t" // q3 = M0 * TXXXX
			"vst1.32 {q2}, [%[o], :128]!	\n\t" // store NXYZW
			
			// VERTEX 4 (q3, q6 still busy)
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load VXYZW
			"vadd.f32 q3, q3, q1			\n\t" // q3 = q3 + q1
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone matrix rows 2 + 3
			"vmov s15, s27					\n\t" // store T.W
			
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load NXYZW
			"vst1.32 {q3}, [%[o], :128]!	\n\t" // store TXYZW
			// q3, q6 done
			"vmul.f32 q0, q12, d8[0]		\n\t" // q0 = M0 * VXXXX
			"vmov q1, q15					\n\t" // DI q1 = M TRANSLATION
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M0 * NXXXX
			"vmla.f32 q1, q13, d8[1]		\n\t" // q1 = q1 + M1 * VYYYY
			"vmul.f32 q3, q13, d10[1]		\n\t" // q3 = M1 * NYYYY
			"vmla.f32 q0, q14, d9[0]		\n\t" // q0 = q0 + M2 * VZZZZ
			"vmla.f32 q2, q14, d11[0]		\n\t" // q2 = q2 + M2 * NZZZZ
			"vld1.32 {q6}, [%[v], :128]!	\n\t" // load TXYZW
			
			LOAD_BONE_PTR
			
			"vadd.f32 q0, q0, q1			\n\t" // q0 = q0 + q1
			"vmul.f32 q1, q13, d12[1]		\n\t" // q1 = M1 * TYYYY
			"vadd.f32 q2, q2, q3			\n\t" // q2 = q2 + q3
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store VXYZW
			"vmla.f32 q1, q14, d13[0]		\n\t" // q1 = q1 + M2 * TZZZZ
			"vmul.f32 q3, q12, d12[0]		\n\t" // q3 = M0 * TXXXX
			"vst1.32 {q2}, [%[o], :128]!	\n\t" // store NXYZW
			
			// VERTEX 5 (q3, q6 still busy)
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load VXYZW
			"vadd.f32 q3, q3, q1			\n\t" // q3 = q3 + q1
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone matrix rows 2 + 3
			"vmov s15, s27					\n\t" // store T.W
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load NXYZW
			"vst1.32 {q3}, [%[o], :128]!	\n\t" // store TXYZW
			// q3, q6 done
			"vmul.f32 q0, q12, d8[0]		\n\t" // q0 = M0 * VXXXX
			"vmov q1, q15					\n\t" // DI q1 = M TRANSLATION
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M0 * NXXXX
			"vmla.f32 q1, q13, d8[1]		\n\t" // q1 = q1 + M1 * VYYYY
			"vmul.f32 q3, q13, d10[1]		\n\t" // q3 = M1 * NYYYY
			"vmla.f32 q0, q14, d9[0]		\n\t" // q0 = q0 + M2 * VZZZZ
			"vmla.f32 q2, q14, d11[0]		\n\t" // q2 = q2 + M2 * NZZZZ
			"vld1.32 {q6}, [%[v], :128]!	\n\t" // load TXYZW
			
			LOAD_BONE_PTR
			
			"vadd.f32 q0, q0, q1			\n\t" // q0 = q0 + q1
			"vmul.f32 q1, q13, d12[1]		\n\t" // q1 = M1 * TYYYY
			"vadd.f32 q2, q2, q3			\n\t" // q2 = q2 + q3
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store VXYZW
			"vmla.f32 q1, q14, d13[0]		\n\t" // q1 = q1 + M2 * TZZZZ
			"vmul.f32 q3, q12, d12[0]		\n\t" // q3 = M0 * TXXXX
			"vst1.32 {q2}, [%[o], :128]!	\n\t" // store NXYZW
			
			// VERTEX 6 (q3, q6 still busy)
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load VXYZW
			"vadd.f32 q3, q3, q1			\n\t" // q3 = q3 + q1
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone matrix rows 2 + 3
			"vmov s15, s27					\n\t" // store T.W
			
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load NXYZW
			"vst1.32 {q3}, [%[o], :128]!	\n\t" // store TXYZW
			// q3, q6 done
			"vmul.f32 q0, q12, d8[0]		\n\t" // q0 = M0 * VXXXX
			"vmov q1, q15					\n\t" // DI q1 = M TRANSLATION
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M0 * NXXXX
			"vmla.f32 q1, q13, d8[1]		\n\t" // q1 = q1 + M1 * VYYYY
			"vmul.f32 q3, q13, d10[1]		\n\t" // q3 = M1 * NYYYY
			"vmla.f32 q0, q14, d9[0]		\n\t" // q0 = q0 + M2 * VZZZZ
			"vmla.f32 q2, q14, d11[0]		\n\t" // q2 = q2 + M2 * NZZZZ
			"vld1.32 {q6}, [%[v], :128]!	\n\t" // load TXYZW
			
			LOAD_BONE_PTR
			
			"vadd.f32 q0, q0, q1			\n\t" // q0 = q0 + q1
			"vmul.f32 q1, q13, d12[1]		\n\t" // q1 = M1 * TYYYY
			"vadd.f32 q2, q2, q3			\n\t" // q2 = q2 + q3
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store VXYZW
			"vmla.f32 q1, q14, d13[0]		\n\t" // q1 = q1 + M2 * TZZZZ
			"vmul.f32 q3, q12, d12[0]		\n\t" // q3 = M0 * TXXXX
			"vst1.32 {q2}, [%[o], :128]!	\n\t" // store NXYZW
			
			// VERTEX 7 (q3, q6 still busy)
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load VXYZW
			"vadd.f32 q3, q3, q1			\n\t" // q3 = q3 + q1
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone matrix rows 2 + 3
			"vmov s15, s27					\n\t" // store T.W
			
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load NXYZW
			"vst1.32 {q3}, [%[o], :128]!	\n\t" // store TXYZW
			// q3, q6 done
			"vmul.f32 q0, q12, d8[0]		\n\t" // q0 = M0 * VXXXX
			"vmov q1, q15					\n\t" // DI q1 = M TRANSLATION
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M0 * NXXXX
			"vmla.f32 q1, q13, d8[1]		\n\t" // q1 = q1 + M1 * VYYYY
			"vmul.f32 q3, q13, d10[1]		\n\t" // q3 = M1 * NYYYY
			"vmla.f32 q0, q14, d9[0]		\n\t" // q0 = q0 + M2 * VZZZZ
			"vmla.f32 q2, q14, d11[0]		\n\t" // q2 = q2 + M2 * NZZZZ
			"vld1.32 {q6}, [%[v], :128]!	\n\t" // load TXYZW
			
			LOAD_BONE_PTR
			
			"vadd.f32 q0, q0, q1			\n\t" // q0 = q0 + q1
			"vmul.f32 q1, q13, d12[1]		\n\t" // q1 = M1 * TYYYY
			"vadd.f32 q2, q2, q3			\n\t" // q2 = q2 + q3
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store VXYZW
			"vmla.f32 q1, q14, d13[0]		\n\t" // q1 = q1 + M2 * TZZZZ
			"vmul.f32 q3, q12, d12[0]		\n\t" // q3 = M0 * TXXXX
			"vst1.32 {q2}, [%[o], :128]!	\n\t" // store NXYZW
			
			// VERTEX 8 (q3, q6 still busy)
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load VXYZW
			"vadd.f32 q3, q3, q1			\n\t" // q3 = q3 + q1
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone matrix rows 2 + 3
			"vmov s15, s27					\n\t" // store T.W
			
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load NXYZW
			"vst1.32 {q3}, [%[o], :128]!	\n\t" // store TXYZW
			// q3, q6 done
			"vmul.f32 q0, q12, d8[0]		\n\t" // q0 = M0 * VXXXX
			"vmov q1, q15					\n\t" // DI q1 = M TRANSLATION
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M0 * NXXXX
			"vmla.f32 q1, q13, d8[1]		\n\t" // q1 = q1 + M1 * VYYYY
			"vmul.f32 q3, q13, d10[1]		\n\t" // q3 = M1 * NYYYY
			"vmla.f32 q0, q14, d9[0]		\n\t" // q0 = q0 + M2 * VZZZZ
			"vmla.f32 q2, q14, d11[0]		\n\t" // q2 = q2 + M2 * NZZZZ
			"vld1.32 {q6}, [%[v], :128]!	\n\t" // load TXYZW
			"vadd.f32 q0, q0, q1			\n\t" // q0 = q0 + q1
			"vmul.f32 q1, q13, d12[1]		\n\t" // q1 = M1 * TYYYY
			"vadd.f32 q2, q2, q3			\n\t" // q2 = q2 + q3
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store VXYZW
			"vmla.f32 q1, q14, d13[0]		\n\t" // q1 = q1 + M2 * TZZZZ
			"vmul.f32 q3, q12, d12[0]		\n\t" // q3 = M0 * TXXXX
			"vst1.32 {q2}, [%[o], :128]!	\n\t" // store NXYZW
						
			// stall q3
			"vadd.f32 q3, q3, q1			\n\t" // q3 = q3 + q1
			// stall q3
			"vmov s15, s27					\n\t" // store T.W
			// stall q3
			"vst1.32 {q3}, [%[o], :128]!	\n\t" // store TXYZW
			
			// loop
			"sub %[nv], %[nv], #8			\n\t"
			"cmp %[nv], #8					\n\t"
			"bge Lskin1_8					\n\t"
			STORE_BONE_OFS
			
		: [o] "+r" (outVerts), [v] "+r" (vertices), [bi] "+r" (boneIndices), [nv] "+r" (numVerts)
		: [bones] "r" (bones)
		: "cc", "r9", "r10", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q12", "q13", "q14", "q15"
		);
	}
	
	if (numVerts >= 4) {
		
		asm volatile (
			
			LOAD_BONE_OFS
			
			"Lskin1_4:						\n\t"
			
			// VERTEX 1
			
			LOAD_BONE_PTR
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load VXYZW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone matrix rows 2 + 3
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load NXYZW
			"vmul.f32 q0, q12, d8[0]		\n\t" // q0 = M0 * VXXXX
			"vmov q1, q15					\n\t" // DI q1 = M TRANSLATION
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M0 * NXXXX
			"vmla.f32 q1, q13, d8[1]		\n\t" // q1 = q1 + M1 * VYYYY
			"vmul.f32 q3, q13, d10[1]		\n\t" // q3 = M1 * NYYYY
			"vmla.f32 q0, q14, d9[0]		\n\t" // q0 = q0 + M2 * VZZZZ
			"vmla.f32 q2, q14, d11[0]		\n\t" // q2 = q2 + M2 * NZZZZ
			"vld1.32 {q6}, [%[v], :128]!	\n\t" // load TXYZW
			
			LOAD_BONE_PTR
			
			"vadd.f32 q0, q0, q1			\n\t" // q0 = q0 + q1
			"vmul.f32 q1, q13, d12[1]		\n\t" // q1 = M1 * TYYYY
			"vadd.f32 q2, q2, q3			\n\t" // q2 = q2 + q3
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store VXYZW
			"vmla.f32 q1, q14, d13[0]		\n\t" // q1 = q1 + M2 * TZZZZ
			"vmul.f32 q3, q12, d12[0]		\n\t" // q3 = M0 * TXXXX
			"vst1.32 {q2}, [%[o], :128]!	\n\t" // store NXYZW
			
			// VERTEX 2 (q3, q6 still busy)
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load VXYZW
			"vadd.f32 q3, q3, q1			\n\t" // q3 = q3 + q1
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone matrix rows 2 + 3
			"vmov s15, s27					\n\t" // store T.W
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load NXYZW
			"vst1.32 {q3}, [%[o], :128]!	\n\t" // store TXYZW
			// q3, q6 done
			"vmul.f32 q0, q12, d8[0]		\n\t" // q0 = M0 * VXXXX
			"vmov q1, q15					\n\t" // DI q1 = M TRANSLATION
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M0 * NXXXX
			"vmla.f32 q1, q13, d8[1]		\n\t" // q1 = q1 + M1 * VYYYY
			"vmul.f32 q3, q13, d10[1]		\n\t" // q3 = M1 * NYYYY
			"vmla.f32 q0, q14, d9[0]		\n\t" // q0 = q0 + M2 * VZZZZ
			"vmla.f32 q2, q14, d11[0]		\n\t" // q2 = q2 + M2 * NZZZZ
			"vld1.32 {q6}, [%[v], :128]!	\n\t" // load TXYZW
			
			LOAD_BONE_PTR
			
			"vadd.f32 q0, q0, q1			\n\t" // q0 = q0 + q1
			"vmul.f32 q1, q13, d12[1]		\n\t" // q1 = M1 * TYYYY
			"vadd.f32 q2, q2, q3			\n\t" // q2 = q2 + q3
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store VXYZW
			"vmla.f32 q1, q14, d13[0]		\n\t" // q1 = q1 + M2 * TZZZZ
			"vmul.f32 q3, q12, d12[0]		\n\t" // q3 = M0 * TXXXX
			"vst1.32 {q2}, [%[o], :128]!	\n\t" // store NXYZW
			
			// VERTEX 3 (q3, q6 still busy)
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load VXYZW
			"vadd.f32 q3, q3, q1			\n\t" // q3 = q3 + q1
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone matrix rows 2 + 3
			"vmov s15, s27					\n\t" // store T.W
			
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load NXYZW
			"vst1.32 {q3}, [%[o], :128]!	\n\t" // store TXYZW
			// q3, q6 done
			"vmul.f32 q0, q12, d8[0]		\n\t" // q0 = M0 * VXXXX
			"vmov q1, q15					\n\t" // DI q1 = M TRANSLATION
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M0 * NXXXX
			"vmla.f32 q1, q13, d8[1]		\n\t" // q1 = q1 + M1 * VYYYY
			"vmul.f32 q3, q13, d10[1]		\n\t" // q3 = M1 * NYYYY
			"vmla.f32 q0, q14, d9[0]		\n\t" // q0 = q0 + M2 * VZZZZ
			"vmla.f32 q2, q14, d11[0]		\n\t" // q2 = q2 + M2 * NZZZZ
			"vld1.32 {q6}, [%[v], :128]!	\n\t" // load TXYZW
			
			LOAD_BONE_PTR
			
			"vadd.f32 q0, q0, q1			\n\t" // q0 = q0 + q1
			"vmul.f32 q1, q13, d12[1]		\n\t" // q1 = M1 * TYYYY
			"vadd.f32 q2, q2, q3			\n\t" // q2 = q2 + q3
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store VXYZW
			"vmla.f32 q1, q14, d13[0]		\n\t" // q1 = q1 + M2 * TZZZZ
			"vmul.f32 q3, q12, d12[0]		\n\t" // q3 = M0 * TXXXX
			"vst1.32 {q2}, [%[o], :128]!	\n\t" // store NXYZW
			
			// VERTEX 4 (q3, q6 still busy)
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load VXYZW
			"vadd.f32 q3, q3, q1			\n\t" // q3 = q3 + q1
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone matrix rows 2 + 3
			"vmov s15, s27					\n\t" // store T.W
			
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load NXYZW
			"vst1.32 {q3}, [%[o], :128]!	\n\t" // store TXYZW
			// q3, q6 done
			"vmul.f32 q0, q12, d8[0]		\n\t" // q0 = M0 * VXXXX
			"vmov q1, q15					\n\t" // DI q1 = M TRANSLATION
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M0 * NXXXX
			"vmla.f32 q1, q13, d8[1]		\n\t" // q1 = q1 + M1 * VYYYY
			"vmul.f32 q3, q13, d10[1]		\n\t" // q3 = M1 * NYYYY
			"vmla.f32 q0, q14, d9[0]		\n\t" // q0 = q0 + M2 * VZZZZ
			"vmla.f32 q2, q14, d11[0]		\n\t" // q2 = q2 + M2 * NZZZZ
			"vld1.32 {q6}, [%[v], :128]!	\n\t" // load TXYZW
			"vadd.f32 q0, q0, q1			\n\t" // q0 = q0 + q1
			"vmul.f32 q1, q13, d12[1]		\n\t" // q1 = M1 * TYYYY
			"vadd.f32 q2, q2, q3			\n\t" // q2 = q2 + q3
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store VXYZW
			"vmla.f32 q1, q14, d13[0]		\n\t" // q1 = q1 + M2 * TZZZZ
			"vmul.f32 q3, q12, d12[0]		\n\t" // q3 = M0 * TXXXX
			"vst1.32 {q2}, [%[o], :128]!	\n\t" // store NXYZW
						
			// stall q3
			"vadd.f32 q3, q3, q1			\n\t" // q3 = q3 + q1
			// stall q3
			"vmov s15, s27					\n\t" // store T.W
			// stall q3
			"vst1.32 {q3}, [%[o], :128]!	\n\t" // store TXYZW
			
			// loop
			"sub %[nv], %[nv], #4			\n\t"
			"cmp %[nv], #4					\n\t"
			"bge Lskin1_4					\n\t"
			"mov %[bi], r10					\n\t"
			
		: [o] "+r" (outVerts), [v] "+r" (vertices), [bi] "+r" (boneIndices), [nv] "+r" (numVerts)
		: [bones] "r" (bones)
		: "cc", "r9", "r10", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q12", "q13", "q14", "q15"
		);
	}
	
	if (numVerts > 0) {
		
		asm volatile (
			
			LOAD_BONE_OFS
			
			"Lskin1_1:						\n\t"
						
			LOAD_BONE_PTR
			
			"vld1.32 {q4}, [%[v], :128]!	\n\t" // load VXYZW
			"vld1.32 {q12-q13}, [r9, :128]!	\n\t" // load bone matrix rows 0 + 1
			"vld1.32 {q14-q15}, [r9, :128]!	\n\t" // load bone matrix rows 2 + 3
			"vld1.32 {q5}, [%[v], :128]!	\n\t" // load NXYZW
			"vmul.f32 q0, q12, d8[0]		\n\t" // q0 = M0 * VXXXX
			"vmov q1, q15					\n\t" // DI q1 = M TRANSLATION
			"vmul.f32 q2, q12, d10[0]		\n\t" // q2 = M0 * NXXXX
			"vmla.f32 q1, q13, d8[1]		\n\t" // q1 = q1 + M1 * VYYYY
			"vmul.f32 q3, q13, d10[1]		\n\t" // q3 = M1 * NYYYY
			"vmla.f32 q0, q14, d9[0]		\n\t" // q0 = q0 + M2 * VZZZZ
			"vmla.f32 q2, q14, d11[0]		\n\t" // q2 = q2 + M2 * NZZZZ
			"vld1.32 {q6}, [%[v], :128]!	\n\t" // load TXYZW
			"vadd.f32 q0, q0, q1			\n\t" // q0 = q0 + q1
			"vmul.f32 q1, q13, d12[1]		\n\t" // q1 = M1 * TYYYY
			"vadd.f32 q2, q2, q3			\n\t" // q2 = q2 + q3
			"vst1.32 {q0}, [%[o], :128]!	\n\t" // store VXYZW
			"vmla.f32 q1, q14, d13[0]		\n\t" // q1 = q1 + M2 * TZZZZ
			"vmul.f32 q3, q12, d12[0]		\n\t" // q3 = M0 * TXXXX
			"vst1.32 {q2}, [%[o], :128]!	\n\t" // store NXYZW
			// stall q3
			"vadd.f32 q3, q3, q1			\n\t" // q3 = q3 + q1
			// stall q3
			"vmov s15, s27					\n\t" // store T.W
			// stall q3
			"vst1.32 {q3}, [%[o], :128]!	\n\t" // store TXYZW
			
			// loop
			"subs %[nv], %[nv], #1			\n\t"
			"bne Lskin1_1					\n\t"
			STORE_BONE_OFS
			
		: [o] "+r" (outVerts), [v] "+r" (vertices), [bi] "+r" (boneIndices), [nv] "+r" (numVerts)
		: [bones] "r" (bones)
		: "cc", "r9", "r10", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q12", "q13", "q14", "q15"
		);
	}
}

}

const SIMDDriver *SIMD_neon_bind()
{
	static SIMDDriver d;

	if (d.name[0])
		return &d;

	d = *SIMD_ref_bind();
	d.SkinVerts[0] = &SkinVerts1B;
	d.SkinVerts[1] = &SkinVerts2B;
	d.SkinVerts[2] = &SkinVerts3B;
	
	string::cpy(d.name, "SIMD_neon");
	return &d;
}

#endif

