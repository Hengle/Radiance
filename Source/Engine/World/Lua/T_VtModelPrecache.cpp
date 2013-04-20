// T_SkModelPrecache.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "T_VtModelPrecache.h"
#include "../../Renderer/VtMesh.h"
#include "D_VtModel.h"

namespace world {

T_VtModelPrecache::T_VtModelPrecache(World *world, const pkg::AssetRef &asset)
: T_Precache(world, asset) {
}

int T_VtModelPrecache::PushResult(lua_State *L) {
	if (result != pkg::SR_Success)
		return 0;

	r::VtMesh::Ref vtMesh = r::VtMesh::New(asset);

	// P_Trim
	asset->Process(
		xtime::TimeSlice::Infinite,
		pkg::P_Trim
	);

	if (vtMesh) {
		// push lua vtmesh data element
		D_VtModel::Ref dvtm(D_VtModel::New(vtMesh));
		dvtm->Push(L);
		return 1;
	}
	return 0;
}

} // world
