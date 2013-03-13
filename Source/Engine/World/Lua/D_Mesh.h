// D_Mesh.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../../Types.h"
#include "D_Asset.h"
#include <Runtime/PushPack.h>

namespace world {

class RADENG_CLASS D_Mesh : public D_Asset
{
public:
	typedef boost::shared_ptr<D_Mesh> Ref;

	static Ref New(const pkg::AssetRef &asset);

private:

	D_Mesh(const pkg::AssetRef &asset);
};

} // world

#include <Runtime/PopPack.h>
