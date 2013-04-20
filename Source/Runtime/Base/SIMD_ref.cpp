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
	int numBones
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

		// tangents
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
				
		boneIndices += numBones;
	}
}

void SkinVerts1B(
	float *outVerts, 
	const float *bones, 
	const float *vertices,
	const U16 *boneIndices,
	int numVerts
) {
	SkinVerts(outVerts, bones, vertices, boneIndices, numVerts, 1);
}

void SkinVerts2B(
	float *outVerts, 
	const float *bones, 
	const float *vertices,
	const U16 *boneIndices,
	int numVerts
) {
	SkinVerts(outVerts, bones, vertices, boneIndices, numVerts, 2);
}

void SkinVerts3B(
	float *outVerts, 
	const float *bones, 
	const float *vertices,
	const U16 *boneIndices,
	int numVerts
) {
	SkinVerts(outVerts, bones, vertices, boneIndices, numVerts, 3);
}

void SkinVerts4B(
	float *outVerts, 
	const float *bones, 
	const float *vertices,
	const U16 *boneIndices,
	int numVerts
) {
	SkinVerts(outVerts, bones, vertices, boneIndices, numVerts, 4);
}

void BlendVerts(
	float *outVerts,
	const float *srcVerts,
	const float *dstVerts,
	float frac,
	int numVerts
) {
	RAD_ASSERT(IsAligned(outVerts, SIMDDriver::kAlignment));
	RAD_ASSERT(IsAligned(srcVerts, SIMDDriver::kAlignment));
	RAD_ASSERT(IsAligned(dstVerts, SIMDDriver::kAlignment));

	const int kNumFloats = numVerts * 12;

	if (frac < 0.01f) {
		memcpy(outVerts, srcVerts, kNumFloats*sizeof(float));
	} else if(frac > 0.99) {
		memcpy(outVerts, dstVerts, kNumFloats*sizeof(float));
	} else {
		// 12 floats per vertex
		for (int i = 0; i < kNumFloats; ++i) {
			outVerts[i] = math::Lerp(srcVerts[i], dstVerts[i], frac);
		}
	}
}

}

const SIMDDriver *SIMD_ref_bind() {
	static SIMDDriver d;

	if (d.name[0])
		return &d;

	d.SkinVerts[0] = &SkinVerts1B;
	d.SkinVerts[1] = &SkinVerts2B;
	d.SkinVerts[2] = &SkinVerts3B;
	d.SkinVerts[3] = &SkinVerts4B;
	d.BlendVerts = &BlendVerts;
	
	string::cpy(d.name, "SIMD_ref");
	return &d;
}

