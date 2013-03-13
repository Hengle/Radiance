/*! \file D_ConversationTree.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup lua_asset
*/

#pragma once

#include "D_Asset.h"
#include "../../Assets/ConversationTreeParser.h"
#include <Runtime/PushPack.h>

namespace world {

class RADENG_CLASS D_ConversationTree : public D_Asset {
public:
	typedef boost::shared_ptr<D_ConversationTree> Ref;

	static Ref New(const pkg::AssetRef &conversationTree);

	RAD_DECLARE_READONLY_PROPERTY(D_ConversationTree, conversationTree, asset::ConversationTreeParser*);

protected:

	virtual void PushElements(lua_State *L);

private:

	D_ConversationTree(const pkg::AssetRef &asset);

	RAD_DECLARE_GET(conversationTree, asset::ConversationTreeParser*) {
		return m_conversationTree;
	}

	static int lua_StringTable(lua_State *L);
	static int lua_Data(lua_State *L);

	asset::ConversationTreeParser *m_conversationTree;
};

} // world

#include <Runtime/PopPack.h>
