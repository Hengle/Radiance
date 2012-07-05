// T_SoundPrecache.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "T_SoundPrecache.h"
#include "../../Renderer/SkMesh.h"
#include "D_Sound.h"
#include "../../Sound/Sound.h"
#include "../World.h"

namespace world {

T_SoundPrecache::T_SoundPrecache(
	World *world, 
	const pkg::AssetRef &asset, 
	int maxInstances
)
: T_Precache(world, asset), m_maxInstances(maxInstances)
{
}

int T_SoundPrecache::PushResult(lua_State *L)
{
	if (result != pkg::SR_Success)
		return 0;

	Sound::Ref sound = world->sound->newSound(
		asset,
		m_maxInstances
	);
	if (sound)
	{
		D_Sound::Ref ds(D_Sound::New(asset, sound));
		ds->Push(L);
		return 1;
	}
	return 0;
}

} // world
