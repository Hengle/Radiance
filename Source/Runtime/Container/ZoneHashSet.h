// ZoneHashSet.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "HashSet.h"
#include "../Base/MemoryPool.h"

template <
	typename Key,
	typename _Zone
>
struct zone_hash_set
{
	typedef typename container::hash_set<Key, zone_allocator<Key, _Zone> >::type type;
};

template <
	typename Key,
	typename _Zone
>
struct zone_pool_hash_set
{
	typedef pool_allocator<Key, _Zone, 8, 0xffffffff, DefaultAlignment, MemoryPool> pool_type;
	typedef typename container::hash_set<Key, pool_type >::type type;
};

template <
	typename Key,
	typename _Zone
>
struct zone_hash_multiset
{
	typedef typename container::hash_multiset<Key, zone_allocator<Key, _Zone> >::type type;
};

template <
	typename Key,
	typename _Zone
>
struct zone_pool_hash_multiset
{
	typedef pool_allocator<Key, _Zone, 8, 0xffffffff, DefaultAlignment, MemoryPool> pool_type;
	typedef typename container::hash_multiset<Key, pool_type >::type type;
};

