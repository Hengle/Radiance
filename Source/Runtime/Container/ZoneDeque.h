// ZoneDeque.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IntContainer.h"
#include <deque>

template
<
	typename T,
	typename _Zone
>
struct zone_deque
{
	typedef ::std::deque<T, zone_allocator<T, _Zone> > type;
};

