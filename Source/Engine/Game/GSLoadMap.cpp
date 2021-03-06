// GSLoadMap.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "GSLoadMap.h"
#include "GameCVars.h"
#include "../COut.h"
#include "../App.h"
#include "../Engine.h"
#include "../World/World.h"
#include "../Sound/Sound.h"
#include "../Packages/Packages.h"
#include <algorithm>
#undef min
#undef max

#if defined(RAD_OPT_PC_TOOLS)
#include <QtGui/QWidget>
#include "../Tools/Editor/EditorMainWindow.h"
#include "../Tools/Editor/EditorProgressDialog.h"
#endif

GSLoadMap::GSLoadMap(int mapId, int slot, bool play, bool loadScreen) : 
Game::Tickable(Priority), 
m_mapId(mapId), 
m_slot(slot), 
m_play(play),
m_loadScreen(loadScreen),
m_mapAsset(0)
#if defined(RAD_OPT_PC_TOOLS)
, m_progressIndicatorParent(0), m_progress(0), m_nested(false)
#endif
{
}

int GSLoadMap::Tick(Game &game, float dt, const xtime::TimeSlice &outerTime, int flags) {
#if defined(RAD_OPT_PC_TOOLS)
	if (m_nested)
		return TickNext;
	m_nested = true;
#endif

	
	if (!m_map) {
		m_map = App::Get()->engine->sys->packages->Asset(m_mapId, pkg::Z_Engine);
		if (!m_map) {
			// TODO: handle failed loading
			COut(C_ErrMsgBox) << "Error loading map!" << std::endl;
			return TickPop;
		}

#if !defined(RAD_OPT_SHIP)
		if (!game.m_dbgServer) {
			game.m_dbgServer = tools::DebugConsoleServer::Start(m_map->path, &game.m_cvarZone);
		} else {
			game.m_dbgServer->SetDescription(m_map->path);
		}
#endif

		m_mapAsset = asset::MapAsset::Cast(m_map);
		if (!m_mapAsset) {
			COut(C_ErrMsgBox) << "Error loading map!" << std::endl;
			return TickPop;
		}

		m_mapAsset->SetGame(game, m_slot);

#if defined(RAD_OPT_PC_TOOLS)
		if (m_progressIndicatorParent) {
			m_progress = new (ZEditor) tools::editor::ProgressDialog(
				"Compiling",
				QString(),
				QString(),
				0,
				0,
				m_progressIndicatorParent
			);
			m_progress->setMinimumDuration(500);
			m_mapAsset->SetProgressIndicator(m_progress);
		}
#endif
	}
	
	App::Get()->throttleFramerate = false; // tick loading as fast as possible.

	int r = pkg::SR_Pending;
	
	xtime::TimeSlice time(std::min<xtime::TimeVal>(100, outerTime.remaining.get()));

	bool first = firstTick;

#if defined(RAD_OPT_PC_TOOLS)
	const int kLoadFlags = (tools::editor::MainWindow::Get()->lowQualityPreview) ? pkg::P_FastCook : 0;
#endif

	while ((r == pkg::SR_Pending) && time.remaining) {
		xtime::TimeVal start = xtime::ReadMilliseconds();
		
		r = m_map->Process(
			time,
			pkg::P_Load|pkg::P_FastPath
#if defined(RAD_OPT_PC_TOOLS)
			| kLoadFlags
#endif
		);

		xtime::TimeVal elapsed = xtime::ReadMilliseconds() - start;

		if (elapsed > 0 || first) {
			first = false;
			game.FlushInput();
			world::World::Ref world = m_mapAsset->world;
			if (world) {
				world->sound->Tick(elapsed/1000.f, false);
				Draw(game, elapsed/1000.f);
			}
		}

#if !defined(RAD_OPT_SHIP)
		if (game.m_dbgServer)
			game.m_dbgServer->ProcessClients();
#endif

#if defined(RAD_OPT_PC_TOOLS)
		if (!m_mapAsset->compiling) {
			if (m_progress) {
				tools::editor::ProgressDialog *d = m_progress;
				m_progress = 0;
				m_mapAsset->SetProgressIndicator(0);
				d->close();
			}
		}
		if (m_progress)
			break;
#endif
	}

	if (r != pkg::SR_Pending) {
#if defined(RAD_OPT_PC_TOOLS)
		if (m_progress) {
			tools::editor::ProgressDialog *d = m_progress;
			m_progress = 0;
			m_mapAsset->SetProgressIndicator(0);
			d->close();
		}
#endif
		App::Get()->throttleFramerate = game.cvars->r_throttle.value; // frame limit if supported.
		
		if (r == pkg::SR_Success) {
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
		} else {
			// TODO: handle failed loading
			COut(C_ErrMsgBox) << "Error loading map!" << std::endl;
#if defined(RAD_OPT_PC_TOOLS)
			if (m_progressIndicatorParent) {
				m_progressIndicatorParent->close();
				m_progressIndicatorParent = 0;
			}
#endif
		}

		return TickPop;
	}

#if defined(RAD_OPT_PC_TOOLS)
	m_nested = false;
#endif

	return TickNext;
}

#if defined(RAD_OPT_PC_TOOLS)
void GSLoadMap::EnableProgressIndicator(QWidget *parent) {
	m_progressIndicatorParent = parent;
}
#endif