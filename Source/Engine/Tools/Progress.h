// Progress.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Types.h"
#include <Runtime/Stream/STLStream.h>
#include <Runtime/String.h>
#include <boost/thread/tss.hpp>
#include <Runtime/PushPack.h>

namespace tools {

enum Severity
{
	S_Debug,
	S_Info,
	S_Warn,
	S_Error,
	S_Max
};

class RADENG_CLASS UIProgress : public boost::noncopyable
{
public:
	typedef boost::shared_ptr<UIProgress> Ref;
	virtual ~UIProgress() {}
	virtual std::ostream &Out(Severity s);
	RAD_DECLARE_PROPERTY(UIProgress, total, int, int);
	RAD_DECLARE_PROPERTY(UIProgress, totalProgress, int, int);
	RAD_DECLARE_PROPERTY(UIProgress, title, const char *, const char *);
	RAD_DECLARE_PROPERTY(UIProgress, subTitle, const char *, const char *);
	RAD_DECLARE_PROPERTY(UIProgress, subTotal, int, int);
	RAD_DECLARE_PROPERTY(UIProgress, subProgress, int, int);

	virtual void Step() = 0;
	virtual void SubStep() = 0;

	virtual Ref Child(const char *title, bool progressBar) = 0;
	virtual void Refresh() = 0;

protected:

	virtual RAD_DECLARE_GET(total, int) = 0;
	virtual RAD_DECLARE_SET(total, int) = 0;
	virtual RAD_DECLARE_GET(totalProgress, int) = 0;
	virtual RAD_DECLARE_SET(totalProgress, int) = 0;
	virtual RAD_DECLARE_GET(title, const char *) = 0;
	virtual RAD_DECLARE_SET(title, const char *) = 0;
	virtual RAD_DECLARE_GET(subTitle, const char *) = 0;
	virtual RAD_DECLARE_SET(subTitle, const char *) = 0;
	virtual RAD_DECLARE_GET(subTotal, int) = 0;
	virtual RAD_DECLARE_SET(subTotal, int) = 0;
	virtual RAD_DECLARE_GET(subProgress, int) = 0;
	virtual RAD_DECLARE_SET(subProgress, int) = 0;

	virtual void EmitString(int level, const std::string &str) = 0;

private:

	class UIStringBuf;
	friend class UIStringBuf;

	class UIStringBuf : public stream::basic_stringbuf<char>
	{
	public:
		typedef stream::basic_stringbuf<char> Super;
		typedef Super::StringType StringType;

		UIStringBuf(UIProgress &ui, int level) : m_ui(ui), m_level(level) {}
		~UIStringBuf() {}

	private:

		virtual int Flush(const StringType &str)
		{
			m_ui.EmitString(m_level, str);
			return 0;
		}

		int m_level;
		UIProgress &m_ui;
	};


	static boost::thread_specific_ptr<UIStringBuf> s_uiStringBufPtr[S_Max];
	static boost::thread_specific_ptr<std::ostream> s_ostreamPtr[S_Max];
};

class NullUIProgress_t : public UIProgress
{
public:
	NullUIProgress_t() {};
	virtual ~NullUIProgress_t() {}

	virtual void Step() { ++m_total[1]; }
	virtual void SubStep() { ++m_sub[1]; }
	virtual Ref Child(const char *title, bool progressBar)
	{
		return Ref(new (ZTools) NullUIProgress_t());
	}
	virtual void Refresh() {}

protected:

	RAD_DECLARE_GET(total, int) { return m_total[0]; }
	RAD_DECLARE_SET(total, int) { m_total[0] = value; }
	RAD_DECLARE_GET(totalProgress, int) { return m_total[1]; }
	RAD_DECLARE_SET(totalProgress, int) { m_total[1] = value; }
	RAD_DECLARE_GET(subTotal, int) { return m_sub[0]; }
	RAD_DECLARE_SET(subTotal, int) { m_sub[0] = value; }
	RAD_DECLARE_GET(subProgress, int) { return m_sub[1]; }
	RAD_DECLARE_SET(subProgress, int) { m_sub[1] = value; }
	RAD_DECLARE_GET(subTitle, const char*) { return m_ssub.c_str(); }
	RAD_DECLARE_SET(subTitle, const char*)
	{
		if (!value)
		{
			m_ssub.clear();
			return;
		}
		m_ssub = value;
	}

	RAD_DECLARE_GET(title, const char*) { return m_title.c_str(); }
	RAD_DECLARE_SET(title, const char*)
	{
		if (!value)
		{
			m_title.clear();
			return;
		}
		m_title = value;
	}

	virtual void EmitString(int, const std::string &) {}

private:

	int m_total[2];
	int m_sub[2];
	String m_title;
	String m_ssub;
};

extern RADENG_API NullUIProgress_t NullUIProgress;

} // tools

#include <Runtime/PopPack.h>
