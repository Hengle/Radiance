// T_MeshPrecache.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "T_Precache.h"
#include <Runtime/PushPack.h>

namespace world {

class RADENG_CLASS T_MeshPrecache : public T_Precache {
public:
	typedef boost::shared_ptr<T_MeshPrecache> Ref;

	T_MeshPrecache(World *world, const pkg::AssetRef &asset);

protected:

	virtual int PushResult(lua_State *L);
};

} // world

#include <Runtime/PopPack.h>
