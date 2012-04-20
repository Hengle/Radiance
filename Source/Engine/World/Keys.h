// Keys.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Types.h"
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/PushPack.h>

namespace world {

///////////////////////////////////////////////////////////////////////////////

struct Keys
{
	typedef boost::shared_ptr<Keys> Ref;
	typedef zone_map<String, String, ZWorldT>::type Pairs;
	Pairs pairs;

	int IntForKey(const char *name, int def=-1) const;
	bool BoolForKey(const char *name, bool def=false) const;
	float FloatForKey(const char *name, float def=0.0f) const;
	const char *StringForKey(const char *name, const char *def = 0) const;
	Color4 Color4ForKey(const char *name, const Color4 &def = Color4::Zero) const;
	Vec3 Vec3ForKey(const char *name, const Vec3 &def = Vec3::Zero) const;

};

} // world

#include <Runtime/PopPack.h>
