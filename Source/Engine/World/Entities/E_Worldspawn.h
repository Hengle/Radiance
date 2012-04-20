// E_WorldSpawn.h
// Copyright (c) 2010 Pyramind Labs LLC, All Rights Reserved
// Author: Joe Riedel (joeriedel@hotmail.com)
// See Radiance/LICENSE for licensing terms.

#include "../Entity.h"
#include <Runtime/PushPack.h>

namespace world {

class RADENG_CLASS E_Worldspawn : public Entity
{
public:
	E_Worldspawn();
	virtual ~E_Worldspawn();

	virtual int Spawn(
		const Keys &keys,
		const xtime::TimeSlice &time,
		int flags
	);
};

} // world

#include <Runtime/PopPack.h>

