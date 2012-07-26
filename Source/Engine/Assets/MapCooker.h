// MapCooker.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "AssetTypes.h"
#include "../Packages/Packages.h"
#include "../World/EntityDef.h"
#include "../Utils/Tokenizer.h"
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

private:

	int ParseEntity(world::EntSpawn &spawn);
	int ParseScript(world::EntSpawn &spawn);

	Tokenizer m_script;
};

} // asset

#include <Runtime/PopPack.h>
