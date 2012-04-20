// Tickable.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Abstract Game Class
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "Types.h"
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/Time.h>
#include <Runtime/PushPack.h>

class App;
class Game;

///////////////////////////////////////////////////////////////////////////////

enum
{
	// Priority classes

	TickPriorityLow,
	TickPriorityNormal = 0x10000000,
	TickPriorityHigh   = 0x20000000,
	TickPriorityEditor = 0x30000000,

	// Priorities

	TickPriorityIdle = TickPriorityLow,
	TickPriorityGame = TickPriorityNormal,
	TickPriorityUI   = TickPriorityHigh,

	// Tick return codes.
	TickNone = 0,
	TickNext,
	TickPop
};

template <typename T> class TickQueue;

template <typename T>
class Tickable : public boost::noncopyable
{
public:
	typedef Tickable<T> SelfType;
	typedef boost::shared_ptr<SelfType> Ref;
	typedef typename zone_multimap<int, Ref, ZEngineT>::type Map;

	Tickable(int priority);
	virtual ~Tickable();

	RAD_DECLARE_READONLY_PROPERTY(Tickable, priority, int);
	RAD_DECLARE_READONLY_PROPERTY(Tickable, firstTick, bool);
	RAD_DECLARE_READONLY_PROPERTY(Tickable, complete, bool);

	virtual int Tick(T &src, float dt, const xtime::TimeSlice &time, int flags) = 0;
	void Dequeue();

private:

	friend class TickQueue<T>;

	RAD_DECLARE_GET(priority, int) { return m_priority; }
	RAD_DECLARE_GET(firstTick, bool) { return m_firstTick; }
	RAD_DECLARE_GET(complete, bool) { return m_complete; }

	typename Map::iterator m_it;
	TickQueue<T> *m_queue;
	int m_priority;
	bool m_firstTick;
	bool m_complete;
};

///////////////////////////////////////////////////////////////////////////////

template <typename T>
class TickQueue
{
public:
	TickQueue() {}
	virtual ~TickQueue() {}

	void Push(const typename Tickable<T>::Ref &state);
	void Pop();
	bool Tick(T &t, float dt, const xtime::TimeSlice &time, int flags);
	void Clear();

	RAD_DECLARE_READONLY_PROPERTY(TickQueue, state, typename Tickable<T>::Ref);

private:

	friend class Tickable<T>;

	RAD_DECLARE_GET(state, typename Tickable<T>::Ref);

	typename Tickable<T>::Map m_states;
};

#include <Runtime/PopPack.h>

#include "Tickable.inl"
