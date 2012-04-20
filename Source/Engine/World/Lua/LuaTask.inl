// LuaTask.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

namespace world {

inline LuaTask::LuaTask() : Entity::Tickable(TickPriorityNormal)
{
}

inline LuaTask::~LuaTask()
{
}

inline int LuaTask::PushResult(lua_State *L)
{
	return 0;
}

inline void LuaTask::Cancel()
{
}

inline int LuaTask::lua_Pending(lua_State *L)
{
	Ref task = Get<LuaTask>(L, "LuaTask", -1, true);
	lua_pushboolean(L, task->complete ? 0 : 1);
	return 1;
}

inline int LuaTask::lua_Result(lua_State *L)
{
	return Get<LuaTask>(L, "LuaTask", -1, true)->PushResult(L);
}

inline int LuaTask::lua_Cancel(lua_State *L)
{
	Get<LuaTask>(L, "LuaTask", -1, true)->Cancel();
	return 0;
}

} // world
