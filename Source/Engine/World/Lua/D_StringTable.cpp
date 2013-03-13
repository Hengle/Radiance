/*! \file D_StringTable.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup lua_asset
*/

#include RADPCH
#include "D_StringTable.h"

namespace world {

D_StringTable::Ref D_StringTable::New(const pkg::AssetRef &stringTable) {
	if (stringTable->type != asset::AT_StringTable)
		return D_StringTable::Ref();

	D_StringTable::Ref r(new (ZWorld) D_StringTable(stringTable));
	return r;
}

D_StringTable::D_StringTable(const pkg::AssetRef &asset) : D_Asset(asset) {
	m_stringTable = asset::StringTableParser::Cast(asset);
	RAD_ASSERT(m_stringTable);
}

void D_StringTable::PushElements(lua_State *L) {
	lua_pushcfunction(L, lua_Find);
	lua_setfield(L, -2, "Find");
}

int D_StringTable::lua_Find(lua_State *L) {
	D_StringTable::Ref self = lua::SharedPtr::Get<D_StringTable>(L, "StringTable", 1, true);

	const String *s = self->stringTable->stringTable->Find(
		luaL_checkstring(L, 2),
		(StringTable::LangId)(int)luaL_checkinteger(L, 3)
	);

	if (s) {
		lua_pushstring(L, s->c_str);
		return 1;
	}

	return 0;
}

} // world
