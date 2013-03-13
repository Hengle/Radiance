/*! \file T_ConversationTreePrecache.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup lua_asset
*/

#include RADPCH
#include "T_ConversationTreePrecache.h"
#include "D_ConversationTree.h"

namespace world {

T_ConversationTreePrecache::T_ConversationTreePrecache(World *world, const pkg::AssetRef &asset)
: T_Precache(world, asset) {
}

int T_ConversationTreePrecache::PushResult(lua_State *L) {
	if (result != pkg::SR_Success)
		return 0;

	D_ConversationTree::Ref conversationTree(D_ConversationTree::New(asset));
	conversationTree->Push(L);
	return 1;
}

} // world
