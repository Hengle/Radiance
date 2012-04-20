// StringTest.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include <Runtime/Runtime.h>
#include "../UTCommon.h"

namespace ut
{
	void StringTest()
	{
        Begin("String Test");
		string::String s("hi");
		std::cout << s << std::endl;
	}
}
