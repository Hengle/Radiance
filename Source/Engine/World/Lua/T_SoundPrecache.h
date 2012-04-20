// T_SoundPrecache.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "T_Precache.h"
#include <Runtime/PushPack.h>

namespace world {

class RADENG_CLASS T_SoundPrecache : public T_Precache
{
public:
	typedef boost::shared_ptr<T_SoundPrecache> Ref;
	
	T_SoundPrecache(World *world, const pkg::AssetRef &asset, int maxInstances);

protected:

	virtual int PushResult(lua_State *L);

private:

	int m_maxInstances;
};

} // world

#include <Runtime/PopPack.h>
