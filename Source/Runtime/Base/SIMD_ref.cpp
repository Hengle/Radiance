/*! \file SIMD_ref.cpp
	\copyright Copyright (c) 2010 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\ingroup runtime
*/

#include RADPCH
#include "SIMD.h"
#include "../StringBase.h"

namespace {

#define MA(m, r, c) m[r*(SIMDDriver::NumBoneFloats/4)+c]

inline void Transform4x3(float *out, const float *mat, const float *vec)
{
	out[0] = 
		MA(mat, 0, 0) * vec[0] +
		MA(mat, 1, 0) * vec[1] +
		MA(mat, 2, 0) * vec[2] + 
		MA(mat, 3, 0) * vec[3];
	out[1] = 
		MA(mat, 0, 1) * vec[0] +
		MA(mat, 1, 1) * vec[1] +
		MA(mat, 2, 1) * vec[2] + 
		MA(mat, 3, 1) * vec[3];
	out[2] =
		MA(mat, 0, 2) * vec[0] +
		MA(mat, 1, 2) * vec[1] +
		MA(mat, 2, 2) * vec[2] + 
		MA(mat, 3, 2) * vec[3];
	out[3] = 1.f;
}

inline void AddVec3(float *out, const float *a, const float *b)
{
	out[0] = a[0] + b[0];
	out[1] = a[1] + b[1];
	out[2] = a[2] + b[2];
}

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

	float vtemp[3][4];

	for (int i = 0; i < numVerts; ++i)
	{
		const float *bone = &bones[boneIndices[0] * SIMDDriver::NumBoneFloats];
		Transform4x3(vtemp[0], bone, vertices);

		bone = &bones[boneIndices[1] * SIMDDriver::NumBoneFloats];
		Transform4x3(vtemp[1], bone, vertices+4);

		AddVec3(vtemp[2], vtemp[0], vtemp[1]);

		bone = &bones[boneIndices[2] * SIMDDriver::NumBoneFloats];
		Transform4x3(vtemp[0], bone, vertices+8);

		AddVec3(vtemp[1], vtemp[0], vtemp[2]);

		bone = &bones[boneIndices[3] * SIMDDriver::NumBoneFloats];
		Transform4x3(vtemp[0], bone, vertices+12);

		AddVec3(outVerts, vtemp[0], vtemp[1]);

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

	float vtemp[3][4];

	for (int i = 0; i < numVerts; ++i)
	{
		const float *bone = &bones[boneIndices[0] * SIMDDriver::NumBoneFloats];
		Transform4x3(vtemp[0], bone, vertices);

		bone = &bones[boneIndices[1] * SIMDDriver::NumBoneFloats];
		Transform4x3(vtemp[1], bone, vertices+4);

		AddVec3(vtemp[2], vtemp[0], vtemp[1]);

		bone = &bones[boneIndices[2] * SIMDDriver::NumBoneFloats];
		Transform4x3(vtemp[0], bone, vertices+8);

		AddVec3(outVerts, vtemp[0], vtemp[2]);

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

	float vtemp[2][4];

	for (int i = 0; i < numVerts; ++i)
	{
		const float *bone = &bones[boneIndices[0] * SIMDDriver::NumBoneFloats];
		Transform4x3(vtemp[0], bone, vertices);

		bone = &bones[boneIndices[1] * SIMDDriver::NumBoneFloats];
		Transform4x3(vtemp[1], bone, vertices+4);

		AddVec3(outVerts, vtemp[0], vtemp[1]);
		
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
		const float *bone = &bones[boneIndices[0] * SIMDDriver::NumBoneFloats];
		Transform4x3(outVerts, bone, vertices);

		vertices += 4;
		++boneIndices;
		outVerts += 4;
	}
}

}

const SIMDDriver *SIMD_ref_bind()
{
	static SIMDDriver d;

	d.SkinVerts[3] = &SkinVerts4;
	d.SkinVerts[2] = &SkinVerts3;
	d.SkinVerts[1] = &SkinVerts2;
	d.SkinVerts[0] = &SkinVerts1;

	string::cpy(d.name, "SIMD_ref");
	return &d;
}

