/*! \file SIMD.h
	\copyright Copyright (c) 2010 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup runtime
*/

#pragma once

#include "Base.h"
#include <iostream>

//! Implements single instruction multiple data optimized routines
struct SIMDDriver {
	enum { 
		kAlignment = 16,
		kNumBoneFloats = 16
	};

	SIMDDriver() {
		name[0] = 0;
	}

	static void Select();

	//! Skins N weighed verts. The number of bone weights depends on the specific function bound.
	/*! \param outVerts 4 floats per vertex elements (numVerts*4 floats will be written)
						elements: vertex, normal, tangents
		\param bones 4x4 row-major matrix (16 floats)
		\param vertices 4 floats per vertex, preweighted by bone weight (N prescaled verts per vertex)
		\param boneIndices index into bone array (N indices per vertex)
		\param numVerts number of verts to skin
	*/
	typedef void (*FSkinVerts) (
		float *outVerts,
		const float *bones,
		const float *vertices,
		const U16 *boneIndices,
		int numVerts
	);

	FSkinVerts SkinVerts[4]; // [numBones]

	typedef void (*FBlendVerts) (
		float *outVerts,
		const float *srcVerts,
		const float *dstVerts,
		float frac,
		int numVerts
	);

	FBlendVerts BlendVerts;

	// accelerated writes for multiples of 16 bytes.
	// NOTE: src, dst, and len must be 16 byte aligned!
	typedef void (*FMemCopy16) (
		void *dst,
		const void *src,
		int len
	);

	FMemCopy16 MemCopy16;

	// Replicates len bytes from src count times into dst
	// NOTE: src, dst, and len must be 16 byte aligned!
	typedef void (*FMemRep16) (
		void *dst,
		const void *src,
		int len,
		int count
	);

	FMemRep16 MemRep16;
	
	char name[16];

};

extern RADRT_API const SIMDDriver *SIMD;

#if defined(RAD_OPT_TOOLS) || (defined(RAD_OPT_IOS_DEVICE) && !defined(RAD_OPT_SHIP))
void SIMDSkinTest(std::ostream &out);
#endif

