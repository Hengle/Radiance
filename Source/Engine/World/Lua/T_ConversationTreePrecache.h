/*! \file T_ConversationTreePrecache.h
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup lua_asset
*/

#pragma once

#include "T_Precache.h"
#include <Runtime/PushPack.h>

namespace world {

class RADENG_CLASS T_ConversationTreePrecache : public T_Precache {
public:
	typedef boost::shared_ptr<T_ConversationTreePrecache> Ref;

	T_ConversationTreePrecache(World *world, const pkg::AssetRef &asset);

protected:

	virtual int PushResult(lua_State *L);

};

} // world

#include <Runtime/PopPack.h>
