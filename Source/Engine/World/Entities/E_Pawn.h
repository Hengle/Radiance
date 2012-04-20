// E_Pawn.h
// Copyright (c) 2010 Pyramind Labs LLC, All Rights Reserved
// Author: Joe Riedel (joeriedel@hotmail.com)
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Entity.h"
#include <Runtime/PushPack.h>

namespace world {

class RADENG_CLASS E_Pawn : public Entity
{
public:
	E_Pawn();
	virtual ~E_Pawn();
};

} // world

#include <Runtime/PopPack.h>
