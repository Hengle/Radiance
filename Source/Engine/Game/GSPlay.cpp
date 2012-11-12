// GSPlay.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "GSPlay.h"
#include "../World/World.h"
#include "../App.h"
#include "../Engine.h"
#include "../Sound/Sound.h"
#include "../UI/UIWidget.h"

GSPlay::GSPlay() : Game::Tickable(Priority) {
}

int GSPlay::Tick(Game &game, float dt, const xtime::TimeSlice &time, int flags) {
	if (firstTick) {
		App::Get()->ClearFrameHistory();
		dt = 0.001f;
	}

	int vpx, vpy, vpw, vph;
	game.Viewport(vpx, vpy, vpw, vph);
	game.world->uiRoot->SetDestViewport(vpx, vpy, vpw, vph);
	game.ProcessInput();
	game.world->Tick(dt);
	game.world->Draw();

	int slot;
	String name;
	world::UnloadDisposition ud;
	bool loadScreen;

	if (game.world->PendingLoadRequest(name, ud, loadScreen)) {
		game.LoadMap(name.c_str, game.world->slot, ud, true, loadScreen);
		return TickPop;
	}

	if (game.world->PendingSwitchLoadRequest(slot, name, ud, loadScreen)) {
		game.Switch(slot);
		game.LoadMap(name.c_str, game.world->slot, ud, true, loadScreen);
		return TickPop;
	}

	if (game.world->PendingReturnRequest())
		game.Return();

	if(game.world->PendingUnloadSlot(slot))
		game.Unload(slot);

	if(game.world->PendingSwitch(slot))
		game.Switch(slot);
	
	return TickNext;
}
