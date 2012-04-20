// LogFile.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "Types.h"
#include <iostream>
#include <boost/thread/mutex.hpp>

class RADENG_CLASS LogFile
{
public:
	LogFile();
	~LogFile();
	void Write(const char *str);

	static LogFile &Get();

private:

	typedef boost::mutex Mutex;
	typedef boost::lock_guard<Mutex> Lock;

	void Open();
	void Close();

	Mutex m_m;
	void *m_fp;

};

std::ostream &Log();

