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

#define LOAD_BONE_PTR \
	"ldrh r9, [%[bi]]           \n\t" \
	"lsls r9, r9, #6            \n\t" \
	"add  r9, r9, %[bones]		\n\t" \
	"add %[bi], %[bi], #2       \n\t"

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

void SkinVerts3(
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
				
			// q1 = q1 + b1_col2 * Z
			"vmla.f32 q1, q8, d11[0]          \n\t"
			// q2 = q2 + b1_col3 * W
			"vmla.f32 q2, q9, d11[1]          \n\t"
			
			// q1 = q1 + b2_col0 * X
			"vmla.f32 q1, q3, d0[0]           \n\t"
			// q2 = q2 + b2_col1 * Y
			"vmla.f32 q2, q4, d0[1]           \n\t"
			// q1 = q1 + b2_col2 * Z
			"vmla.f32 q1, q6, d1[0]           \n\t"
			// q2 = q2 + b2_col3 * W
			"vmla.f32 q2, q7, d1[1]           \n\t"
					  
			"vadd.f32 q1, q1, q2              \n\t"
			"vst1.32 {q1}, [%[o], :128]!      \n\t"
		: [o] "+r" (outVerts), [v] "+r" (vertices), [bi] "+r" (boneIndices)
		: [bones] "r" (bones)
		: "cc", "r9", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "s12", "s13", "s14", "s15", "s16", "s17", "s18", "s19", "s20", "s21", "s22", "s23", "s24", "s25", "s26", "s27", "s28", "s29", "s30", "s31", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d8", "d9", "d10", "d11", "d12", "d13", "d14", "d15", "d16", "d17", "d18", "d19", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9"
		);
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
			"vld1.32 {q1-q2}, [r9, :128]!  \n\t"
			"vld1.32 {q3-q4}, [r9, :128]!  \n\t"
			LOAD_BONE_PTR
					  
			// q1 = b0_col0 * X
			"vmul.f32 q1, q1, d0[0]           \n\t"
			// q5 = b1 vertex (DI)
			"vld1.32 {q5}, [%[v], :128]!      \n\t"
					  
			// q2 = b0_col1 * X
			"vmul.f32 q2, q2, d0[1]			  \n\t"
			// q6-q7 = b1_col0+1 (DI)
			"vld1.32 {q6-q7}, [r9, :128]!     \n\t"
					  
			// q1 = q1 + b0_col2 * Z
			"vmla.f32 q1, q3, d1[0]           \n\t"
			// q8-q9 = b1_col1+2 (DI)
			"vld1.32 {q8-q9}, [r9, :128]!     \n\t"

			// q2 = q2 + b0_col3 * W
			"vmla.f32 q2, q4, d1[1]           \n\t"
			// q1 = q1 + b1_col0 * X
			"vmla.f32 q1, q6, d10[0]          \n\t"
			// q2 = q2 + b1_col1 * Y
			"vmla.f32 q2, q7, d10[1]          \n\t"
			// q1 = q1 + b1_col2 * Z
			"vmla.f32 q1, q8, d11[0]          \n\t"
			// q2 = q2 + b1_col3 * W
			"vmla.f32 q2, q9, d11[1]          \n\t"
			
			"vadd.f32 q1, q1, q2              \n\t"
			"vst1.32 {q1}, [%[o], :128]!      \n\t"
		: [o] "+r" (outVerts), [v] "+r" (vertices), [bi] "+r" (boneIndices)
		: [bones] "r" (bones)
		: "cc", "r9", "r10", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "s12", "s13", "s14", "s15", "s16", "s17", "s18", "s19", "s20", "s21", "s22", "s23", "s24", "s25", "s26", "s27", "s28", "s29", "s30", "s31", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d8", "d9", "d10", "d11", "d12", "d13", "d14", "d15", "d16", "d17", "d18", "d19", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9"
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
	
#define FETCH_VERT_0 "vld1.32 {q0}, [%[v], :128]! \n\t"
#define FETCH_NORM_0 "vld1.32 {q1}, {%[v], :128]! \n\t"
#define FETCH_TAN_0 "vld1.32 {q1}, {%[v], :128]! \n\t"

#define FETCH_VERT_1 "vld1.32 {q1}, [%[v], :128]! \n\t"
	
#define FETCH_BONE_0_0 "vld1.32 {q2-q3}, [r9, :128]!  \n\t"
#define FETCH_BONE_0_1 "vld1.32 {q4-q5}, [r9, :128]!  \n\t"
#define FETCH_BONE_1_0 "vld1.32 {q6-q7}, [r9, :128]!  \n\t"
#define FETCH_BONE_1_1 "vld1.32 {q8-q9}, [r9, :128]!  \n\t"
	
#define VERT_0_OP1 "vmul.f32 q2, q2, d0[0] \n\t"
#define VERT_0_OP2 "vmla.f32 q5, q3, d0[1] \n\t"
#define VERT_0_OP3 "vmla.f32 q2, q4, d1[0] \n\t"
#define VERT_0_OP4 "vadd.f32 q5, q5, q2    \n\t"
#define VERT_0_STORE "vst1.32 {q5}, [%[o], :128]! \n\t"

#define LOAD_BONE_PTR \
	"ldrh r9, [%[bi]]           \n\t" \
	"lsls r9, r9, #6            \n\t" \
	"add  r9, r9, %[bones]		\n\t" \
	"add %[bi], %[bi], #2       \n\t"
	
	while (numVerts-- > 0) {
		const float *bone0 = bones + boneIndices[0]*SIMDDriver::kNumBoneFloats;
		++boneIndices;
		
		asm volatile (
			
			"vld1.32 {q4}, [%[v], :128]!			\n\t" // load VXYZW
			"vld1.32 {q12-q13}, [%[bone00], :128]	\n\t" // load bone matrix rows 0 + 1
			"vld1.32 {q14-q15}, [%[bone01], :128]	\n\t" // load bone matrix rows 2 + 3
			"vld1.32 {q5}, [%[v], :128]!			\n\t" // load NXYZW
			"vmul.f32 q0, q12, d8[0]				\n\t" // q0 = M0 * VXXXX
			"vmov q1, q15							\n\t" // DI q1 = M TRANSLATION
			"vmul.f32 q2, q12, d10[0]				\n\t" // q2 = M0 * NXXXX
			"vmla.f32 q1, q13, d8[1]				\n\t" // q1 = q1 + M1 * VYYYY
			"vmul.f32 q3, q13, d10[1]				\n\t" // q3 = M1 * NYYYY
			"vmla.f32 q0, q14, d9[0]				\n\t" // q0 = q0 + M2 * VZZZZ
			"vmla.f32 q2, q14, d11[0]				\n\t" // q2 = q2 + M2 * NZZZZ
			"vld1.32 {q4}, [%[v], :128]!			\n\t" // load TXYZW
			"vadd.f32 q0, q0, q1					\n\t" // q0 = q0 + q1
			"vmul.f32 q1, q13, d8[1]				\n\t" // q1 = M1 * TYYYY
			"vadd.f32 q2, q2, q3					\n\t" // q2 = q2 + q3
			"vst1.32 {q0}, [%[o], :128]!			\n\t" // store VXYZW
			"vmla.f32 q1, q14, d9[0]				\n\t" // q1 = q1 + M2 * TZZZZ
			"vmul.f32 q3, q12, d8[0]				\n\t" // q3 = M0 * TXXXX
			"vst1.32 {q2}, [%[o], :128]!			\n\t" // store NXYZW
			// stall q3
			"vadd.f32 q3, q3, q1					\n\t" // q3 = q3 + q1
			// stall q3
			"vmov s15, s19							\n\t" // store T.W
			// stall q3
			"vst1.32 {q3}, [%[o], :128]!			\n\t" // store TXYZW
			
		: [o] "+r" (outVerts), [v] "+r" (vertices)
		: [bone00] "r" (bone0), [bone01] "r" (bone0+8)
		: "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q12", "q13", "q14", "q15"

		);
	}
}

/*void SkinVerts1B(
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
	
// Most verts are 1 bone verts so I'm trying to keep the load store and fp pipes
// full on the neon by interleaving and hoping for dual issue to be happening.
	
#define FETCH_VERT_0 "vld1.32 {q0}, [%[v], :128]! \n\t"
#define FETCH_VERT_1 "vld1.32 {q1}, [%[v], :128]! \n\t"
	
#define FETCH_BONE_0_0 "vld1.32 {q2-q3}, [r9, :128]!  \n\t"
#define FETCH_BONE_0_1 "vld1.32 {q4-q5}, [r9, :128]!  \n\t"
#define FETCH_BONE_1_0 "vld1.32 {q6-q7}, [r9, :128]!  \n\t"
#define FETCH_BONE_1_1 "vld1.32 {q8-q9}, [r9, :128]!  \n\t"
	
#define VERT_0_OP1 "vmul.f32 q2, q2, d0[0] \n\t"
#define VERT_0_OP2 "vmla.f32 q5, q3, d0[1] \n\t"
#define VERT_0_OP3 "vmla.f32 q2, q4, d1[0] \n\t"
#define VERT_0_OP4 "vadd.f32 q5, q5, q2    \n\t"
#define VERT_0_STORE "vst1.32 {q5}, [%[o], :128]! \n\t"
	
#define VERT_1_OP1 "vmul.f32 q6, q6, d2[0] \n\t"
#define VERT_1_OP2 "vmla.f32 q9, q7, d2[1] \n\t"
#define VERT_1_OP3 "vmla.f32 q6, q8, d3[0] \n\t"
#define VERT_1_OP4 "vadd.f32 q9, q9, q6    \n\t"
#define VERT_1_STORE "vst1.32 {q9}, [%[o], :128]! \n\t"
	
	while (numVerts > 8)
	{
		numVerts -= 8;
		
		asm volatile (
			LOAD_BONE_PTR
			FETCH_VERT_0
			FETCH_BONE_0_0
			FETCH_BONE_0_1
			VERT_0_OP1
			LOAD_BONE_PTR
			FETCH_VERT_1
			VERT_0_OP2
			FETCH_BONE_1_0
			VERT_0_OP3
			FETCH_BONE_1_1
			VERT_0_OP4
			VERT_1_OP1
			VERT_0_STORE // 1
			VERT_1_OP2
			LOAD_BONE_PTR
			FETCH_VERT_0
			VERT_1_OP3
			FETCH_BONE_0_0
			VERT_1_OP4
			FETCH_BONE_0_1
			VERT_1_STORE // 2
			VERT_0_OP1
			LOAD_BONE_PTR
			FETCH_VERT_1
			VERT_0_OP2
			FETCH_BONE_1_0
			VERT_0_OP3
			FETCH_BONE_1_1
			VERT_0_OP4
			VERT_1_OP1
			VERT_0_STORE // 3
			VERT_1_OP2
			LOAD_BONE_PTR
			FETCH_VERT_0
			VERT_1_OP3
			FETCH_BONE_0_0
			VERT_1_OP4
			FETCH_BONE_0_1
			VERT_1_STORE // 4
			VERT_0_OP1
			LOAD_BONE_PTR
			FETCH_VERT_1
			VERT_0_OP2
			FETCH_BONE_1_0
			VERT_0_OP3
			FETCH_BONE_1_1
			VERT_0_OP4
			VERT_1_OP1
			VERT_0_STORE // 5
			VERT_1_OP2
			LOAD_BONE_PTR
			FETCH_VERT_0
			VERT_1_OP3
			FETCH_BONE_0_0
			VERT_1_OP4
			FETCH_BONE_0_1
			VERT_1_STORE // 6
			VERT_0_OP1
			LOAD_BONE_PTR
			FETCH_VERT_1
			VERT_0_OP2
			FETCH_BONE_1_0
			VERT_0_OP3
			FETCH_BONE_1_1
			VERT_0_OP4
			VERT_1_OP1
			VERT_0_STORE // 7
			VERT_1_OP2
			VERT_1_OP3
			VERT_1_OP4
			VERT_1_STORE // 8
		: [o] "+r" (outVerts), [v] "+r" (vertices), [bi] "+r" (boneIndices)
		: [bones] "r" (bones)
		: "cc", "r9", "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "s12", "s13", "s14", "s15", "s16", "s17", "s18", "s19", "s20", "s21", "s22", "s23", "s24", "s25", "s26", "s27", "s28", "s29", "s30", "s31", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d8", "d9", "d10", "d11", "d12", "d13", "d14", "d15", "d16", "d17", "d18", "d19", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9"
		);
	}
	
	while (numVerts > 4)
	{
		numVerts -= 4;
		
		asm volatile (
			LOAD_BONE_PTR
			FETCH_VERT_0
			FETCH_BONE_0_0
			FETCH_BONE_0_1
			VERT_0_OP1
			LOAD_BONE_PTR
			FETCH_VERT_1
			VERT_0_OP2
			FETCH_BONE_1_0
			VERT_0_OP3
			FETCH_BONE_1_1
			VERT_0_OP4
			VERT_1_OP1
			VERT_0_STORE // 1
			VERT_1_OP2
			LOAD_BONE_PTR
			FETCH_VERT_0
			VERT_1_OP3
			FETCH_BONE_0_0
			VERT_1_OP4
			FETCH_BONE_0_1
			VERT_1_STORE // 2
			VERT_0_OP1
			LOAD_BONE_PTR
			FETCH_VERT_1
			VERT_0_OP2
			FETCH_BONE_1_0
			VERT_0_OP3
			FETCH_BONE_1_1
			VERT_0_OP4
			VERT_1_OP1
			VERT_0_STORE // 3
			VERT_1_OP2
			VERT_1_OP3
			VERT_1_OP4
			VERT_1_STORE // 4			
		: [o] "+r" (outVerts), [v] "+r" (vertices), [bi] "+r" (boneIndices)
		: [bones] "r" (bones)
		: "cc", "r9", "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "s12", "s13", "s14", "s15", "s16", "s17", "s18", "s19", "s20", "s21", "s22", "s23", "s24", "s25", "s26", "s27", "s28", "s29", "s30", "s31", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d8", "d9", "d10", "d11", "d12", "d13", "d14", "d15", "d16", "d17", "d18", "d19", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9"
		);
	}

	while (numVerts-- > 0)
	{		
		asm volatile (
			LOAD_BONE_PTR
			FETCH_VERT_0
			FETCH_BONE_0_0
			FETCH_BONE_0_1
			VERT_0_OP1
			VERT_0_OP2
			VERT_0_OP3
			VERT_0_OP4
			VERT_0_STORE
		: [o] "+r" (outVerts), [v] "+r" (vertices), [bi] "+r" (boneIndices)
		: [bones] "r" (bones)
		: "cc", "r9", "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s12", "s13", "s14", "s15", "s16", "s17", "s18", "s19", "s20", "s21", "s22", "s23", "d0", "d1", "d2", "d3", "d6", "d7", "d8", "d9", "d10", "d11", "q0", "q1", "q3", "q4", "q5"

		);
	}
}*/

}

const SIMDDriver *SIMD_neon_bind()
{
	static SIMDDriver d;

	if (d.name[0])
		return &d;

	d = *SIMD_ref_bind();
	d.SkinVerts[0] = &SkinVerts1B;
	
	string::cpy(d.name, "SIMD_neon");
	return &d;
}

#endif

