// T_SkModelPrecache.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "T_SkModelPrecache.h"
#include "../../Renderer/SkMesh.h"
#include "D_SkModel.h"

namespace world {

T_SkModelPrecache::T_SkModelPrecache(World *world, const pkg::AssetRef &asset)
: T_Precache(world, asset) {
}

int T_SkModelPrecache::PushResult(lua_State *L) {
	if (result != pkg::SR_Success)
		return 0;

	r::SkMesh::Ref skMesh = r::SkMesh::New(asset);

	// P_Trim
	asset->Process(
		xtime::TimeSlice::Infinite,
		pkg::P_Trim
	);

	if (skMesh) {
		// push lua skmesh data element
		D_SkModel::Ref dskm(D_SkModel::New(skMesh));
		dskm->Push(L);
		return 1;
	}
	return 0;
}

} // world
