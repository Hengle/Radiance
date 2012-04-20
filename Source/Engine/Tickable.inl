// Tickable.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Abstract Game Class
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

template <typename T>
inline Tickable<T>::Tickable(int priority) : 
m_priority(priority), 
m_firstTick(true), 
m_complete(false), 
m_queue(0)
{
}

template <typename T>
inline Tickable<T>::~Tickable()
{
}

template <typename T>
inline void Tickable<T>::Dequeue()
{
	if (!m_queue)
		return;
	m_queue->m_states.erase(m_it);
	m_queue = 0;
}

template <typename T>
inline void TickQueue<T>::Push(const typename Tickable<T>::Ref &state)
{
	state->m_it = m_states.insert(typename Tickable<T>::Map::value_type(state->priority, state));
	state->m_queue = this;
}

template <typename T>
inline void TickQueue<T>::Pop()
{
	if (!m_states.empty())
	{
		m_states.begin()->second->m_queue = 0;
		m_states.erase(m_states.begin());
	}
}

template <typename T>
bool TickQueue<T>::Tick(T &t, float elapsed, const xtime::TimeSlice &time, int flags)
{
	for (typename Tickable<T>::Map::iterator it = m_states.begin(); it != m_states.end();)
	{
		const typename Tickable<T>::Ref &state = it->second;
		RAD_ASSERT(state);
		int x = state->Tick(t, elapsed, time, flags);
		state->m_firstTick = false;

		switch (x)
		{
		case TickNext:
			++it;
			break;
		case TickPop:
			{
				state->m_complete = true;
				state->m_queue = 0;
				typename Tickable<T>::Map::iterator next = it;
				++next;
				m_states.erase(it);
				it = next;
			} break;
		default:
			it = m_states.end();
		}
	}

	return !m_states.empty();
}

template <typename T>
typename Tickable<T>::Ref TickQueue<T>::RAD_IMPLEMENT_GET(state)
{
	typename Tickable<T>::Ref x;
	if (!m_states.empty())
		x = m_states.begin()->second;
	return x;
}

template <typename T>
inline void TickQueue<T>::Clear()
{
	for (typename Tickable<T>::Map::const_iterator it = m_states.begin(); it != m_states.end(); ++it)
		it->second->m_queue = 0;
	m_states.clear();
}
