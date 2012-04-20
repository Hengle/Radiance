// EditorTickState.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../../App.h"
#include "EditorProgressDialog.h"
#include <Runtime/Thread.h>
#include <Runtime/PushPack.h>

namespace tools {
namespace editor {

#define P(_x) \
	_x##_head, \
	_x = TickPriorityEditor|_x##_head, \
	_x##_tail = _x##_head

enum TickPriority
{
	P(P_Init),
	P(P_Load)
};

#undef P

class RADENG_CLASS TickInit : public App::Tickable, private thread::Thread
{
public:

	static App::Tickable::Ref New();

	enum
	{
		Priority = P_Init
	};

	TickInit();
	virtual int Tick(::App &app, float dt, const xtime::TimeSlice &time, int flags);

protected:

	virtual int ThreadProc();

private:

	bool m_result;
};

class RADENG_CLASS TickLoad : public App::Tickable, private thread::Thread
{
public:

	static App::Tickable::Ref New();

	enum
	{
		Priority = P_Load
	};

	TickLoad();
	virtual int Tick(::App &app, float dt, const xtime::TimeSlice &time, int flags);

protected:

	virtual int ThreadProc();

private:

	bool m_result;
	ProgressDialog *m_d;
};

} // editor
} // tools

#include <Runtime/PopPack.h>
