/*! \file ConversationTreeCooker.h
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

#pragma once

#include "AssetTypes.h"
#include "../Packages/Packages.h"
#include <Runtime/PushPack.h>

namespace asset {

class RADENG_CLASS ConversationTreeCooker : public pkg::Cooker {
public:

	static void Register(Engine &engine);

	enum {
		AssetType = AT_ConversationTree
	};

	ConversationTreeCooker();
	virtual ~ConversationTreeCooker();

	virtual pkg::CookStatus Status(int flags);
	virtual int Compile(int flags);

};

} // asset


#include <Runtime/PopPack.h>
