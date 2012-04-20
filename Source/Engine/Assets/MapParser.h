// MapParser.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#if !defined(RAD_OPT_TOOLS)
#error "This file should only be included in tools builds!"
#endif

#include "../Packages/Packages.h"
#include "../FileSystem/FileSystem.h"
#include "../World/EntityDef.h"
#include "../Utils/Tokenizer.h"

#include <Runtime/PushPack.h>

class Engine;

namespace asset {

class RADENG_CLASS MapParser : public pkg::Sink<MapParser>
{
public:

	static void Register(Engine &engine);

	enum
	{
		SinkStage = pkg::SS_Parser,
		AssetType = AT_Map
	};

	typedef boost::shared_ptr<MapParser> Ref;

	MapParser();
	virtual ~MapParser();

	enum
	{
		SR_End = pkg::SR_User
	};

	int ParseEntity(world::EntSpawn &spawn);

protected:

	int Process(
		const xtime::TimeSlice &time,
		Engine &engine,
		const pkg::Asset::Ref &asset,
		int flags
	);

private:

	int ParseScript(world::EntSpawn &spawn);

	int Load(
		const xtime::TimeSlice &time,
		Engine &engine,
		const pkg::Asset::Ref &asset,
		int flags
	);

	enum
	{
		S_None,
		S_Loading,
		S_Done
	};

	Tokenizer m_script;

	file::HBufferedAsyncIO m_buf;
	int m_state;
};

} // asset

#include <Runtime/PopPack.h>
