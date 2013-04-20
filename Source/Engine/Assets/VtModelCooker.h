/*! \file VtModelCooker.h
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

#pragma once

#include "AssetTypes.h"
#include "../Packages/Packages.h"
#include <Runtime/PushPack.h>

class Engine;

namespace asset {

class RADENG_CLASS VtModelCooker : public pkg::Cooker {
public:

	static void Register(Engine &engine);

	enum {
		AssetType = AT_VtModel
	};

	VtModelCooker();
	virtual ~VtModelCooker();

	virtual pkg::CookStatus Status(int flags);
	virtual int Compile(int flags);
};

} // asset

#include <Runtime/PopPack.h>
