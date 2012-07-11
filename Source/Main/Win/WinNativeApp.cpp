/*! \file WinNativeApp.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup Main
*/

#include RADPCH
#include "../NativeApp.h"
#include <Runtime/Win/WinHeaders.h>
#include <ShellAPI.h>

namespace details {

NativeApp::NativeApp()
#if defined(RAD_OPT_PC_TOOLS)
: m_quit(false)
#endif
{
}

bool NativeApp::BindDisplayDevice(const ::DisplayDeviceRef &display, const r::VidMode &mode) {
	return false;
}

void NativeApp::ResetDisplayDevice() {
}

void NativeApp::LaunchURL(const char *sz) {
	RAD_ASSERT(sz);
	ShellExecuteA(0, "open", sz, 0, 0, SW_SHOWNORMAL);
}

StringTable::LangId NativeApp::RAD_IMPLEMENT_GET(systemLangId) {
	StringTable::LangId id = StringTable::LangId_EN;
	
	LANGID winId = GetUserDefaultUILanguage();
	
	switch (winId&0xff) {
		case LANG_CHINESE:
			id = StringTable::LangId_CH;
			break;
		case LANG_FRENCH:
			id = StringTable::LangId_FR;
			break;
		case LANG_GERMAN:
			id = StringTable::LangId_GR;
			break;
		case LANG_ITALIAN:
			id = StringTable::LangId_IT;
			break;
		case LANG_JAPANESE:
			id = StringTable::LangId_JP;
			break;
		case LANG_RUSSIAN:
		case LANG_UKRAINIAN:
			id = StringTable::LangId_RU;
			break;
		case LANG_SPANISH:
			id = StringTable::LangId_SP;
			break;
		default:
			break;
	}
	
	return id;
}

const ::DisplayDeviceVec &NativeApp::RAD_IMPLEMENT_GET(displayDevices) {
	return m_displayDevices;
}

} // details
