// MapParser.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#if !defined(RAD_OPT_TOOLS)
#error "This file should only be included in tools builds!"
#endif

#include "../Packages/Packages.h"
#include "../World/EntityDef.h"
#include "../SkAnim/SkAnimDef.h"
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

	typedef boost::shared_ptr<MapParser> Ref;

	MapParser();
	virtual ~MapParser();

#if defined(RAD_OPT_TOOLS)
	int ParseEntity(world::EntSpawn &spawn);

	RAD_DECLARE_READONLY_PROPERTY(MapParser, caMap, const tools::CinematicActorCompressionMap*);
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
	int ParseScript(world::EntSpawn &spawn);

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
