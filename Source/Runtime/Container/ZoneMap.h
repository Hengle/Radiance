// ZoneMap.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IntContainer.h"
#include "../Base/MemoryPool.h"
#include <map>

template <
	typename K,
	typename T,
	typename _Zone,
	typename Pr = std::less<K>
>
struct zone_map
{
	typedef ::std::map<K, T, Pr, zone_allocator<std::pair<const K, T>, _Zone> > type;
};

template <
	typename K,
	typename T,
	typename _Zone,
	typename Pr = std::less<K>
>
struct zone_pool_map
{
	typedef pool_allocator<typename std::pair<const K, T>, _Zone, 8, MaxUReg, DefaultAlignment, MemoryPool> pool_type;
	typedef ::std::map<K, T, Pr, pool_type > type;
};

template <
	typename K,
	typename T,
	typename _Zone,
	typename Pr = std::less<K>
>
struct zone_multimap
{
	typedef ::std::multimap<K, T, Pr, zone_allocator<std::pair<const K, T>, _Zone> > type;
};

template <
	typename K,
	typename T,
	typename _Zone,
	typename Pr = std::less<K>
>
struct zone_pool_multimap
{
	typedef pool_allocator<std::pair<const K, T>, _Zone, 8, MaxUReg, DefaultAlignment, MemoryPool> pool_type;
	typedef ::std::multimap<K, T, Pr, pool_type > type;
};

