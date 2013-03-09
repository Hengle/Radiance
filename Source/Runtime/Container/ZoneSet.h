// ZoneSet.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IntContainer.h"
#include "../Base/MemoryPool.h"
#include <set>

template <
	typename Key,
	typename _Zone,
	typename Pr = std::less<Key>
>
struct zone_set
{
	typedef ::std::set<Key, Pr, zone_allocator<Key, _Zone> > type;
};

template <
	typename Key,
	typename _Zone,
	typename Pr = std::less<Key>
>
struct zone_pool_set
{
	typedef pool_allocator<Key, _Zone, 8, 0xffffffff, kDefaultAlignment, MemoryPool> pool_type;
	typedef ::std::set<Key, Pr, pool_type > type;
};

template <
	typename Key,
	typename _Zone,
	typename Pr = std::less<Key>
>
struct zone_multiset
{
	typedef ::std::multiset<Key, Pr, zone_allocator<Key, _Zone> > type;
};

template <
	typename Key,
	typename _Zone,
	typename Pr = std::less<Key>
>
struct zone_pool_multiset
{
	typedef pool_allocator<Key, _Zone, 8, 0xffffffff, kDefaultAlignment, MemoryPool> pool_type;
	typedef ::std::multiset<Key, Pr, pool_type > type;
};
