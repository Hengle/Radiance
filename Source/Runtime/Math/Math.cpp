/*! \file Math.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup math
*/

#include RADPCH
#include "Math.h"

namespace math {
namespace {
#include "MathTables.inl"

inline void AngleFrac(float angle, int &low, int &high, float &frac) {
	static const float kAngleFrac = (float)(kNumSinFloats-1) / math::Constants<float>::_2_PI();
	RAD_ASSERT(angle >= -math::Constants<float>::PI());
	RAD_ASSERT(angle <=  math::Constants<float>::PI());

	float angleIndex = (angle + math::Constants<float>::PI()) * kAngleFrac;
	float angleFloor = FloorFastFloat(angleIndex);
	low = FloatToInt(angleIndex);
	frac = angleIndex - angleFloor;

	high = low+1;
	if (high >= kNumSinFloats)
		high = 0;

	RAD_ASSERT(low >= 0 && low < kNumSinFloats);
	RAD_ASSERT(high >= 0 && high < kNumSinFloats);
	RAD_ASSERT(frac >= 0.f && frac < 1.f);
}

}

float FastFloatRand() {
	static int s_index=0;
	int x = s_index;
	s_index = (s_index+1)&(kNumRandFloats-1);
	return kRandFloats[x];
}

float FastFloatRand(float min, float max){
	return min + FastFloatRand()*(max-min);
}

float FastSin(const float &angle) {
	int low, high;
	float frac;

	AngleFrac(angle, low, high, frac);
	return math::Lerp(kSinFloats[low], kSinFloats[high], frac);
}

float FastCos(const float &angle) {
	int low, high;
	float frac;

	AngleFrac(angle, low, high, frac);
	return math::Lerp(kCosFloats[low], kCosFloats[high], frac);
}

} // math
