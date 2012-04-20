// TimeDef.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IntTime.h"
#include "../PushPack.h"


namespace xtime {

typedef UReg TimeVal;
struct TimeDate;
class TimeSlice;

template <typename T>
struct Constants
{
	static T MillisPerSecond() { return T(1000); }
	static T MicrosPerSecond() { return MillisPerSecond() * T(1000); }
	static T MillisPerMicro()  { return MillisPerSecond() / MicrosPerSecond(); }
	static T MicrosPerMilli()  { return MicrosPerSecond() / MillisPerSecond(); }
	static T MillisPerMinute() { return MillisPerSecond() * T(60); }
	static T MillisPerHour()   { return MillisPerMinute() * T(60); }
	static T MillisPerDay()    { return MillisPerHour() * T(24); }
	static T MillisPerWeek()   { return MillisPerDay() * T(7); }
	static T MillisPerYear()   { return MillisPerDay() * T(365); }
	static T MicrosPerMinute() { return MillisPerMinute() * MicrosPerMilli(); }
	static T MicrosPerHour()   { return MillisPerHour() * MicrosPerMilli(); }
	static T MicrosPerDay()    { return MillisPerDay() * MicrosPerMilli(); }
	static T MicrosPerWeek()   { return MillisPerWeek() * MicrosPerMilli(); }
	static T MicrosPerYear()   { return MillisPerYear() * MicrosPerMilli(); }
	static T MilliToMicro(const T &x) { return MillisPerMicro() * x; }
	static T MicroToMilli(const T &x) { return MicrosPerMilli() * x; }
	static T MilliToSecond(const T &x) { return x / MillisPerSecond(); }
	static T MicrosToSecond(const T &x) { return x / MicrosPerSecond(); }
	static T SecondsToMilli(const T &x) { return x * MillisPerSecond(); }
	static T SecondsToMicro(const T &x) { return x * MicrosPerSecond(); }
};

enum
{
	MillisPerSecond = 1000,
	MicrosPerMilli  = 1000,
	MicrosPerSecond = MillisPerSecond * MicrosPerMilli,
	MillisPerMinute = MillisPerSecond * 60,
	MillisPerHour   = MillisPerMinute * 60,
	MillisPerDay    = MillisPerHour * 24,
	MillisPerWeek   = MillisPerDay * 7,
	MicrosPerMinute = MillisPerMinute * MicrosPerMilli // no more micros (will overflow 32bits)
};

} // xtime


#include "../PopPack.h"
