// LogFile.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "LogFile.h"
#include <stdio.h>
#include <iostream>
#include <Runtime/Stream/STLStream.h>
#include <boost/thread/tss.hpp>

LogFile::LogFile() : m_fp(0)
{
	Open();
}

LogFile::~LogFile()
{
	Close();
}

void LogFile::Open()
{
	Close();
	m_fp = fopen("log.txt", "wt");
}

void LogFile::Close()
{
	if (m_fp)
	{
		fclose((FILE*)m_fp);
		m_fp = 0;
	}
}

void LogFile::Write(const char *str)
{
	Lock l(m_m);
	
	if (m_fp&&str&&str[0])
	{
		fprintf((FILE*)m_fp, "%s", str);
		fflush((FILE*)m_fp);
	}
#if defined(RAD_OPT_IOS)
	std::cerr << str << std::flush;
#else
	std::cout << str << std::flush;
#endif
#if defined(RAD_OPT_WIN)
	RAD_DEBUG_ONLY(DebugString(str));
#endif
}

LogFile &LogFile::Get()
{
	static LogFile x;
	return x;
}

namespace {

class LogStringBuf : public stream::basic_stringbuf<char>
{
public:
	typedef stream::basic_stringbuf<char> Super;
	typedef Super::StringType StringType;

	LogStringBuf() {}

private:

	virtual int Flush(const StringType &str)
	{
		LogFile::Get().Write(str.c_str());
		return 0;
	}
};

boost::thread_specific_ptr<LogStringBuf> s_logStringBufPtr;
boost::thread_specific_ptr<std::ostream> s_ostreamPtr;

} // namespace

std::ostream &Log()
{
	if (s_ostreamPtr.get() == 0)
	{
		RAD_ASSERT(s_logStringBufPtr.get() == 0);
		s_logStringBufPtr.reset(new LogStringBuf());
		s_ostreamPtr.reset(new std::ostream(s_logStringBufPtr.get()));
	}
	return *s_ostreamPtr.get();
}
