// Keys.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "Keys.h"
#include <stdlib.h>

namespace world {

///////////////////////////////////////////////////////////////////////////////

int Keys::IntForKey(const char *name, int def) const
{
	RAD_ASSERT(name);
	String s(name);
	Pairs::const_iterator it = pairs.find(s);
	if (it == pairs.end())
		return def;
	int r;
#define CAWARN_DISABLE 6031 // return value ignored
#include <Runtime/PushCAWarnings.h>
	sscanf(it->second.c_str, "%d", &r);
#include <Runtime/PopCAWarnings.h>
	return r;
}

bool Keys::BoolForKey(const char *name, bool def) const
{
	RAD_ASSERT(name);
	String s(name);
	Pairs::const_iterator it = pairs.find(s);
	if (it == pairs.end())
		return def;
	return it->second == "true";
}

float Keys::FloatForKey(const char *name, float def) const
{
	RAD_ASSERT(name);
	String s(name);
	Pairs::const_iterator it = pairs.find(s);
	if (it == pairs.end())
		return def;
	float r;
#define CAWARN_DISABLE 6031 // return value ignored
#include <Runtime/PushCAWarnings.h>
	sscanf(it->second.c_str, "%f", &r);
#include <Runtime/PopCAWarnings.h>
	return r;
}

const char *Keys::StringForKey(const char *name, const char *def) const
{
	RAD_ASSERT(name);
	String s(name);
	Pairs::const_iterator it = pairs.find(s);
	if (it == pairs.end())
		return def;
	return it->second.c_str;
}

Color4 Keys::Color4ForKey(const char *name, const Color4 &def) const
{
	RAD_ASSERT(name);
	String s(name);
	Pairs::const_iterator it = pairs.find(s);
	if (it == pairs.end())
		return def;
	int r, g, b, a;
#define CAWARN_DISABLE 6031 // return value ignored
#include <Runtime/PushCAWarnings.h>
	sscanf(it->second.c_str, "%d %d %d %d", &r, &g, &b, &a);
#include <Runtime/PopCAWarnings.h>
	return Color4(r/255.0f, g/255.0f, b/255.0f, a/255.0f);
}

Vec3 Keys::Vec3ForKey(const char *name, const Vec3 &def) const
{
	RAD_ASSERT(name);
	String s(name);
	Pairs::const_iterator it = pairs.find(s);
	if (it == pairs.end())
		return def;
	float x, y, z;
#define CAWARN_DISABLE 6031 // return value ignored
#include <Runtime/PushCAWarnings.h>
	sscanf(it->second.c_str, "%f %f %f", &x, &y, &z);
#include <Runtime/PopCAWarnings.h>
	return Vec3(x, y, z);
}

} // world
