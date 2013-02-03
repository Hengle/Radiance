// T_MeshPrecache.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "T_MeshPrecache.h"
#include "../../Renderer/Mesh.h"
#include "D_Mesh.h"

namespace world {

T_MeshPrecache::T_MeshPrecache(World *world, const pkg::AssetRef &asset)
: T_Precache(world, asset) {
}

int T_MeshPrecache::PushResult(lua_State *L) {
	if (result != pkg::SR_Success)
		return 0;

	r::MeshBundle::Ref mesh = r::MeshBundle::New(asset);

	// P_Trim
	asset->Process(
		xtime::TimeSlice::Infinite,
		pkg::P_Trim
	);

	if (mesh) {
		// push lua mesh data element
		D_Mesh::Ref dmesh(D_Mesh::New(mesh));
		dmesh->Push(L);
		return 1;
	}
	return 0;
}

} // world
