// GSPriority.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Tickable.h"

#define P(_x) \
	_x##_head, \
	_x = TickPriorityGame|_x##_head, \
	_x##_tail = _x##_head

enum GSPriority
{
	GSP_Default
};
