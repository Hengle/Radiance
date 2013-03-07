/*! \file MapCooker.h
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

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

	virtual pkg::CookStatus Status(int flags);
	virtual int Compile(int flags);

	void SetProgressIndicator(tools::UIProgress *ui);

private:

	int TickCompile(int flags);

	int ParseCinematicCompressionMap(int flags);

	Tokenizer m_script;
	tools::CinematicActorCompressionMap m_caMap;
	tools::UIProgress *m_ui;
	MapParser *m_parser;
	tools::map_builder::MapBuilder::Ref m_mapBuilder;
	bool m_parsing;
};

} // asset

#include <Runtime/PopPack.h>
