// SCC.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Types.h"
#include <boost/shared_ptr.hpp>
#include <Runtime/PushPack.h>

class RADENG_CLASS SCC
{
public:

	typedef boost::shared_ptr<SCC> Ref;
	
	virtual ~SCC() {}

	static Ref Create(const char *name);
	virtual bool Checkout(const char *filename) = 0;	
};

#include <Runtime/PopPack.h>
