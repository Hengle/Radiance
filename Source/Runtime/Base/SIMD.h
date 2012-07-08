/*! \file SIMD.h
	\copyright Copyright (c) 2010 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup runtime
*/

#pragma once

#include "Base.h"

//! Implements single instruction multiple data optimized routines
struct SIMDDriver
{
	enum 
	{ 
		Alignment = 16,
		NumBoneFloats = 16
	};

	static void Select();

	//! Skins N weighed verts. The number of bone weights depends on the specific function bound.
	/*! \param outVerts 4 floats per vertex (numVerts*4 floats will be written)
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

	FSkinVerts SkinVerts[4];

	char name[16];

};

extern RADRT_API const SIMDDriver *SIMD;

