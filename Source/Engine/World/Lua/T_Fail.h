// T_Fail.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "LuaTask.h"
#include <Runtime/PushPack.h>

namespace world {

// task that always fails
class RADENG_CLASS T_Fail : public LuaTask
{
public:
	typedef boost::shared_ptr<T_Fail> Ref;

	virtual int Tick(T &src, float dt, const xtime::TimeSlice &time, int flags)
	{
		return TickPop;
	}

protected:

	virtual int PushResult(lua_State *L)
	{
		lua_pushnil(L);
		return 1;
	}

};

} // world

#include <Runtime/PopPack.h>
