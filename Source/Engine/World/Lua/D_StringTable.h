/*! \file D_StringTable.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup lua_asset
*/

#pragma once

#include "D_Asset.h"
#include "../../Assets/StringTableParser.h"
#include <Runtime/PushPack.h>

namespace world {

class RADENG_CLASS D_StringTable : public D_Asset {
public:
	typedef boost::shared_ptr<D_StringTable> Ref;

	static Ref New(const pkg::AssetRef &stringTable);

	RAD_DECLARE_READONLY_PROPERTY(D_StringTable, stringTable, asset::StringTableParser*);

protected:

	virtual void PushElements(lua_State *L);

private:

	D_StringTable(const pkg::AssetRef &asset);

	RAD_DECLARE_GET(stringTable, asset::StringTableParser*) {
		return m_stringTable;
	}

	static int lua_Find(lua_State *L);

	asset::StringTableParser *m_stringTable;
};

} // world

#include <Runtime/PopPack.h>
