// Time.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "../PushSystemMacros.h"

namespace xtime {

inline TimeVal ReadMilliseconds()
{
	boost::xtime t;
	boost::xtime_get(&t, boost::TIME_UTC);
	return (TimeVal)(Constants<boost::xtime::xtime_sec_t>::SecondsToMilli(t.sec) + t.nsec / (1000*1000));
}

inline TimeVal ReadMicroseconds()
{
	boost::xtime t;
	boost::xtime_get(&t, boost::TIME_UTC);
	return (TimeVal)(Constants<boost::xtime::xtime_sec_t>::SecondsToMicro(t.sec) + t.nsec / 1000);
}

inline float ReadSeconds()
{
	return ReadMilliseconds() / 1000.f;
}

inline boost::xtime xtime_now()
{
	boost::xtime t;
	boost::xtime_get(&t, boost::TIME_UTC);
	return t;
}

inline boost::xtime xtime_add_milli(const boost::xtime &time, TimeVal m)
{
	boost::xtime r;
	U64 nsec = time.nsec;
	nsec += (m * 1000 * 1000);
	boost::xtime::xtime_sec_t overflow = (boost::xtime::xtime_sec_t)(nsec / (1000*1000*1000));
	nsec -= overflow * 1000*1000*1000;
	r.sec = time.sec + overflow;
	r.nsec = (boost::xtime::xtime_nsec_t)nsec;
	return r;
}

inline boost::xtime xtime_future(TimeVal m)
{
	return xtime_add_milli(xtime_now(), m);
}

inline boost::posix_time::milliseconds duration(TimeVal m)
{
	return boost::posix_time::milliseconds(m);
}

//////////////////////////////////////////////////////////////////////////////////////////

inline bool TimeDate::operator == (const TimeDate &td) const
{
	return Compare(td) == 0;
}

inline bool TimeDate::operator != (const TimeDate &td) const
{
	return Compare(td) != 0;
}

inline bool TimeDate::operator > (const TimeDate &td) const
{
	return Compare(td) > 0;
}

inline bool TimeDate::operator < (const TimeDate &td) const
{
	return Compare(td) < 0;
}

inline bool TimeDate::operator >= (const TimeDate &td) const
{
	return Compare(td) >= 0;
}

inline bool TimeDate::operator <= (const TimeDate &td) const
{
	return Compare(td) <= 0;
}

//////////////////////////////////////////////////////////////////////////////////////////

inline MicroClock::MicroClock() 
{}
inline MicroClock::~MicroClock()
{}
inline TimeVal MicroClock::Read()
{
	return ReadMicroseconds();
}
inline TimeVal MicroClock::WrapElapsed(TimeVal start, TimeVal end)
{
	if (end >= start) return end - start;
	// wrap.
	return (std::numeric_limits<TimeVal>::max() - start) + end;
}

inline MilliClock::MilliClock()
{}
inline MilliClock::~MilliClock()
{}

inline TimeVal MilliClock::Read()
{
	return ReadMilliseconds();
}

inline TimeVal MilliClock::WrapElapsed(TimeVal start, TimeVal end)
{
	if (end >= start) return end - start;
	// wrap.
	return (std::numeric_limits<TimeVal>::max() - start) + end;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Timer
//////////////////////////////////////////////////////////////////////////////////////////

template <typename ClockType, typename TimerValue, UReg TimerDivisor>
inline Timer<ClockType, TimerValue, TimerDivisor>::Timer() : 
m_scale(FReg(1)), m_timing(false), m_start(0), m_end(0)
{
}

template <typename ClockType, typename TimerValue, UReg TimerDivisor>
inline void Timer<ClockType, TimerValue, TimerDivisor>::Start()
{
	m_timing = true;
	m_start = m_end = m_clock.Read();
}

template <typename ClockType, typename TimerValue, UReg TimerDivisor>
inline void Timer<ClockType, TimerValue, TimerDivisor>::Stop()
{
	m_end = m_clock.Read();
	m_timing = false;
}

template <typename ClockType, typename TimerValue, UReg TimerDivisor>
inline bool Timer<ClockType, TimerValue, TimerDivisor>::IsTiming() const
{
	return m_timing;
}

template <typename ClockType, typename TimerValue, UReg TimerDivisor>
inline void Timer<ClockType, TimerValue, TimerDivisor>::Resume()
{
	RAD_ASSERT(m_timing == false);
	m_timing = true;
	ClockValue val = m_clock.Read();
	m_start = val - ClockType::WrapElapsed(m_start, m_end);
	m_end = val;
}

template <typename ClockType, typename TimerValue, UReg TimerDivisor>
inline void Timer<ClockType, TimerValue, TimerDivisor>::SetScale(FReg timeScale)
{
	m_scale = timeScale;
}

template <typename ClockType, typename TimerValue, UReg TimerDivisor>
inline FReg Timer<ClockType, TimerValue, TimerDivisor>::Scale() const
{
	return m_scale;
}

template <typename ClockType, typename TimerValue, UReg TimerDivisor>
TimerValue Timer<ClockType, TimerValue, TimerDivisor>::Elapsed(bool applyScale) const
{
	ClockValue cv_d;
	if (m_timing)
	{
		cv_d = ClockType::WrapElapsed(m_start, m_clock.Read());
	}
	else
	{
		cv_d = ClockType::WrapElapsed(m_start, m_end);
	}

	TimerValue d = TimerValue(cv_d) / TimerValue(TimerDivisor);
	
	if (applyScale)
	{
		d *= TimerValue(m_scale);
	}

	return d;
}

template <typename ClockType, typename TimerValue, UReg TimerDivisor>
inline ClockType& Timer<ClockType, TimerValue, TimerDivisor>::Clock() const
{
	return m_clock;
}

inline TimeSlice::TimeSlice(TimeVal millis) :
m_time(millis)
{
	m_timer.Start();
}

inline TimeVal TimeSlice::RAD_IMPLEMENT_GET(remaining)
{
	TimeVal x = m_timer.Elapsed(false);
	return (m_time==MaxUReg) ? MaxUReg : (x < m_time) ? (m_time-x) : 0;
}

inline bool TimeSlice::RAD_IMPLEMENT_GET(infinite)
{
	return this->remaining == MaxUReg;
}

inline TimeSlice::operator TimeSlice::unspecified_bool_type () const
{
	return (remaining > 0) ? &TimeSlice::bool_val : 0;
}

} // namespace time

#include "../PopSystemMacros.h"
