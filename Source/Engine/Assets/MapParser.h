/*! \file MapParser.h
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

#pragma once

#if !defined(RAD_OPT_TOOLS)
#error "This file should only be included in tools builds!"
#endif

#include "../Packages/Packages.h"
#include "../World/EntityDef.h"
#include "../SkAnim/SkAnimDef.h"
#include "../World/MapBuilder/MapBuilder.h"
#include <Runtime/Tokenizer.h>
#include <Runtime/File.h>
#include <Runtime/PushPack.h>

class Engine;

namespace asset {

class RADENG_CLASS MapParser : public pkg::Sink<MapParser> {
public:

	static void Register(Engine &engine);

	enum {
		SinkStage = pkg::SS_Parser,
		AssetType = AT_Map,
		SR_End = pkg::SR_User
	};

	MapParser();
	virtual ~MapParser();

#if defined(RAD_OPT_TOOLS)
	int ParseEntity(tools::map_builder::EntSpawn &spawn);
	RAD_DECLARE_READONLY_PROPERTY(MapParser, caMap, const tools::CinematicActorCompressionMap*);

	static int ParseEntity(
		Tokenizer &script,
		tools::map_builder::EntSpawn &spawn
	);

	static int ParseCinematicCompressionMap(
		Tokenizer &script,
		tools::CinematicActorCompressionMap &caMap
	);

#endif

protected:

	virtual int Process(
		const xtime::TimeSlice &time,
		Engine &engine,
		const pkg::Asset::Ref &asset,
		int flags
	);

private:

#if defined(RAD_OPT_TOOLS)

	static int ParseBrush(
		Tokenizer &script,
		tools::SceneFile::Brush &brush
	);
	
	int ParseCinematicCompressionMap(
		Engine &engine,
		const pkg::Asset::Ref &asset,
		int flags
	);

	int Load(
		const xtime::TimeSlice &time,
		Engine &engine,
		const pkg::Asset::Ref &asset,
		int flags
	);

	RAD_DECLARE_GET(caMap, const tools::CinematicActorCompressionMap*) {
		return &m_caMap;
	}

	enum {
		S_None,
		S_Done
	};

	tools::CinematicActorCompressionMap m_caMap;
	Tokenizer m_script;
	int m_state;
#endif
};

} // asset

#include <Runtime/PopPack.h>
