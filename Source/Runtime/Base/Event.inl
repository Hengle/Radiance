// Event.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

namespace details {

template <typename C, typename T>
class EventHandlerClassMethodBindManual : public EventHandler<T>
{
public:
	typedef void(C::*MemFn)(const T&);
	EventHandlerClassMethodBindManual(C *_class, MemFn fn) : m_class(_class), m_fn(fn) {}

	virtual void Invoke(const T &t)
	{
		(m_class->*m_fn)(t);
	}

	virtual bool Equals(void *a, void *b)
	{
		return reinterpret_cast<void*>(m_class) == a &&
			*reinterpret_cast<void**>(&m_fn) == b;
	}

	virtual void DestructNotify(BaseEvent *e) {}

private:

	C *m_class;
	MemFn m_fn;
};

template <typename C, typename T>
class EventHandlerClassMethodBindAuto : public EventHandler<T>
{
public:
	typedef void(C::*MemFn)(const T&);
	EventHandlerClassMethodBindAuto(C *_class, MemFn fn) : m_class(_class), m_fn(fn) {}

	virtual void Invoke(const T &t)
	{
		(m_class->*m_fn)(t);
	}

	virtual bool Equals(void *a, void *b)
	{
		return false;
	}

	virtual void DestructNotify(BaseEvent *e)
	{
		m_class->__rad_handle_event_destruct(e);
	}

private:

	C *m_class;
	MemFn m_fn;
};

template <typename C>
class EventHandlerClassMethodBindManual<C, void> : public EventHandler<void>
{
public:
	typedef void(C::*MemFn)();
	EventHandlerClassMethodBindManual(C *_class, MemFn fn) : m_class(_class), m_fn(fn) {}

	virtual void Invoke()
	{
		(m_class->*m_fn)();
	}

	virtual bool Equals(void *a, void *b)
	{
		return reinterpret_cast<void*>(m_class) == a &&
			*reinterpret_cast<void**>(&m_fn) == b;
	}

	virtual void DestructNotify(BaseEvent *e) {}

private:

	C *m_class;
	MemFn m_fn;
};

template <typename C>
class EventHandlerClassMethodBindAuto<C, void> : public EventHandler<void>
{
public:
	typedef void(C::*MemFn)();
	EventHandlerClassMethodBindAuto(C *_class, MemFn fn) : m_class(_class), m_fn(fn) {}

	virtual void Invoke()
	{
		(m_class->*m_fn)();
	}

	virtual bool Equals(void *a, void *b)
	{
		return false;
	}

	virtual void DestructNotify(BaseEvent *e)
	{
		m_class->__rad_handle_event_destruct(e);
	}

private:

	C *m_class;
	MemFn m_fn;
};

} // details

template <typename T, typename TAccess>
Event<T, TAccess>::~Event()
{
	DestructNotify();
}

template <typename T, typename TAccess>
template <typename C>
inline void Event<T, TAccess>::Bind(C *instance, void(C::*fn)(const T&), const ManualReleaseEventTag&)
{
	typename THandler::Ref handler(new details::EventHandlerClassMethodBindManual<C, T>(instance, fn));
	TAccess().Lock(this);
	m_handlers.insert(typename HandlerMMap::value_type(instance, handler));
	TAccess().Unlock(this);
}

template <typename T, typename TAccess>
template <typename C>
inline EventBinding::Ref Event<T, TAccess>::Bind(C *instance, void(C::*fn)(const T&), const AutoReleaseEventTag&)
{
	typename THandler::Ref handler(new details::EventHandlerClassMethodBindAuto<C, T>(instance, fn));
	TAccess().Lock(this);
	typename HandlerMMap::iterator it = m_handlers.insert(typename HandlerMMap::value_type(instance, handler));
	TAccess().Unlock(this);
	return Binding::Ref(new Binding(this, it));
}

template <typename T, typename TAccess>
inline void Event<T, TAccess>::Trigger(const DataType &t)
{
	TAccess().Lock(this);
	typename HandlerMMap::iterator it = m_handlers.begin();
	typename HandlerMMap::iterator end = m_handlers.end();
	
	for (; it != end; ++it)
	{
		it->second->Invoke(t);
	}
	TAccess().Unlock(this);
}

template <typename T, typename TAccess>
template <typename C>
void Event<T, TAccess>::Unbind(C *instance, void(C::*fn)(const T&))
{
	TAccess().Lock(this);
	typename HandlerMMap::iterator it = m_handlers.begin();
	typename HandlerMMap::iterator end = m_handlers.end();
	
	for (; it != end;)
	{
		if (it->second->Equals(instance, *reinterpret_cast<void**>(&fn)))
		{
			m_handlers.erase(it);
			break;
		}
		else
		{
			++it;
		}
	}
	TAccess().Unlock(this);
}

template <typename T, typename TAccess>
inline void Event<T, TAccess>::Unbind(EventBinding *binding)
{
	TAccess().Lock(this);
	m_handlers.erase(static_cast<Binding*>(binding)->it);
	TAccess().Unlock(this);
}

template <typename T, typename TAccess>
inline void Event<T, TAccess>::DestructNotify()
{
	TAccess().Lock(this);
	typename HandlerMMap::iterator it = m_handlers.begin();
	typename HandlerMMap::iterator end = m_handlers.end();
	
	for (; it != end; ++it)
	{
		it->second->DestructNotify(this);
	}
	
	TAccess().Unlock(this);
}

template <typename TAccess>
Event<void, TAccess>::~Event()
{
	DestructNotify();
}

template <typename TAccess>
template <typename C>
inline void Event<void, TAccess>::Bind(C *instance, void(C::*fn)(), const ManualReleaseEventTag&)
{
	THandler::Ref handler(new details::EventHandlerClassMethodBindManual<C, void>(instance, fn));
	TAccess().Lock(this);
	m_handlers.insert(HandlerMMap::value_type(instance, handler));
	TAccess().Unlock(this);
}

template <typename TAccess>
template <typename C>
inline EventBinding::Ref Event<void, TAccess>::Bind(C *instance, void(C::*fn)(), const AutoReleaseEventTag&)
{
	THandler::Ref handler(new details::EventHandlerClassMethodBindAuto<C, void>(instance, fn));
	TAccess().Lock(this);
	typename HandlerMMap::iterator it = m_handlers.insert(HandlerMMap::value_type(instance, handler));
	TAccess().Unlock(this);
	return Binding::Ref(new Binding(this, it));
}

template <typename TAccess>
inline void Event<void, TAccess>::Trigger()
{
	TAccess().Lock(this);
	
	typename HandlerMMap::iterator it = m_handlers.begin();
	typename HandlerMMap::iterator end = m_handlers.end();
	
	for (; it != end; ++it)
	{
		it->second->Invoke();
	}
	TAccess().Unlock(this);
}

template <typename TAccess>
template <typename C>
void Event<void, TAccess>::Unbind(C *instance, void(C::*fn)())
{
	TAccess().Lock(this);
	
	typename HandlerMMap::iterator it = m_handlers.begin();
	typename HandlerMMap::iterator end = m_handlers.end();
	
	for (; it != end;)
	{
		if (it->second->Equals(instance, *reinterpret_cast<void**>(&fn)))
		{
			m_handlers.erase(it);
			break;
		}
		else
		{
			++it;
		}
	}
	TAccess().Unlock(this);
}

template <typename TAccess>
inline void Event<void, TAccess>::Unbind(EventBinding *binding)
{
	TAccess().Lock(this);
	m_handlers.erase(static_cast<Binding*>(binding)->it);
	TAccess().Unlock(this);
}

template <typename TAccess>
inline void Event<void, TAccess>::DestructNotify()
{
	TAccess().Lock(this);
	
	typename HandlerMMap::iterator it = m_handlers.begin();
	typename HandlerMMap::iterator end = m_handlers.end();
	
	for (; it != end; ++it)
	{
		it->second->DestructNotify(this);
	}
	
	TAccess().Unlock(this);
}
