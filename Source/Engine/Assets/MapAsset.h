// MapAsset.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Types.h"
#include "AssetTypes.h"
#include "../Packages/Packages.h"
#include "../World/Entity.h"
#include "../World/WorldDef.h"
#include "../World/BSPFile.h"

#if defined(RAD_OPT_TOOLS)
#include "MapParser.h"
#include "../World/MapBuilder/MapBuilder.h"
#include "../Tools/Progress.h"
#endif

#include <Runtime/PushPack.h>

class Engine;
class Game;

namespace asset {

class RADENG_CLASS MapAsset : public pkg::Sink<MapAsset> {
public:

	static void Register(Engine &engine);

	enum {
		SinkStage = pkg::SS_Process,
		AssetType = asset::AT_Map
	};

	typedef boost::shared_ptr<MapAsset> Ref;

	MapAsset();
	virtual ~MapAsset();

	void SetGame(Game &game, int slot) { 
		m_game = &game;
		m_slot = slot;
	}

#if defined(RAD_OPT_TOOLS)
	void SetProgressIndicator(tools::UIProgress *ui);
	void SetDebugUI(tools::map_builder::DebugUI *ui);
	void DebugDraw(float time, float dt, const QRect &viewport);
	void OnDebugMenu(const QVariant &data);

	RAD_DECLARE_READONLY_PROPERTY(MapAsset, compiling, bool);
	RAD_DECLARE_READONLY_PROPERTY(MapAsset, bspFile, world::bsp_file::BSPFile::Ref);
#endif

	RAD_DECLARE_READONLY_PROPERTY(MapAsset, world, world::WorldRef);

protected:

	virtual int Process(
		const xtime::TimeSlice &time,
		Engine &engine,
		const pkg::Asset::Ref &asset,
		int flags
	);

	RAD_DECLARE_GET(world, ::world::WorldRef) { return m_world; }

private:

	int SpawnCooked(
		const xtime::TimeSlice &time,
		Engine &engine,
		const pkg::Asset::Ref &asset,
		int flag
	);

#if defined(RAD_OPT_TOOLS)
	int SpawnTool(
		const xtime::TimeSlice &time,
		Engine &engine,
		const pkg::Asset::Ref &asset,
		int flags
	);
#endif

	Game *m_game;
	int m_slot;
	bool m_spawning;
	::world::WorldRef m_world;
	file::MMapping::Ref m_bspData;
	world::bsp_file::BSPFileParser::Ref m_bspFile;

#if defined(RAD_OPT_TOOLS)
	tools::UIProgress *m_ui;
	tools::map_builder::DebugUI *m_debugUI;
	tools::map_builder::MapBuilder::Ref m_mapBuilder;
	MapParser::Ref m_parser;
	pkg::Cooker::Ref m_cooker;

	RAD_DECLARE_GET(compiling, bool) {
		return m_cooker;
	}

	RAD_DECLARE_GET(bspFile, world::bsp_file::BSPFile::Ref) {
		if (m_mapBuilder)
			return m_mapBuilder->bspFile;
		return m_bspFile;
	}

#endif

};

} // asset

#include <Runtime/PopPack.h>
