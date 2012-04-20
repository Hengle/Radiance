// SCC_null.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "SCC.h"
#include <Runtime/PushPack.h>

class SCC_null : public SCC
{
public:
	static SCC *Create() { return new SCC_null(); }
	virtual bool Checkout(const char *filename) { return true; }
};

#include <Runtime/PopPack.h>
