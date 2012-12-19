/*! \file SIMD.cpp
	\copyright Copyright (c) 2010 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\ingroup runtime
*/

#include RADPCH
#include "SIMD.h"

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
