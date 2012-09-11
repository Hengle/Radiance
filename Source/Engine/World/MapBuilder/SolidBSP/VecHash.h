// VecHash.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "MapTypes.h"
#include <Runtime/Container/HashMap.h>
#include <Runtime/PushPack.h>

namespace tools {
namespace solid_bsp {

inline size_t HashVec(const Vec3 &v)
{
	size_t a = *((size_t*)&v[0]);
	size_t b = *((size_t*)&v[1]);
	size_t c = *((size_t*)&v[2]);
	return a ^ b ^ c;
}

inline size_t HashVec(const Vec4 &v)
{
	size_t a = *((size_t*)&v[0]);
	size_t b = *((size_t*)&v[1]);
	size_t c = *((size_t*)&v[2]);
	size_t d = *((size_t*)&v[3]);
	return a ^ b ^ c ^ d;
}

#define VEC_WELD_DIST 0.000005

inline bool operator < (const Vec3 &a, const Vec3 &b)
{
	if (a[0] + Vec3::ValueType(VEC_WELD_DIST) >= b[0]) return false;
	if (a[1] + Vec3::ValueType(VEC_WELD_DIST) >= b[1]) return false;
	if (a[2] + Vec3::ValueType(VEC_WELD_DIST) >= b[2]) return false;
	return true;
}

inline bool operator < (const Vec4 &a, const Vec4 &b)
{
	if (a[0] + Vec3::ValueType(VEC_WELD_DIST) >= b[0]) return false;
	if (a[1] + Vec3::ValueType(VEC_WELD_DIST) >= b[1]) return false;
	if (a[2] + Vec3::ValueType(VEC_WELD_DIST) >= b[2]) return false;
	if (a[3] + Vec3::ValueType(VEC_WELD_DIST) >= b[3]) return false;
	return true;
}

#undef VEC_WELD_DIST

} // box_bsp
} // tools

#include <Runtime/PopPack.h>

namespace RAD_STDEXT {

#if defined(RAD_OPT_GCC)

template<>
struct hash<tools::solid_bsp::Vec3>
{
	size_t operator() (const tools::solid_bsp::Vec3 &v) const
	{
		return tools::solid_bsp::HashVec(v);
	}
};

template<>
struct hash<tools::solid_bsp::Vec4>
{
	size_t operator() (const tools::solid_bsp::Vec4 &v) const
	{
		return tools::solid_bsp::HashVec(v);
	}
};

#else

template<>
inline size_t hash_value(const tools::solid_bsp::Vec3 &v)
{
	return tools::solid_bsp::HashVec(v);
}

template<>
inline size_t hash_value(const tools::solid_bsp::Vec4 &v)
{
	return tools::solid_bsp::HashVec(v);
}

#endif

} // RAD_STDEXT
