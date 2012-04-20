// LuaTask.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Entity.h"
#include "../../Lua/LuaRuntime.h"
#include <Runtime/PushPack.h>

namespace world {

class LuaTask : public Entity::Tickable, public lua::SharedPtr
{
public:
	typedef boost::shared_ptr<LuaTask> Ref;

	LuaTask();
	virtual ~LuaTask();

	RAD_DECLARE_READONLY_PROPERTY(LuaTask, complete, bool);

protected:

	virtual RAD_DECLARE_GET(complete, bool) = 0;

	virtual void PushElements(lua_State *L); // <- from lua::SharedPtr
	virtual int PushResult(lua_State *L);
	virtual void Cancel();

private:

	static int lua_Pending(lua_State *L);
	static int lua_Result(lua_State *L);
	static int lua_Cancel(lua_State *L);

};

} // world

#include <Runtime/PopPack.h>
#include "LuaTask.inl"
