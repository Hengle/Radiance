// T_TypefacePrecache.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "T_TypefacePrecache.h"
#include "D_Typeface.h"

namespace world {

T_TypefacePrecache::T_TypefacePrecache(World *world, const pkg::AssetRef &asset)
: T_Precache(world, asset)
{
}

int T_TypefacePrecache::PushResult(lua_State *L)
{
	if (result != pkg::SR_Success)
		return 0;

	D_Typeface::Ref typeface(D_Typeface::New(asset));
	typeface->Push(L);
	return 1;
}

} // world
