// Time.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IntTime.h"
#include "TimeDef.h"
#include "../StreamDef.h"
#include "../String/String.h"
#include <boost/thread/xtime.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include "../PushPack.h"

namespace xtime { // avoid colliding with std-c 'time' symbol

struct RADRT_CLASS TimeDate
{
	U16 year;
	U16 millis;
	U8 month;
	U8 dayOfMonth;
	U8 dayOfWeek;
	U8 hour;
	U8 minute;
	U8 second;

	struct local_time_tag_s {};
	struct universal_time_tag_s {};

	static const local_time_tag_s local_time_tag;
	static const universal_time_tag_s universal_time_tag;

	static TimeDate Now(const local_time_tag_s&);
	static TimeDate Now(const universal_time_tag_s&);
	static TimeDate Zero();
	static TimeDate FromString(const char *str);
	
	
	bool Read(stream::InputStream &is, UReg *errorCode = 0);
	bool Write(stream::OutputStream &os, UReg *errorCode = 0) const;

	string::String ToString() const;

	bool operator == (const TimeDate &td) const;
	bool operator != (const TimeDate &td) const;
	bool operator > (const TimeDate &td) const;
	bool operator < (const TimeDate &td) const;
	bool operator >= (const TimeDate &td) const;
	bool operator <= (const TimeDate &td) const;

	int Compare(const TimeDate &td) const;
};

class MicroClock
{
public:
	typedef TimeVal ClockValue;
	enum { TicksPerSecond = 1000*1000	};

	MicroClock();
	~MicroClock();

	TimeVal Read();

	static TimeVal WrapElapsed(TimeVal start, TimeVal end);
};

class MilliClock
{
public:
	typedef TimeVal ClockValue;
	enum { TicksPerSecond = 1000 };

	MilliClock();
	~MilliClock();

	TimeVal Read();

	static TimeVal WrapElapsed(TimeVal start, TimeVal end);
};

//////////////////////////////////////////////////////////////////////////////////////////
// Timers
//////////////////////////////////////////////////////////////////////////////////////////

template <typename ClockType, typename TimerValue = typename ClockType::ClockValue, UReg TimerDivisor = UReg(1)>
class Timer
{
public:
	typedef typename ClockType::ClockValue ClockValue;

	Timer();
	
	void Start();
	void Stop();
	void Resume();
	void SetScale(FReg timeScale);
	FReg Scale() const;
	bool IsTiming() const;

	TimerValue Elapsed(bool applyScale = true) const;
	
	ClockType& Clock() const;

private:

	mutable ClockType m_clock;
	ClockValue m_start;
	ClockValue m_end;
	FReg m_scale;
	bool m_timing;
};

template <typename ClockType = MilliClock>
class SecondsTimer : public Timer<ClockType, FReg, ClockType::TicksPerSecond> {};
class MicroTimer : public Timer<MicroClock, MicroClock::ClockValue> {};
class MilliTimer : public Timer<MilliClock, MilliClock::ClockValue> {};

class TimeSlice : public boost::noncopyable
{
public:
	TimeSlice(TimeVal millis);
	RAD_DECLARE_READONLY_PROPERTY(TimeSlice, remaining, TimeVal);
	RAD_DECLARE_READONLY_PROPERTY(TimeSlice, infinite, bool);
	
	static const TimeSlice Infinite;

	typedef void (TimeSlice::*unspecified_bool_type)();
	operator unspecified_bool_type () const;

private:
	void bool_val() {}
	RAD_DECLARE_GET(remaining, TimeVal);
	RAD_DECLARE_GET(infinite, bool);
	MilliTimer m_timer;
	TimeVal m_time;
};

RADRT_API void RADRT_CALL MilliToDayHourSecond(TimeVal time, UReg* days, UReg* hours, UReg* minutes, FReg* seconds);
RADRT_API TimeVal RADRT_CALL DayHourSecondToMilli(UReg days, UReg hours, UReg minutes, UReg seconds);

TimeVal ReadMilliseconds();
TimeVal ReadMicroseconds();
float ReadSeconds();

boost::xtime xtime_now();
boost::xtime xtime_add_milli(const boost::xtime &time, TimeVal m);
boost::xtime xtime_future(TimeVal m);
boost::posix_time::milliseconds duration(TimeVal m);

} // xtime


#include "../PopPack.h"
#include "Time.inl"
