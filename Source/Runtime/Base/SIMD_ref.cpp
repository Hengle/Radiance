/*! \file SIMD_ref.cpp
	\copyright Copyright (c) 2010 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\ingroup runtime
*/

#include RADPCH
#include "SIMD.h"
#include "../StringBase.h"

namespace {

#define MA(m, r, c) m[r*(SIMDDriver::kNumBoneFloats/4)+c]

inline void Transform4x3(float *out, const float *mat, const float *vec) {
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

inline void Transform3x3(float *out, const float *mat, const float *vec) {
	out[0] = 
		MA(mat, 0, 0) * vec[0] +
		MA(mat, 1, 0) * vec[1] +
		MA(mat, 2, 0) * vec[2];
	out[1] = 
		MA(mat, 0, 1) * vec[0] +
		MA(mat, 1, 1) * vec[1] +
		MA(mat, 2, 1) * vec[2];
	out[2] =
		MA(mat, 0, 2) * vec[0] +
		MA(mat, 1, 2) * vec[1] +
		MA(mat, 2, 2) * vec[2];
	out[3] = vec[3];
}

inline void AddVec3(float *out, const float *a, const float *b) {
	out[0] = a[0] + b[0];
	out[1] = a[1] + b[1];
	out[2] = a[2] + b[2];
}

inline void AddVec4(float *out, const float *a, const float *b) {
	out[0] = a[0] + b[0];
	out[1] = a[1] + b[1];
	out[2] = a[2] + b[2];
	out[3] = a[3] + b[3];
}

inline void ZeroVec3(float *out) {
	out[0] = 0.f;
	out[1] = 0.f;
	out[2] = 0.f;
	out[3] = 1.f;
}

inline void ZeroVec4(float *out) {
	out[0] = 0.f;
	out[1] = 0.f;
	out[2] = 0.f;
	out[3] = 0.f;
}

inline void StoreVec4(float *out, float *a) {
	out[0] = a[0];
	out[1] = a[1];
	out[2] = a[2];
	out[3] = a[3];
}

void SkinVerts(
	float *outVerts, 
	const float *bones, 
	const float *vertices,
	const U16 *boneIndices,
	int numVerts,
	int numBones,
	int numTangents
)
{
	RAD_ASSERT(IsAligned(outVerts, SIMDDriver::kAlignment));
	RAD_ASSERT(IsAligned(bones, SIMDDriver::kAlignment));
	RAD_ASSERT(IsAligned(vertices, SIMDDriver::kAlignment));
	RAD_ASSERT(IsAligned(boneIndices, SIMDDriver::kAlignment));

	float acc[4];
	float out[4];

	for (int i = 0; i < numVerts; ++i) {
		ZeroVec3(acc);
		
		// verts

		for (int b = 0; b < numBones; ++b) {
			const float *bone = &bones[boneIndices[b] * SIMDDriver::kNumBoneFloats];
			Transform4x3(out, bone, vertices);
			AddVec3(acc, acc, out);
			vertices += 4;
		}

		StoreVec4(outVerts, acc);
		outVerts += 4;

		// normals.

		ZeroVec3(acc);

		for (int b = 0; b < numBones; ++b) {
			const float *bone = &bones[boneIndices[b] * SIMDDriver::kNumBoneFloats];
			Transform3x3(out, bone, vertices);
			AddVec3(acc, acc, out);
			vertices += 4;
		}

		StoreVec4(outVerts, acc);
		outVerts += 4;

		for (int k = 0; k < numTangents; ++k) {
			ZeroVec3(acc);

			for (int b = 0; b < numBones; ++b) {
				const float *bone = &bones[boneIndices[b] * SIMDDriver::kNumBoneFloats];
				Transform3x3(out, bone, vertices);
				AddVec3(acc, acc, out);
				acc[3] = out[3];
				vertices += 4;
			}

			StoreVec4(outVerts, acc);
			outVerts += 4;
		}
		
		boneIndices += numBones;
	}
}

void SkinVerts1B1T(
	float *outVerts, 
	const float *bones, 
	const float *vertices,
	const U16 *boneIndices,
	int numVerts
) {
	SkinVerts(outVerts, bones, vertices, boneIndices, numVerts, 1, 1);
}

void SkinVerts1B2T(
	float *outVerts, 
	const float *bones, 
	const float *vertices,
	const U16 *boneIndices,
	int numVerts
) {
	SkinVerts(outVerts, bones, vertices, boneIndices, numVerts, 1, 2);
}

void SkinVerts2B1T(
	float *outVerts, 
	const float *bones, 
	const float *vertices,
	const U16 *boneIndices,
	int numVerts
) {
	SkinVerts(outVerts, bones, vertices, boneIndices, numVerts, 2, 1);
}

void SkinVerts2B2T(
	float *outVerts, 
	const float *bones, 
	const float *vertices,
	const U16 *boneIndices,
	int numVerts
) {
	SkinVerts(outVerts, bones, vertices, boneIndices, numVerts, 2, 2);
}

void SkinVerts3B1T(
	float *outVerts, 
	const float *bones, 
	const float *vertices,
	const U16 *boneIndices,
	int numVerts
) {
	SkinVerts(outVerts, bones, vertices, boneIndices, numVerts, 3, 1);
}

void SkinVerts3B2T(
	float *outVerts, 
	const float *bones, 
	const float *vertices,
	const U16 *boneIndices,
	int numVerts
) {
	SkinVerts(outVerts, bones, vertices, boneIndices, numVerts, 3, 2);
}

void SkinVerts4B1T(
	float *outVerts, 
	const float *bones, 
	const float *vertices,
	const U16 *boneIndices,
	int numVerts
) {
	SkinVerts(outVerts, bones, vertices, boneIndices, numVerts, 4, 1);
}

void SkinVerts4B2T(
	float *outVerts, 
	const float *bones, 
	const float *vertices,
	const U16 *boneIndices,
	int numVerts
) {
	SkinVerts(outVerts, bones, vertices, boneIndices, numVerts, 4, 2);
}

}

const SIMDDriver *SIMD_ref_bind() {
	static SIMDDriver d;

	if (d.name[0])
		return &d;

	d.SkinVerts[0][0] = &SkinVerts1B1T;
	d.SkinVerts[0][1] = &SkinVerts1B2T;
	d.SkinVerts[1][0] = &SkinVerts2B1T;
	d.SkinVerts[1][1] = &SkinVerts2B2T;
	d.SkinVerts[2][0] = &SkinVerts3B1T;
	d.SkinVerts[2][1] = &SkinVerts3B2T;
	d.SkinVerts[3][0] = &SkinVerts4B1T;
	d.SkinVerts[3][1] = &SkinVerts4B2T;

	string::cpy(d.name, "SIMD_ref");
	return &d;
}

