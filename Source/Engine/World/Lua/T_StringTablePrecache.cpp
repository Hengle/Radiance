/*! \file T_StringTablePrecache.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup lua_asset
*/

#include RADPCH
#include "T_StringTablePrecache.h"
#include "D_StringTable.h"

namespace world {

T_StringTablePrecache::T_StringTablePrecache(World *world, const pkg::AssetRef &asset)
: T_Precache(world, asset) {
}

int T_StringTablePrecache::PushResult(lua_State *L) {
	if (result != pkg::SR_Success)
		return 0;

	D_StringTable::Ref stringTable(D_StringTable::New(asset));
	stringTable->Push(L);
	return 1;
}

} // world
