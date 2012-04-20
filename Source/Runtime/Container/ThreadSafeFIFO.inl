// ThreadSafeFIFO.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

namespace container {

template<typename T, UReg NumBuckets, typename A>
inline DynamicThreadSafeFIFO<T, NumBuckets, A>::DynamicThreadSafeFIFO() : 
	m_nextRead(0), 
	m_nextWrite(0), 
	m_queueLength(0)
{
	// MUST BE A POWER OF TWO!
	RAD_STATIC_ASSERT((NumBuckets & (NumBuckets-1)) == 0);
}

template<typename T, UReg NumBuckets, typename A>
inline DynamicThreadSafeFIFO<T, NumBuckets, A>::~DynamicThreadSafeFIFO()
{
	Clear();
}

template<typename T, UReg NumBuckets, typename A>
bool DynamicThreadSafeFIFO<T, NumBuckets, A>::Read(T &item)
{
	m_readCS.lock();
	while (m_queueLength > 0)
	{
		--m_queueLength;
		m_readCS.unlock();
		
		UReg pos = m_nextRead++;
		pos = pos & (NumBuckets-1);

		m_itemLock.Lock(ReadKey); // don't run while in Erase()
		
		m_list[pos].m.lock();
		if (m_list[pos].empty())
		{
			m_list[pos].m.unlock();
			m_itemLock.Unlock(ReadKey);
			m_readCS.lock();
			continue;
		}

		_Item *temp = m_list[pos].front();

		if (temp->p)
			temp->p->i = 0;

		item = temp->t;
		m_list[pos].pop_front();
		m_list[pos].m.unlock();
		m_itemLock.Unlock(ReadKey);

		m_alloc.destroy(temp);
		m_alloc.deallocate(temp, 1);
		return true;
	}
	
	m_readCS.unlock();
	return false;
}

template<typename T, UReg NumBuckets, typename A>
void DynamicThreadSafeFIFO<T, NumBuckets, A>::Write(const T &item, Item *pip)
{
	_Item *i = m_alloc.allocate(1);
	RAD_OUT_OF_MEM(i);
	i = new (i) _Item;
	i->t = item;
	i->p = pip;
	UReg pos = (UReg)(m_nextWrite++);
	pos = pos & (NumBuckets-1);
	m_list[pos].m.lock();
	m_list[pos].push_back(i);
	if (pip)
	{
		pip->i = i;
		pip->l = &m_list[pos];
		pip->it = --(m_list[pos].end());
	}
	m_list[pos].m.unlock();
	++m_queueLength;
}

template<typename T, UReg NumBuckets, typename A>
bool DynamicThreadSafeFIFO<T, NumBuckets, A>::Erase(Item &i, T *t)
{
	m_itemLock.Lock(EraseKey); // don't run while in Read()
	bool r = i.i != 0;
	if (r)
	{
		i.l->m.lock();
		i.l->erase(i.it);
		i.l->m.unlock();
	}
	m_itemLock.Unlock(EraseKey);
	if (r)
	{
		if (t) 
			*t = i.i->t;
		m_alloc.destroy(i.i);
		m_alloc.deallocate(i.i, 1);
		i.i = 0;
	}
	return r;
}

template<typename T, UReg NumBuckets, typename A>
inline UReg DynamicThreadSafeFIFO<T, NumBuckets, A>::Length() const
{
	return m_queueLength;
}

template<typename T, UReg NumBuckets, typename A>
inline bool DynamicThreadSafeFIFO<T, NumBuckets, A>::Empty() const
{
	return Length() == 0;
}

template<typename T, UReg NumBuckets, typename A>
void DynamicThreadSafeFIFO<T, NumBuckets, A>::Clear()
{
	m_nextRead = 0;
	m_nextWrite = 0;
	m_queueLength = 0;

	for (UReg i = 0; i < NumBuckets; ++i)
	{
		while (!m_list[i].empty())
		{
			_Item *z = m_list[i].front();
			m_list[i].pop_front();
			if (z->p) 
			{ 
				z->p->i = 0; z->p->l = 0; 
			}
			m_alloc.destroy(z);
			m_alloc.deallocate(z, 1);
		}
	}
}

} // container
