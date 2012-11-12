// MapCooker.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "AssetTypes.h"
#include "../Packages/Packages.h"
#include "../World/EntityDef.h"
#include "../Tools/Progress.h"
#include "MapParser.h"
#include "../World/MapBuilder/MapBuilder.h"
#include <Runtime/Tokenizer.h>
#include <Runtime/PushPack.h>

class Engine;

namespace asset {

class RADENG_CLASS MapCooker : public pkg::Cooker {
public:

	static void Register(Engine &engine);

	enum {
		AssetType = AT_Map,
		SR_End = pkg::SR_User
	};

	MapCooker();
	virtual ~MapCooker();

	virtual pkg::CookStatus Status(int flags, int allflags);
	virtual int Compile(int flags, int allflags);

	void SetProgressIndicator(tools::UIProgress *ui);

private:

	int TickCompile(int flags, int allflags);

	int ParseEntity(world::EntSpawn &spawn);
	int ParseScript(world::EntSpawn &spawn);

	Tokenizer m_script;
	tools::UIProgress *m_ui;
	MapParser::Ref m_parser;
	tools::MapBuilder::Ref m_mapBuilder;
	bool m_parsing;
};

} // asset

#include <Runtime/PopPack.h>
