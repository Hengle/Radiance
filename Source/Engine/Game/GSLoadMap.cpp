// GSLoadMap.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "GSLoadMap.h"
#include "../COut.h"
#include "../App.h"
#include "../Engine.h"
#include "../World/World.h"
#include "../Sound/Sound.h"
#include <algorithm>
#undef min
#undef max

#if defined(RAD_OPT_IOS)
void __IOS_Throttle(bool);
#endif

GSLoadMap::GSLoadMap(int mapId, int slot, bool play, bool loadScreen) : 
Game::Tickable(Priority), 
m_mapId(mapId), 
m_slot(slot), 
m_play(play),
m_loadScreen(loadScreen)
{
}

int GSLoadMap::Tick(Game &game, float dt, const xtime::TimeSlice &outerTime, int flags)
{
	if (!m_map)
	{
		m_map = App::Get()->engine->sys->packages->Asset(m_mapId, pkg::Z_Engine);
		if (!m_map)
		{
			// TODO: handle failed loading
			COut(C_ErrMsgBox) << "Error loading map!" << std::endl;
			return TickPop;
		}

		m_mapAsset = asset::MapAsset::Cast(m_map);
		if (!m_mapAsset)
		{
			COut(C_ErrMsgBox) << "Error loading map!" << std::endl;
			return TickPop;
		}

		m_mapAsset->SetGame(game, m_slot);
		
#if defined(RAD_OPT_IOS)
		__IOS_Throttle(false);
#endif
	}

	int r = pkg::SR_Pending;
	asset::MapAsset::Ref m_mapAsset = asset::MapAsset::Cast(m_map);
	
	xtime::TimeSlice time(std::min<xtime::TimeVal>(100, outerTime.remaining.get()));

	bool first = firstTick;

	while (r == pkg::SR_Pending && time.remaining)
	{
		xtime::TimeVal start = xtime::ReadMilliseconds();
		
		r = m_map->Process(
			time,
			pkg::P_Load|pkg::P_FastPath
		);

		xtime::TimeVal elapsed = xtime::ReadMilliseconds() - start;

		if (elapsed > 0 || first)
		{
			first = false;
			game.FlushInput();
			world::World::Ref world = m_mapAsset->world;
			if (world)
			{
				world->sound->Tick(elapsed/1000.f, false);
				Draw(game, elapsed/1000.f);
			}
		}
	}

	if (r != pkg::SR_Pending)
	{
#if defined(RAD_OPT_IOS)
		__IOS_Throttle(true);
#endif
		
		if (r == pkg::SR_Success)
		{
			COut(C_Info) << "Map loaded successfully." << std::endl;

			Game::Map::Ref map(new (ZWorld) Game::Map());
			map->world = m_mapAsset->world;
			map->asset = m_map;
			map->id = m_mapId;
			game.m_maps[m_slot].active = map;
			if (game.world.get().get() == map->world.get())
				game.FlushInput(true);
			
			if (m_play)
				game.Play();
			
			App::DumpMemStats(C_Debug);
		}
		else
		{
			// TODO: handle failed loading
			COut(C_ErrMsgBox) << "Error loading map!" << std::endl;
		}

		return TickPop;
	}

	return TickNext;
}
