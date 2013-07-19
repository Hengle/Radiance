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
	SIMD = SIMD_neon_bind();
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
		memset(outVerts, 0, (size_t)zone_malloc_size(outVerts));
		bones = (float*)safe_zone_malloc(ZRuntime, sizeof(float)*SIMDDriver::kNumBoneFloats*numBones, 0, SIMDDriver::kAlignment);
		memset(bones, 0, (size_t)zone_malloc_size(bones));
		const int kFloatsPerVert = 8+(4*numChannels);
		vertices = (float*)safe_zone_malloc(ZRuntime, sizeof(float)*kFloatsPerVert*numVerts*bonesPerVert, 0, SIMDDriver::kAlignment);
		memset(vertices, 0, (size_t)zone_malloc_size(vertices));
		boneIndices = (U16*)safe_zone_malloc(ZRuntime, sizeof(U16)*numVerts*bonesPerVert, 0, SIMDDriver::kAlignment);

		for (int i = 0; i < numBones; ++i) {
			float *m = bones + (i*SIMDDriver::kNumBoneFloats);
			m[0] = 0.f;
			m[1] = 0.f;
			m[2] = 0.f;
			m[3] = 1.f;
			m[4] = 0.f;
			m[5] = 0.f;
			m[6] = 0.f;
			m[7] = 1.f;
			m[8] = 0.f;
			m[9] = 0.f;
			m[10] = 0.f;
			m[11] = 1.f;
			m[12] = 0.f;
			m[13] = 0.f;
			m[14] = 0.f;
			m[15] = 1.f;
		}
		
		for (int i = 0; i < numVerts; ++i) {
			boneIndices[i] = (U16)(rand() % numBones);
			float *v = vertices + (kFloatsPerVert*bonesPerVert*i);
			for (int k = 0; k < kFloatsPerVert; ++k) {
				for (int j = 0; j < bonesPerVert; ++j) {
					v[0] = (rand() / (float)RAND_MAX) * 5000.f;
					v[1] = (rand() / (float)RAND_MAX) * 5000.f;
					v[2] = (rand() / (float)RAND_MAX) * 5000.f;
					v[3] = 1.f;
					v += 4;
				}
			}
		}
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
		kNumVerts = 27*kKilo, // doing an odd number
		kNumBones = 256,
		kMultiplier = (2*kMeg) / kNumVerts
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
	
	int vps[2];
	vps[0] = VertsPerSecond(refTime, kNumVerts*kMultiplier);
	vps[1] = VertsPerSecond(simdTime, kNumVerts*kMultiplier);
	float pct = ((vps[1] / (float)vps[0]) - 1.f) * 100.f;
	
	out << "(1B1T) " << ref->name << ": " << vps[0] << " (vps), " << SIMD->name << ": " << vps[1] << " (vps). " << pct << "%" << std::endl;

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
	
	vps[0] = VertsPerSecond(refTime, kNumVerts*kMultiplier);
	vps[1] = VertsPerSecond(simdTime, kNumVerts*kMultiplier);
	pct = ((vps[1] / (float)vps[0]) - 1.f) * 100.f;
	
	out << "(2B1T) " << ref->name << ": " << vps[0] << " (vps), " << SIMD->name << ": " << vps[1] << " (vps). " << pct << "%" << std::endl;

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
	
	vps[0] = VertsPerSecond(refTime, kNumVerts*kMultiplier);
	vps[1] = VertsPerSecond(simdTime, kNumVerts*kMultiplier);
	pct = ((vps[1] / (float)vps[0]) - 1.f) * 100.f;
	
	out << "(3B1T) " << ref->name << ": " << vps[0] << " (vps), " << SIMD->name << ": " << vps[1] << " (vps). " << pct << "%" << std::endl;
	
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
	
	vps[0] = VertsPerSecond(refTime, kNumVerts*kMultiplier);
	vps[1] = VertsPerSecond(simdTime, kNumVerts*kMultiplier);
	pct = ((vps[1] / (float)vps[0]) - 1.f) * 100.f;
	
	out << "(4B1T) " << ref->name << ": " << vps[0] << " (vps), " << SIMD->name << ": " << vps[1] << " (vps). " << pct << "%" << std::endl;
	
	out << "******************************" << std::endl;
}

#endif