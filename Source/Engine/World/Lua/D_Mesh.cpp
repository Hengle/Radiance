// D_Mesh.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "D_Mesh.h"

namespace world {

D_Mesh::Ref D_Mesh::New(const pkg::AssetRef &asset) {
	return Ref(new (ZWorld) D_Mesh(asset));
}

D_Mesh::D_Mesh(const pkg::AssetRef &asset) : D_Asset(asset) {
}

} // world
