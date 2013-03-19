// ZoneHashMap.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "HashMap.h"
#include "../Base/MemoryPool.h"

template <
	typename Key,
	typename Type,
	typename _Zone
>
struct zone_hash_map
{
	typedef typename container::hash_map<Key, Type, zone_allocator<std::pair<Key, Type>, _Zone> >::type type;
};

template <
	typename Key,
	typename Type,
	typename _Zone
>
struct zone_pool_hash_map
{
	typedef pool_allocator<std::pair<Key, Type>, _Zone, 16, 0x7fffffff, MemoryPool> pool_type;
	typedef typename container::hash_map<Key, Type, pool_type >::type type;
};

template <
	typename Key,
	typename Type,
	typename _Zone
>
struct zone_hash_multimap
{
	typedef typename container::hash_multimap<Key, Type, zone_allocator<std::pair<Key, Type>, _Zone> >::type type;
};

template <
	typename Key,
	typename Type,
	typename _Zone
>
struct zone_pool_hash_multimap
{
	typedef pool_allocator<std::pair<Key, Type>, _Zone, 16, 0x7fffffff, MemoryPool> pool_type;
	typedef typename container::hash_multimap<Key, Type, pool_type >::type type;
};
