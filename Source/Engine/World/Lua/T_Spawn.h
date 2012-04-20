// T_Spawn.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "LuaTask.h"
#include "../../Packages/PackagesDef.h"
#include "../EntityDef.h"
#include "../Keys.h"
#include <Runtime/PushPack.h>

namespace world {

class RADENG_CLASS T_Spawn : public LuaTask
{
public:
	typedef boost::shared_ptr<T_Spawn> Ref;

	static Ref New(World *world, const Keys &keys);

	virtual int Tick(Entity &e, float dt, const xtime::TimeSlice &time, int flags);

protected:

	virtual RAD_DECLARE_GET(complete, bool) { return m_r <= pkg::SR_Success; }

	virtual int PushResult(lua_State *L);

private:

	T_Spawn(World *world, const Keys &keys);

	EntityRef m_entity;
	Keys m_keys;
	World *m_world;
	int m_r;
	bool m_post;
};

} // world

#include <Runtime/PopPack.h>
