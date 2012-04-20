// EditorContentThumbCache.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "EditorContentThumbCache.h"

namespace tools {
namespace editor {

ContentThumbCache::Item::Item(int id) :
m_id(id), m_usage(0), m_touched(false), m_active(false)
{
}

ContentThumbCache::Item::~Item()
{
}

void ContentThumbCache::Item::MarkVisible()
{
	RAD_ASSERT(m_cache);
	m_cache->Touch(shared_from_this());
}

void ContentThumbCache::Item::Resize(int newSize)
{
	RAD_ASSERT(m_cache);
	m_cache->Resize(shared_from_this(), newSize);
}

void ContentThumbCache::Item::Evict()
{
	RAD_ASSERT(m_cache);
	m_cache->Evict(shared_from_this());
}

ContentThumbCache::ContentThumbCache(int maxLoad, int maxSize) : 
m_size(0), 
m_maxLoad(maxLoad), 
m_maxSize(maxSize), 
m_usage(0),
m_load(0)
{
	RAD_ASSERT(maxLoad>0);
	RAD_ASSERT(maxSize>0);
}

ContentThumbCache::~ContentThumbCache()
{
	CancelAll();
}

ContentThumbCache::Item::Ref ContentThumbCache::Find(int id)
{
	ItemMap::const_iterator it = m_items.find(id);
	return (it != m_items.end()) ? it->second : Item::Ref();
}

void ContentThumbCache::Insert(const Item::Ref &item)
{
	m_items[item->id] = item;
	item->m_cache = this;
	item->m_lruPos = m_lru.end();
}

void ContentThumbCache::BeginRefresh()
{
	m_limbo.clear();
	m_active.swap(m_limbo);
	m_touched.clear();
	m_pending.clear();
	m_untouched = m_items;
	m_load = 0;
}

void ContentThumbCache::EndRefresh()
{
	for (ItemMap::const_iterator it = m_touched.begin(); it != m_touched.end(); ++it)
	{
		if (it->second->active)
		{
			m_limbo.erase(it->first);
			m_active[it->first] = it->second;
			++m_load;
			RAD_ASSERT(m_load <= m_maxLoad);
		}

		RemoveFromLRU(it->second);
		it->second->m_touched = true;
	}

	for (ItemMap::const_iterator it = m_touched.begin(); it != m_touched.end(); ++it)
	{
		if (!it->second->active && it->second->CheckActivate())
		{
			if (m_load < m_maxLoad)
			{
				m_active[it->first] = it->second;
				it->second->m_active = true;
				++m_load;
			}
			else
			{
				m_pending[it->first] = it->second;
			}
		}
	}

	for (ItemMap::iterator it = m_untouched.begin(); it != m_untouched.end();)
	{
		it->second->m_touched = false;
		if (it->second->active && m_load < m_maxLoad)
		{
			RemoveFromLRU(it->second);

			m_limbo.erase(it->first);
			m_active[it->first] = it->second;
			ItemMap::iterator del = it; ++it;
			m_untouched.erase(del);
			++m_load;
			if (m_load == m_maxLoad)
				break;
		}
		else
		{
			InsertLRU(it->second);
			++it;
		}
	}

	// all other outstanding IO must be canceled.
	for (ItemMap::const_iterator it = m_limbo.begin(); it != m_limbo.end(); ++it)
	{
		it->second->m_active = false;
		it->second->Cancel();
	}
	m_limbo.clear();

	if (m_pending.empty())
		m_pending.swap(m_untouched);

	PurgeLRU();
}

bool ContentThumbCache::Tick(float dt, const xtime::TimeSlice &time, bool &visibleTicking)
{
	bool completed = false;
	visibleTicking = false;

	for (ItemMap::iterator it = m_active.begin(); it != m_active.end();)
	{
		RAD_ASSERT(it->second->m_active);

		ThumbResult r = it->second->Tick(dt, time);

		visibleTicking = visibleTicking || (it->second->m_touched && r != TR_Noop);

		if (r != TR_Pending)
		{
			--m_load;
			RAD_ASSERT(m_load>=0);
			it->second->m_active = false;
			completed = completed || (it->second->m_touched && r == TR_Complete);
			ItemMap::iterator del = it; ++it;
			m_active.erase(del);
		}
		else
		{
			++it;
		}
	}

	for (ItemMap::const_iterator it = m_touched.begin(); it != m_touched.end(); ++it)
	{
		if (!it->second->active)
			it->second->InactiveTick(dt);
	}

	// try to move pending items into loading queue.
	while (m_load < m_maxLoad && !m_pending.empty())
	{
		ItemMap::iterator it = m_pending.begin();
		Item::Ref item = it->second;
		m_pending.erase(it);

		if (item->CheckActivate())
		{
			m_active[item->id] = item;
			item->m_active = true;
			++m_load;
		}
	}

	if (m_pending.empty())
		m_pending.swap(m_untouched);

	return completed;
}

void ContentThumbCache::EvictAll()
{
	while (!m_items.empty())
	{
		ItemMap::iterator it = m_items.begin();
		it->second->Evict();
	}
}

void ContentThumbCache::Touch(const Item::Ref &item)
{
	RemoveFromLRU(item);
	m_untouched.erase(item->id);
	m_touched[item->id] = item;
	item->m_usage = m_usage++;
}

void ContentThumbCache::Evict(const Item::Ref &item)
{
	RemoveFromLRU(item);

	if (item->m_active)
	{
		--m_load;
		RAD_ASSERT(m_load>=0);
		item->Cancel();
		item->m_active = false;
	}

	m_active.erase(item->id);
	m_pending.erase(item->id);
	m_touched.erase(item->id);
	m_untouched.erase(item->id);
	m_items.erase(item->id);

	m_size -= item->size;
	RAD_ASSERT(m_size >= 0);

	item->m_cache = 0;
}

void ContentThumbCache::Resize(const Item::Ref &item, int newSize)
{
	m_size -= item->size;
	RAD_ASSERT(m_size >= 0);
	m_size += newSize;
}

void ContentThumbCache::InsertLRU(const Item::Ref &item)
{
	if (item->m_lruPos == m_lru.end())
	{
		item->m_lruPos = m_lru.insert(ItemMap::value_type(item->m_usage, item)).first;
	}
}

void ContentThumbCache::RemoveFromLRU(const Item::Ref &item)
{
	if (item->m_lruPos != m_lru.end())
	{
		m_lru.erase(item->m_lruPos);
		item->m_lruPos = m_lru.end();
	}
}

void ContentThumbCache::PurgeLRU()
{
	while (m_size > m_maxSize && !m_lru.empty())
	{
		ItemMap::iterator it = m_lru.begin();
		Item::Ref item = it->second;
		Evict(item);
	}
}

void ContentThumbCache::CancelAll()
{
	for (ItemMap::const_iterator it = m_active.begin(); it != m_active.end(); ++it)
		it->second->Cancel();
	m_active.clear();
	m_pending.clear();
	m_untouched.clear();
	m_load = 0;
}

} // editor
} // tools
