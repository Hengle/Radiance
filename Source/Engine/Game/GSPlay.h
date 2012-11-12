// GSPlay.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "Game.h"
#include "GSPriority.h"
#include <Runtime/PushPack.h>

class RADENG_CLASS GSPlay : public Game::Tickable {
public:
	static Game::Tickable::Ref New();
	enum { Priority = GSP_Default };
	GSPlay();

	virtual int Tick(Game &game, float dt, const xtime::TimeSlice &time, int flags);
};

#include <Runtime/PopPack.h>
