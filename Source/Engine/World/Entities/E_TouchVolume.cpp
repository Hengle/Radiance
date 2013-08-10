/*! \file E_TouchVolume.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#include RADPCH
#include "E_TouchVolume.h"
#include "../World.h"
#include "../../COut.h"

namespace world {

E_TouchVolume::E_TouchVolume() : 
E_CONSTRUCT_BASE, 
m_firstBrush(0), 
m_numBrushes(0) {
}

E_TouchVolume::~E_TouchVolume() {
}

int E_TouchVolume::Spawn(
	const Keys &keys,
	const xtime::TimeSlice &time,
	int flags
) {
	E_SPAWN_BASE();
	m_firstBrush = keys.IntForKey("firstBrush");
	m_numBrushes = keys.IntForKey("numBrushes");
	return pkg::SR_Success;
}

Entity::Vec E_TouchVolume::GetTouching(int classbits) const {
	Entity::Vec ents;
	EntityPtrSet set;

	for (int i = 0; i < m_numBrushes; ++i) {
		Entity::Vec touching = world->EntitiesTouchingBrush(classbits, m_firstBrush + i);
		ents.reserve(touching.size());
		for (Entity::Vec::const_iterator it = touching.begin(); it != touching.end(); ++it) {
			// filter duplicates
			if (set.find((*it).get()) == set.end()) {
				set.insert((*it).get());
				ents.push_back((*it));
			}
		}
	}

	return ents;
}

void E_TouchVolume::PushCallTable(lua_State *L) {
	Entity::PushCallTable(L);
	lua_pushcfunction(L, lua_GetTouching);
	lua_setfield(L, -2, "GetTouching");
}

int E_TouchVolume::lua_GetTouching(lua_State *L) {
	E_TouchVolume *self = static_cast<E_TouchVolume*>(WorldLua::EntFramePtr(L, 1, true));
	Entity::Vec ents = self->GetTouching(luaL_checkinteger(L, 2));
	if (ents.empty())
		return 0;

	int ofs = 0;
	lua_createtable(L, (int)ents.size(), 0);
	for (Entity::Vec::const_iterator it = ents.begin(); it != ents.end(); ++it) {
		lua_pushnumber(L, ++ofs);
		(*it)->PushEntityFrame(L);
		lua_settable(L, -3);
	}
	return 1;
}

} // world
