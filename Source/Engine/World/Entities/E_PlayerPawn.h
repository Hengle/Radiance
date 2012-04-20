// E_PlayerPawn.h
// Copyright (c) 2010 Pyramind Labs LLC, All Rights Reserved
// Author: Joe Riedel (joeriedel@hotmail.com)
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "E_Pawn.h"
#include <Runtime/PushPack.h>

namespace world {

class RADENG_CLASS E_PlayerPawn : public E_Pawn
{
public:
	E_PlayerPawn();
	virtual ~E_PlayerPawn();
};

} // world

#include <Runtime/PopPack.h>
