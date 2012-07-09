// T_MaterialPrecache.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "T_MaterialPrecache.h"
#include "D_Material.h"

namespace world {

T_MaterialPrecache::T_MaterialPrecache(World *world, const pkg::AssetRef &asset)
: T_Precache(world, asset)
{
}

int T_MaterialPrecache::PushResult(lua_State *L)
{
	if (result != pkg::SR_Success)
		return 0;

	D_Material::Ref dmat(D_Material::New(asset));
	dmat->Push(L);
	return 1;
}

} // world
