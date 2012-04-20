// StringTest.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include <Runtime/Runtime.h>
#include <Runtime/Base/SIMD.h>
#include <Runtime/Time.h>
#include "../UTCommon.h"

const SIMDDriver *SIMD_ref_bind();
const SIMDDriver *SIMD_sse2_bind();

namespace ut
{
	enum
	{
		NumBones = 256, // must be power of 2
		NumVerts = 5*Meg,
		MaxBonesPerVert = 4
	};

	void IdentBone(float *bone)
	{
		memset(bone, 0, sizeof(float)*16);
		bone[0*4+0] = 1.f;
		bone[1*4+1] = 1.f;
		bone[2*4+2] = 1.f;
		bone[3*4+2] = 1.f;
	}

	void SIMDTest()
	{
		Begin("SIMDTest");
		
		std::cout << "Initializing" << std::endl;

		const SIMDDriver *refDriver = SIMD_ref_bind();
		const SIMDDriver *sse2Driver = SIMD_sse2_bind();

		U16 *indexPrecache = (U16*)safe_aligned_malloc(sizeof(U16)*NumBones, 0, SIMDDriver::Alignment);
		for (int i = 0; i < NumBones; ++i)
			indexPrecache[i] = rand() % NumBones;

		U16 *boneIndices = (U16*)safe_aligned_malloc(sizeof(U16)*NumVerts*MaxBonesPerVert, 0, SIMDDriver::Alignment);
		for (int i = 0; i < NumVerts*MaxBonesPerVert; ++i)
			boneIndices[i] = indexPrecache[i & (NumBones-1)];

		float *outVerts = (float*)safe_aligned_malloc(sizeof(float)*NumVerts*4, 0, SIMDDriver::Alignment);
		memset(outVerts, 0, sizeof(float)*NumVerts*4);

		float *inVerts = (float*)safe_aligned_malloc(sizeof(float)*NumVerts*MaxBonesPerVert*4, 0, SIMDDriver::Alignment);
		memset(inVerts, 0, sizeof(float)*NumVerts*MaxBonesPerVert*4);

		float *bones = (float*)safe_aligned_malloc(sizeof(float)*NumBones*16, 0, SIMDDriver::Alignment);
		for (int i = 0; i < NumBones; ++i)
			IdentBone(bones + i*16);

		for (int i = 0; i < MaxBonesPerVert; ++i)
		{
			std::cout << "Skin" << (i+1) << " ref_driver test..." << std::endl;
			unsigned int start = xtime::ReadMicroseconds();

			refDriver->SkinVerts[i](
				outVerts,
				bones,
				inVerts,
				boneIndices,
				NumVerts
			);

			unsigned int end = xtime::ReadMicroseconds();
			std::cout << "Skin" << (i+1) << " ref_driver total(" << (end-start) << "us), avg(" << ((end-start)/NumVerts) << "us)" << std::endl;
		
			std::cout << "Skin" << (i+1) << " sse2_driver test..." << std::endl;
			start = xtime::ReadMicroseconds();

			sse2Driver->SkinVerts[i](
				outVerts,
				bones,
				inVerts,
				boneIndices,
				NumVerts
			);

			end = xtime::ReadMicroseconds();
			std::cout << "Skin" << (i+1) << " sse2_driver total(" << (end-start) << "us), avg(" << ((end-start)/NumVerts) << "us)" << std::endl;
		}

		aligned_free(indexPrecache);
		aligned_free(boneIndices);
		aligned_free(outVerts);
		aligned_free(inVerts);
		aligned_free(bones);
	}
}
