// D_Mesh.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "D_Mesh.h"

namespace world {

D_Mesh::Ref D_Mesh::New(const r::MeshBundle::Ref &bundle)
{
	return Ref(new (ZWorld) D_Mesh(bundle));
}

D_Mesh::D_Mesh(const r::MeshBundle::Ref &bundle) : D_Asset(bundle->asset),
m_bundle(bundle)
{
}

} // world
