// Types.h
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel

#pragma once

#include "IntMath.h"
#include "../PushPack.h"

namespace math {

template <int TSize>
struct stack_tag {
	enum {
		kSize = TSize
	};
};

} // math

#include "../PopPack.h"