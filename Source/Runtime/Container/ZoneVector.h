// ZoneVector.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IntContainer.h"
#include <vector>

template <
	typename T,
	typename _Zone
>
struct zone_vector
{
	typedef ::std::vector<T, zone_allocator<T, _Zone> > type;
};
