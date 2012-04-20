// EditorContentThumbCache.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../EditorTypes.h"
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/Container/ZoneList.h>
#include <Runtime/Time.h>
#include <Runtime/PushPack.h>

namespace tools {
namespace editor {

enum ThumbResult
{
	TR_Noop,
	TR_Complete,
	TR_Pending
};

class RADENG_CLASS ContentThumbCache
{
public:

	class Item : public boost::enable_shared_from_this<Item>
	{
	public:
		
		typedef boost::shared_ptr<Item> Ref;

		Item(int id);
		virtual ~Item();

		RAD_DECLARE_READONLY_PROPERTY(Item, id, int);
		RAD_DECLARE_READONLY_PROPERTY(Item, size, int);
		RAD_DECLARE_READONLY_PROPERTY(Item, active, bool);
	
		void MarkVisible();

		// Remove item from cache.
		void Evict();

	protected:

		virtual RAD_DECLARE_GET(size, int) = 0;

		// Redeclare cache size.
		void Resize(int newSize);

		virtual ThumbResult Tick(float dt, const xtime::TimeSlice &time) = 0;
		virtual void InactiveTick(float dt) {}
		virtual void Cancel() = 0;
		virtual bool CheckActivate() const = 0;

	private:

		typedef zone_map<int, Ref, ZEditorT>::type Map;

		friend class ContentThumbCache;
		RAD_DECLARE_GET(id, int) { return m_id; }
		RAD_DECLARE_GET(active, bool) { return m_active; }

		int m_id;
		int m_usage;
		bool m_active;
		bool m_touched;
		Map::iterator m_lruPos;
		ContentThumbCache *m_cache;
	};

	typedef boost::shared_ptr<ContentThumbCache> Ref;

	ContentThumbCache(int maxLoad, int maxSize);
	~ContentThumbCache();

	Item::Ref Find(int id);
	void Insert(const Item::Ref &item);

	void BeginRefresh();
	void EndRefresh();

	// returns true if a visible ticking item finished loading.
	bool Tick(float dt, const xtime::TimeSlice &time, bool &visibleTicking);

	void EvictAll();

private:

	typedef Item::Map ItemMap;

	void Touch(const Item::Ref &item);
	void Evict(const Item::Ref &item);
	void Resize(const Item::Ref &item, int newSize);
	void InsertLRU(const Item::Ref &item);
	void RemoveFromLRU(const Item::Ref &item);
	void PurgeLRU();
	void CancelAll();

	ItemMap m_active;
	ItemMap m_pending;
	ItemMap m_touched;
	ItemMap m_untouched;
	ItemMap m_limbo;
	ItemMap m_items;
	ItemMap m_lru;
	int m_size;
	int m_maxSize;
	int m_maxLoad;
	int m_usage;
	int m_load;
};

} // editor
} // tools

#include <Runtime/PopPack.h>

