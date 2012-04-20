// GSLoadMap.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "Game.h"
#include "GSPriority.h"
#include "../Assets/MapAsset.h"
#include <Runtime/PushPack.h>

class RADENG_CLASS GSLoadMap : public Game::Tickable
{
public:
	static Game::Tickable::Ref New(int mapId, int slot, bool play, bool loadScreen);
	enum { Priority = GSP_Default };
	GSLoadMap(int mapId, int slot, bool play, bool loadScreen);

	virtual int Tick(Game &game, float dt, const xtime::TimeSlice &time, int flags);

protected:

	RAD_DECLARE_READONLY_PROPERTY(GSLoadMap, loadScreen, bool);
	RAD_DECLARE_READONLY_PROPERTY(GSLoadMap, mapAsset, const asset::MapAsset::Ref &);

	virtual void Draw(Game &game, float dt) {}

private:

	RAD_DECLARE_GET(loadScreen, bool) { return m_loadScreen; }
	RAD_DECLARE_GET(mapAsset, const asset::MapAsset::Ref&) { return m_mapAsset; }

	int m_mapId;
	int m_slot;
	bool m_play;
	bool m_loadScreen;
	pkg::Asset::Ref m_map;
	asset::MapAsset::Ref m_mapAsset;
};

#include <Runtime/PopPack.h>
