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

/*
==============================================================================
Event System (similiar to C#):
	
	--------------------------------------------------------------------------
	Declare an event type using the Event<> template like so:
	--------------------------------------------------------------------------

	struct MyEventPayload {
		...fields...
	};

	typedef Event<MyEventPayload, AccessControlType> MyEvent;

	The AccessControlType is a traits class that contains:
		class AccessControl {
			static void Lock(MyEvent* or BaseEvent*);
			static void Unlock(MyEvent* or BaseEvent*);
		};

		This is used to synchronize access to the event object if there are multipe
		threads using the event

	Use EventNoAccess if you have no need for synchronization.

	--------------------------------------------------------------------------
	Trigger an event like so:
	--------------------------------------------------------------------------

	class MyClassSender {
		MyEvent OnMyEvent;
	};

	MyEventPayload payload;
	payload.fields = {}

	myClassSenderInstance.OnMyEvent.Trigger(payload);

	--------------------------------------------------------------------------
	Bind an event handler like so:
	--------------------------------------------------------------------------

	There are 2 types of event handlers: manual release and auto release. Auto
	release events manage the linkage between object instances automatically.
	If the sender is deleted, all the receivers are automatically disconnected.
	If a receive is deleted it automatically unregisters from the sender. Note
	this could cause issues if sender and receiver are destroyed on different
	threads in an undefined way (like both at the same time).

	A manual release event is bound like so:

	class MyEventReceiver {
		void MyEventHandler(const MyEventPayload &payload);
	};

	myClassSenderInstance.OnMyEvent.Bind(myReceiverInstance, &MyEventReceiver::MyEventHandler, ManualReleaseTag);

	Unbind a manual release event liks so:

	myClassSenderInstance.OnMyEvent.Unbind(myReceiverInstance, &MyClassReceiver::MyEventHandler);

	An auto release event is bound like so:

	class MyEventReceiver {
		RAD_EVENT_CLASS(EventNoAccess)
		void MyEventHandler(const MyEventPayload &payload);
	};

	myReceiverInstance.Bind(myClassSenderInstance.OnMyEvent, &MyEventReceiver::MyEventHandler);

	No need to unregister!

==============================================================================
*/

RAD_ZONE_DEC(RADRT_API, ZEvents);

struct ManualReleaseEventTag_t { ManualReleaseEventTag_t() {} };
struct AutoReleaseEventTag_t { AutoReleaseEventTag_t() {} };
static const ManualReleaseEventTag_t ManualReleaseEventTag;
static const AutoReleaseEventTag_t AutoReleaseEventTag;

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
	virtual ~EventBinding() {}
	bool Equals(BaseEvent *e) const { return m_e == e; }
protected:
	void Unbind() { m_e->Unbind(this); }
private:
	BaseEvent *m_e;
};

struct EventNoAccess
{
	static void Lock(BaseEvent*) {}
	static void Unlock(BaseEvent*) {}
	static void Lock(void*) {}
	static void Unlock(void*) {}
};

template <typename T, typename TAccess>
class Event : public BaseEvent, public boost::noncopyable
{
public:
	typedef Event<T, TAccess> SelfType;
	typedef T DataType;

	~Event();

	template <typename C>
	void Bind(C *instance, void(C::*fn)(const T&), const ManualReleaseEventTag_t&);
	template <typename C>
	EventBinding::Ref Bind(C *instance, void(C::*fn)(const T&), const AutoReleaseEventTag_t&);

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
		virtual ~Binding() { Unbind(); }
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
	void Bind(C *instance, void(C::*fn)(), const ManualReleaseEventTag_t&);
	template <typename C>
	EventBinding::Ref Bind(C *instance, void(C::*fn)(), const AutoReleaseEventTag_t&);

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
		virtual ~Binding() { Unbind(); }
		typename HandlerMMap::iterator it;
	};

	virtual void Unbind(EventBinding *binding);
	void DestructNotify();

	HandlerMMap m_handlers;
};

// Classes that use auto-release events need to place this macro in their class definition
#define RAD_EVENT_CLASS(_TAccess) \
private: \
	typedef zone_list<EventBinding::Ref, ZEventsT>::type __rad_event_handlers; \
	__rad_event_handlers __event_handlers; \
public: \
	template <typename E, typename C> \
	void Bind(E &e, void (C::*F)(const typename E::DataType&)) \
	{ \
		_TAccess::Lock(this); \
		__event_handlers.push_back(e.Bind(this, F, AutoReleaseEventTag)); \
		_TAccess::Unlock(this); \
	} \
	void __rad_handle_event_destruct(BaseEvent *e) \
	{ \
		_TAccess::Lock(this); \
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
		_TAccess::Unlock(this); \
	} \
private:

#include "../PopPack.h"
#include "Event.inl"

