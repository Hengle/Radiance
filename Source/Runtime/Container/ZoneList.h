// ZoneList.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IntContainer.h"
#include "../Base/MemoryPool.h"
#include <list>

template <
	typename T,
	typename _Zone
>
struct zone_list
{
	typedef ::std::list<T, zone_allocator<T, _Zone> > type;
};

template <
	typename T,
	typename _Zone
>
struct zone_pool_list
{
	typedef pool_allocator<T, _Zone, 8, 0xffffffff, kDefaultAlignment, MemoryPool> pool_type;
	typedef ::std::list<T, pool_type > type;
};


