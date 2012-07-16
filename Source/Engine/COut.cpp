// COut.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Console Print
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "COut.h"
#include "LogFile.h"
#include <Runtime/Stream/STLStream.h>
#include <Runtime/String.h>
#include <boost/thread/tss.hpp>

#if defined(RAD_OPT_PC_TOOLS)
#include "Tools/Editor/EditorLogWindow.h"
#include "Tools/Editor/EditorCookerDialog.h"
#endif

#undef MessageBox

namespace {

class ConStringBuf : public stream::basic_stringbuf<char>
{
public:
	typedef stream::basic_stringbuf<char> Super;
	typedef Super::StringType StringType;

	ConStringBuf(int level) : m_level(level) {}
	ConStringBuf() {}

protected:

	virtual int Flush(const StringType &str)
	{
#if defined(RAD_OPT_PC_TOOLS)
		tools::editor::CookerDialog::SPrint(str.c_str());
		bool dialog = tools::editor::LogWindow::SPrint(m_level, str.c_str());
#endif
#if !defined(RAD_OPT_DEBUG) && (defined(RAD_OPT_SHIP) || defined(RAD_OPT_ADHOC))
		if (m_level != C_Debug)
#endif
		Log() << str.c_str() << std::flush;
#if defined(RAD_OPT_PC)
		if (m_level == C_ErrMsgBox
#if defined(RAD_OPT_PC_TOOLS)
			&& !dialog
#endif
			)
		{
			MessageBox("Error", str.c_str(), MBStyleOk);
		}
#endif
		return 0;
	}

private:

	int m_level;
};

boost::thread_specific_ptr<ConStringBuf> s_conStringBufPtr[C_Max];
boost::thread_specific_ptr<std::ostream> s_ostreamPtr[C_Max];

} // namespace

RADENG_API std::ostream &RADENG_CALL COut(int level)
{
	RAD_ASSERT(level >= 0 && level < C_Max);

	if (s_ostreamPtr[level].get() == 0)
	{
		RAD_ASSERT(s_conStringBufPtr[level].get() == 0);
		s_conStringBufPtr[level].reset(new ConStringBuf(level));
		s_ostreamPtr[level].reset(new std::ostream(s_conStringBufPtr[level].get()));
	}
	return *s_ostreamPtr[level].get();
}

#if !defined(RAD_OPT_NO_REFLECTION) // lua bindings
#include "../Runtime/ReflectDef.h"
#include "Lua/LuaRuntime.h"

LUART_IMPLEMENT_API(RADENG_API, COut);

class Con
{
public:
	static void Write(int level, const char *string)
	{
		COut(level) << string;
	}
};

RADREFLECT_DECLARE(RADENG_API, Con);

#include "../Runtime/ReflectMap.h"

// NOTE: we can call this enum the same name as the Con class
// Lua will just stick them in the same table.
RADREFLECT_BEGIN_ENUM("Con", COutLevel)
	RADREFLECT_ENUM_VALUE_NAMED("Debug", C_Debug)
	RADREFLECT_ENUM_VALUE_NAMED("Info",  C_Info)
	RADREFLECT_ENUM_VALUE_NAMED("Warn", C_Warn)
	RADREFLECT_ENUM_VALUE_NAMED("Error", C_Error)
	RADREFLECT_ENUM_VALUE_NAMED("ErrMsgBox", C_ErrMsgBox)
RADREFLECT_END(RADENG_API, COutLevel)

RADREFLECT_BEGIN_CLASS("Con", Con)
	RADREFLECT_BEGIN_STATICMETHOD(void)
		RADREFLECT_ARG("level", int)
		RADREFLECT_ARG("msg", const char*)
	RADREFLECT_END_METHOD(Write)
RADREFLECT_END(RADENG_API, Con)

LUART_REGISTER_TYPE(RADENG_API, COut, COutLevel);
LUART_REGISTER_TYPE(RADENG_API, COut, Con);

#endif
