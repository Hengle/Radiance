/*! \file SIMD.cpp
	\copyright Copyright (c) 2010 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\ingroup runtime
*/

#include RADPCH
#include "SIMD.h"

#if defined(RAD_OPT_TOOLS)
#include "../Time.h"
#include "../Math.h"
#endif

RADRT_API const SIMDDriver *SIMD = 0;

const SIMDDriver *SIMD_ref_bind();
const SIMDDriver *SIMD_sse2_bind();
const SIMDDriver *SIMD_neon_bind();

void SIMDDriver::Select() {
#if defined(RAD_OPT_WINX) && !defined(_WIN64)
	SIMD = SIMD_sse2_bind();
#elif defined(__ARM_NEON__)
	SIMD = SIMD_ref_bind(); // for now.
#else
	SIMD = SIMD_ref_bind();
#endif
}

#if defined(RAD_OPT_TOOLS)|| (defined(RAD_OPT_IOS_DEVICE) && !defined(RAD_OPT_SHIP))
struct SkinTestData {

	SkinTestData() {
		outVerts = 0;
		bones = 0;
		vertices = 0;
		boneIndices = 0;
	}

	~SkinTestData() {
		Free();
	}

	float *outVerts;
	float *bones;
	float *vertices;
	U16 *boneIndices;
	int numChannels;
	int bonesPerVert;

	void Create(int numVerts, int _bonesPerVert, int numBones, int _numChannels) {
		Free();
		numChannels = _numChannels;
		bonesPerVert = _bonesPerVert;
		outVerts = (float*)safe_zone_malloc(ZRuntime, sizeof(float)*(8+(4*((int)numChannels)))*numVerts, 0, SIMDDriver::kAlignment);
		memset(outVerts, 0, zone_malloc_size(outVerts));
		bones = (float*)safe_zone_malloc(ZRuntime, sizeof(float)*SIMDDriver::kNumBoneFloats*numBones, 0, SIMDDriver::kAlignment);
		memset(bones, 0, zone_malloc_size(bones));
		vertices = (float*)safe_zone_malloc(ZRuntime, sizeof(float)*(8+(4*((int)numChannels)))*numVerts*bonesPerVert, 0, SIMDDriver::kAlignment);
		memset(vertices, 0, zone_malloc_size(vertices));
		boneIndices = (U16*)safe_zone_malloc(ZRuntime, sizeof(U16)*numVerts*bonesPerVert, 0, SIMDDriver::kAlignment);

		for (int i = 0; i < numVerts; ++i)
			boneIndices[i] = (U16)(rand() % numBones);
	}

	void Free() {
		if (outVerts)
			zone_free(outVerts);
		if (bones)
			zone_free(bones);
		if (vertices)
			zone_free(vertices);
		if (boneIndices)
			zone_free(boneIndices);

		outVerts = 0;
		bones = 0;
		vertices = 0;
		boneIndices = 0;
	}
};

inline int VertsPerSecond(xtime::MicroTimer &timer, int numVerts) {
	float s = xtime::Constants<float>::MicrosToSecond(timer.Elapsed());
	return (int)math::Floor<float>((((float)numVerts) / s) + 0.5f);
}

void SIMDSkinTest(std::ostream &out) {
	enum {
		kNumVerts = 64*Kilo,
		kNumBones = 256,
		kMultiplier = (2*Meg) / kNumVerts
	};
	xtime::MicroTimer refTime, simdTime;
	SkinTestData skinData;

	const SIMDDriver *ref = SIMD_ref_bind();
	
	out << "******** SIMDSkinTest ********" << std::endl;
	
	// 1B1T
	skinData.Create(kNumVerts, 1, kNumBones, 1);
	refTime.Start();
	for (int i = 0; i < kMultiplier; ++i) {
		ref->SkinVerts[0](skinData.outVerts, skinData.bones, skinData.vertices, skinData.boneIndices, kNumVerts);
	}
	refTime.Stop();
	simdTime.Start();
	for (int i = 0; i < kMultiplier; ++i) {
		SIMD->SkinVerts[0](skinData.outVerts, skinData.bones, skinData.vertices, skinData.boneIndices, kNumVerts);
	}
	simdTime.Stop();
	out << "(1B1T) " << ref->name << ": " << VertsPerSecond(refTime, kNumVerts*kMultiplier) << " (vps), " << SIMD->name << ": " << VertsPerSecond(simdTime, kNumVerts*kMultiplier) << " (vps)." << std::endl;

	// 2B1T
	skinData.Create(kNumVerts, 2, kNumBones, 1);
	refTime.Start();
	for (int i = 0; i < kMultiplier; ++i) {
		ref->SkinVerts[1](skinData.outVerts, skinData.bones, skinData.vertices, skinData.boneIndices, kNumVerts);
	}
	refTime.Stop();
	simdTime.Start();
	for (int i = 0; i < kMultiplier; ++i) {
		SIMD->SkinVerts[1](skinData.outVerts, skinData.bones, skinData.vertices, skinData.boneIndices, kNumVerts);
	}
	simdTime.Stop();
	out << "(2B1T) " << ref->name << ": " << VertsPerSecond(refTime, kNumVerts*kMultiplier) << " (vps), " << SIMD->name << ": " << VertsPerSecond(simdTime, kNumVerts*kMultiplier) << " (vps)." << std::endl;

	// 3B1T
	skinData.Create(kNumVerts, 3, kNumBones, 1);
	refTime.Start();
	for (int i = 0; i < kMultiplier; ++i) {
		ref->SkinVerts[2](skinData.outVerts, skinData.bones, skinData.vertices, skinData.boneIndices, kNumVerts);
	}
	refTime.Stop();
	simdTime.Start();
	for (int i = 0; i < kMultiplier; ++i) {
		SIMD->SkinVerts[2](skinData.outVerts, skinData.bones, skinData.vertices, skinData.boneIndices, kNumVerts);
	}
	simdTime.Stop();
	out << "(3B1T) " << ref->name << ": " << VertsPerSecond(refTime, kNumVerts*kMultiplier) << " (vps), " << SIMD->name << ": " << VertsPerSecond(simdTime, kNumVerts*kMultiplier) << " (vps)." << std::endl;
	
	// 4B1T
	skinData.Create(kNumVerts, 4, kNumBones, 1);
	refTime.Start();
	for (int i = 0; i < kMultiplier; ++i) {
		ref->SkinVerts[3](skinData.outVerts, skinData.bones, skinData.vertices, skinData.boneIndices, kNumVerts);
	}
	refTime.Stop();
	simdTime.Start();
	for (int i = 0; i < kMultiplier; ++i) {
		SIMD->SkinVerts[3](skinData.outVerts, skinData.bones, skinData.vertices, skinData.boneIndices, kNumVerts);
	}
	simdTime.Stop();
	out << "(4B1T) " << ref->name << ": " << VertsPerSecond(refTime, kNumVerts*kMultiplier) << " (vps), " << SIMD->name << ": " << VertsPerSecond(simdTime, kNumVerts*kMultiplier) << " (vps)." << std::endl;
	
	out << "******************************" << std::endl;
}

#endif