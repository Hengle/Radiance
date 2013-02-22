// Time.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "IntTime.h"
#include "Time.h"
#include <boost/thread/thread_time.hpp>
#include <boost/date_time/posix_time/conversion.hpp>
#include "boost/date_time/posix_time/posix_time_types.hpp"
#include <stdio.h>

namespace xtime {
	
const TimeSlice TimeSlice::Infinite(std::numeric_limits<TimeVal>::max());

RADRT_API void RADRT_CALL MilliToDayHourSecond(TimeVal time, UReg* days, UReg* hours, UReg* minutes, FReg* seconds)
{
	*days = (UReg)(time / Constants<UReg>::MillisPerDay());
	time -= (TimeVal)(*days * Constants<UReg>::MillisPerDay());
	*hours = (UReg)(time / Constants<UReg>::MillisPerHour());
	time -= (TimeVal)(*hours * Constants<UReg>::MillisPerHour());
	*minutes = (UReg)(time / Constants<UReg>::MillisPerMinute());
	time -= (TimeVal)(*minutes * Constants<UReg>::MillisPerMinute());
	*seconds = (FReg)((FReg)time / Constants<FReg>::MillisPerSecond());
}

RADRT_API TimeVal RADRT_CALL DayHourSecondToMilli(UReg days, UReg hours, UReg minutes, UReg seconds)
{
	return (TimeVal)(
		days * Constants<UReg>::MillisPerDay() +
		hours * Constants<UReg>::MillisPerHour() +
		minutes * Constants<UReg>::MillisPerMinute() +
		seconds * Constants<UReg>::MillisPerSecond()
	);
}

const TimeDate::local_time_tag_s TimeDate::local_time_tag;
const TimeDate::universal_time_tag_s TimeDate::universal_time_tag;

TimeDate TimeDate::Now(const local_time_tag_s&)
{
	std::tm t = boost::posix_time::to_tm(boost::posix_time::second_clock::local_time());

	TimeDate td;
	td.year = 1900 + (U16)t.tm_year;
	td.millis = 0;
	td.month = (U8)t.tm_mon + 1;
	td.dayOfMonth = (U8)t.tm_mday;
	td.dayOfWeek = (U8)t.tm_wday;
	td.hour = (U8)t.tm_hour;
	td.minute = (U8)t.tm_min;
	td.second = (U8)t.tm_sec;
	return td;
}

TimeDate TimeDate::Now(const universal_time_tag_s&)
{
	std::tm t = boost::posix_time::to_tm(boost::posix_time::second_clock::universal_time());

	TimeDate td;
	td.year = 1900 + (U16)t.tm_year;
	td.millis = 0;
	td.month = (U8)t.tm_mon + 1;
	td.dayOfMonth = (U8)t.tm_mday;
	td.dayOfWeek = (U8)t.tm_wday;
	td.hour = (U8)t.tm_hour;
	td.minute = (U8)t.tm_min;
	td.second = (U8)t.tm_sec;
	return td;
}

TimeDate TimeDate::Zero() {
	TimeDate td;
	td.year = 0;
	td.millis = 0;
	td.month = 0;
	td.month = 0;
	td.dayOfMonth = 0;
	td.dayOfWeek = 0;
	td.hour = 0;
	td.minute = 0;
	td.second = 0;
	return td;
}

TimeDate TimeDate::FromString(const char *str)
{
	int year, millis, month, dayOfMonth, dayOfWeek, hour, minute, second;
	sscanf(str, "%d %d %d %d %d %d %d %d",
		&year,
		&millis,
		&month,
		&dayOfMonth,
		&dayOfWeek,
		&hour,
		&minute,
		&second
	);

	TimeDate td;
	td.year = (U16)year;
	td.millis = (U16)millis;
	td.month = (U8)month;
	td.dayOfMonth = (U8)dayOfMonth;
	td.dayOfWeek = (U8)dayOfWeek;
	td.hour = (U8)hour;
	td.minute = (U8)minute;
	td.second = (U8)second;

	return td;
}

bool TimeDate::Read(stream::InputStream &is, UReg *errorCode) {
	return is.Read(&year, errorCode) &&
		is.Read(&millis, errorCode) &&
		is.Read(&month, errorCode) &&
		is.Read(&dayOfMonth, errorCode) &&
		is.Read(&dayOfWeek, errorCode) &&
		is.Read(&hour, errorCode) &&
		is.Read(&minute, errorCode) &&
		is.Read(&second);
}

bool TimeDate::Write(stream::OutputStream &os, UReg *errorCode) const {
	return os.Write(year, errorCode) &&
		os.Write(millis, errorCode) &&
		os.Write(month, errorCode) &&
		os.Write(dayOfMonth, errorCode) &&
		os.Write(dayOfWeek, errorCode) &&
		os.Write(hour, errorCode) &&
		os.Write(minute, errorCode) &&
		os.Write(second);
}

string::String TimeDate::ToString() const
{
	string::String s;
	s.Printf("%d %d %d %d %d %d %d %d",
		(int)year,
		(int)millis,
		(int)month,
		(int)dayOfMonth,
		(int)dayOfWeek,
		(int)hour,
		(int)minute,
		(int)second
	);

	return s;
}

int TimeDate::Compare(const TimeDate &td) const
{
	if (year > td.year)
		return 1;
	if (year < td.year)
		return -1;
	if (month > td.month)
		return 1;
	if (month < td.month)
		return -1;
	if (dayOfMonth > td.dayOfMonth)
		return 1;
	if (dayOfMonth < td.dayOfMonth)
		return -1;
	if (hour > td.hour)
		return 1;
	if (hour < td.hour)
		return -1;
	if (minute > td.minute)
		return 1;
	if (minute < td.minute)
		return -1;
	if (second > td.second)
		return 1;
	if (second < td.second)
		return -1;
	if (millis > td.millis)
		return 1;
	if (millis < td.millis)
		return -1;
	if (dayOfWeek > td.dayOfWeek)
		return 1;
	if (dayOfWeek < td.dayOfWeek)
		return -1;
	return 0;
}

} // xtime

