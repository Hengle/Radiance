// TextureCooker.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "AssetTypes.h"
#include "../Packages/Packages.h"
#include <Runtime/PushPack.h>

class Engine;

namespace asset {

class RADENG_CLASS TextureCooker : public pkg::Cooker
{
public:

	static void Register(Engine &engine);

	enum
	{
		AssetType = AT_Texture
	};

	TextureCooker();
	virtual ~TextureCooker();

	virtual pkg::CookStatus Status(int flags, int allflags);
	virtual int Compile(int flags, int allflags);

private:

	pkg::CookStatus CheckRebuild(int flags, int allflags);
	int MatchTargetKeys(int flags, int allflags);
};

} // asset

#include <Runtime/PopPack.h>
