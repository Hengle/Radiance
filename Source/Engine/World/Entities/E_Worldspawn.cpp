// E_Worldspawn.cpp
// Copyright (c) 2010 Pyramind Labs LLC, All Rights Reserved
// Author: Joe Riedel (joeriedel@hotmail.com)
// See Radiance/LICENSE for licensing terms.

#include "E_Worldspawn.h"
#include "../World.h"

namespace world {

E_Worldspawn::E_Worldspawn()
{
}

E_Worldspawn::~E_Worldspawn()
{
}

int E_Worldspawn::Spawn(
	const Keys &keys,
	const xtime::TimeSlice &time,
	int flags
)
{
	world->worldspawn = shared_from_this();
	return pkg::SR_Success;
}

} // world
