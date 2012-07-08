// Event.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Types.h"
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/PushPack.h>

namespace world {

class RADENG_CLASS Event : public boost::noncopyable
{
public:
	typedef boost::shared_ptr<Event> Ref;
	typedef zone_vector<Ref, ZWorldT>::type Vec;

	enum Target
	{
		T_Id,
		T_Name,
		T_ViewController,
		T_PlayerPawn,
		T_World
	};

	Event(Target target, const char *cmd, const char *args=0)
		: m_target(target), m_targetId(-1), m_cmd(cmd), m_args(args?args:"")
	{
		RAD_ASSERT(target >= T_ViewController && target <= T_World);
	}

	Event(int targetId, const char *cmd, const char *args=0)
		: m_target(T_Id), m_targetId(targetId), m_cmd(cmd), m_args(args?args:"")
	{
		RAD_ASSERT(targetId!=-1);
	}

	Event(const char *name, const char *cmd, const char *args=0)
		: m_target(T_Name), m_targetId(-1), m_name(name), m_cmd(cmd), m_args(args?args:"")
	{
	}

	virtual ~Event() {}

	RAD_DECLARE_READONLY_PROPERTY(Event, target, Target);
	RAD_DECLARE_READONLY_PROPERTY(Event, targetId, int);
	RAD_DECLARE_READONLY_PROPERTY(Event, name, const char*);
	RAD_DECLARE_READONLY_PROPERTY(Event, cmd, const char*);
	RAD_DECLARE_READONLY_PROPERTY(Event, args, const char*);

private:

	RAD_DECLARE_GET(target, Target) { return m_target; }
	RAD_DECLARE_GET(targetId, int) { return m_targetId; }
	RAD_DECLARE_GET(name, const char*) { return m_name.c_str; }
	RAD_DECLARE_GET(cmd, const char*) { return m_cmd.c_str; }
	RAD_DECLARE_GET(args, const char*) { return m_args.empty ? 0 : (const char*)m_args.c_str; }

	Target m_target;
	int m_targetId;
	String m_name;
	String m_cmd;
	String m_args;
};

} // world


#include <Runtime/PopPack.h>
