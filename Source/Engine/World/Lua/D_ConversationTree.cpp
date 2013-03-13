/*! \file D_ConversationTree.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup lua_asset
*/

#include RADPCH
#include "D_ConversationTree.h"
#include "D_StringTable.h"

namespace world {

D_ConversationTree::Ref D_ConversationTree::New(const pkg::AssetRef &conversationTree) {
	if (conversationTree->type != asset::AT_ConversationTree)
		return D_ConversationTree::Ref();

	D_ConversationTree::Ref r(new (ZWorld) D_ConversationTree(conversationTree));
	return r;
}

D_ConversationTree::D_ConversationTree(const pkg::AssetRef &asset) : D_Asset(asset) {
	m_conversationTree = asset::ConversationTreeParser::Cast(asset);
	RAD_ASSERT(m_conversationTree);
}

void D_ConversationTree::PushElements(lua_State *L) {
	lua_pushcfunction(L, lua_StringTable);
	lua_setfield(L, -2, "StringTable");
	lua_pushcfunction(L, lua_Data);
	lua_setfield(L, -2, "Data");
}

int D_ConversationTree::lua_StringTable(lua_State *L) {
	D_ConversationTree::Ref self = lua::SharedPtr::Get<D_ConversationTree>(L, "ConversationTree", 1, true);
	D_StringTable::Ref r = D_StringTable::New(self->conversationTree->stringTableAsset);
	r->Push(L);
	return 1;
}

int D_ConversationTree::lua_Data(lua_State *L) {
	D_ConversationTree::Ref self = lua::SharedPtr::Get<D_ConversationTree>(L, "ConversationTree", 1, true);
	return self->conversationTree->conversationTree->PushCopy(L);
}

} // world
