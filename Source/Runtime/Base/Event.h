// Event.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "Base.h"
#include "../Container/ZoneMap.h"
#include "../Container/ZoneList.h"
#include "EventDetails.h"
#include "../PushPack.h"

RAD_ZONE_DEC(RADRT_API, ZEvents);

struct ManualReleaseEventTag {};
struct AutoReleaseEventTag {};
class EventBinding;

class BaseEvent
{
protected:
	friend class EventBinding;
	virtual void Unbind(EventBinding *binding) = 0;
};

class EventBinding
{
public:
	typedef boost::shared_ptr<EventBinding> Ref;
	EventBinding(BaseEvent *e) : m_e(e) {}
	virtual ~EventBinding() { m_e->Unbind(this); }
	bool Equals(BaseEvent *e) const { return m_e == e; }
private:
	BaseEvent *m_e;
};

struct EventNoAccess
{
	void Lock(BaseEvent*) {}
	void Unlock(BaseEvent*) {}
	void Lock(void*) {}
	void Unlock(void*) {}
};

template <typename T, typename TAccess>
class Event : public BaseEvent, public boost::noncopyable
{
public:
	typedef Event<T, TAccess> SelfType;
	typedef T DataType;

	~Event();

	template <typename C>
	void Bind(C *instance, void(C::*fn)(const T&), const ManualReleaseEventTag&);
	template <typename C>
	EventBinding::Ref Bind(C *instance, void(C::*fn)(const T&), const AutoReleaseEventTag&);

	void Trigger(const DataType &t);

	template <typename C>
	void Unbind(C *instance, void(C::*fn)(const T&));

private:

	typedef details::EventHandler<T> THandler;
	typedef typename zone_multimap<void*, typename THandler::Ref, ZEventsT>::type HandlerMMap;

	class Binding : public EventBinding
	{
	public:
		Binding(SelfType *self, typename HandlerMMap::iterator _it) : EventBinding(self), it(_it) {}
		typename HandlerMMap::iterator it;
	};

	virtual void Unbind(EventBinding *binding);
	void DestructNotify();

	HandlerMMap m_handlers;
};

template <typename TAccess>
class Event<void, TAccess> : public BaseEvent, public boost::noncopyable
{
public:
	typedef Event<void, TAccess> SelfType;
	typedef void DataType;

	~Event();

	template <typename C>
	void Bind(C *instance, void(C::*fn)(), const ManualReleaseEventTag&);
	template <typename C>
	EventBinding::Ref Bind(C *instance, void(C::*fn)(), const AutoReleaseEventTag&);

	void Trigger();

	template <typename C>
	void Unbind(C *instance, void(C::*fn)());

private:

	typedef details::EventHandler<void> THandler;
	typedef typename zone_multimap<void*, typename THandler::Ref, ZEventsT>::type HandlerMMap;

	class Binding : public EventBinding
	{
	public:
		Binding(SelfType *self, typename HandlerMMap::iterator _it) : EventBinding(self), it(_it) {}
		typename HandlerMMap::iterator it;
	};

	virtual void Unbind(EventBinding *binding);
	void DestructNotify();

	HandlerMMap m_handlers;
};

#define RAD_EVENT_CLASS(_TAccess) \
private: \
	typedef zone_list<EventBinding::Ref, ZEventsT>::type __rad_event_handlers; \
	__rad_event_handlers __event_handlers; \
public: \
	template <typename E, typename C> \
	void Bind(E &e, void (C::*F)(const typename E::DataType&)) \
	{ \
		_TAccess().Lock(this); \
		__event_handlers.push_back(e.Bind(this, F), AutoReleaseEventTag()); \
		_TAccess().Unlock(this); \
	} \
	void __rad_handle_event_destruct(BaseEvent *e) \
	{ \
		_TAccess().Lock(this); \
		for (__rad_event_handlers::iterator it = __event_handlers.begin(); it != __event_handlers.end();) \
		{ \
			if ((*it)->Equals(e)) \
			{ \
				it = __event_handlers.erase(it); \
			} \
			else \
			{ \
				++it; \
			} \
		} \
		_TAccess().Unlock(this); \
	} \
private:

#include "../PopPack.h"
#include "Event.inl"

