// SystemLanguage.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Application entry point for pc platforms.
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

// Implements __SystemLanguage()

#include <Engine/StringTable.h>

#if defined(RAD_OPT_WINX)

#include <Runtime/Win/WinHeaders.h>

StringTable::LangId __SystemLanguage() {
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

#else if (RAD_OPT_APPLE)

// We use the function provided in IOSMacLangPrefs.mm which extracts the user-language prefs from CoreFoundation
// and implement the Radiance __SystemLanguage() function.

#include <vector>
#include <string>

// __CopyIOSMacPreferredLangs is in IOSMacLangPrefs.mm
void __CopyIOSMacPreferredLangs(std::vector<std::string> &langs);

StringTable::LangId __SystemLanguage() {
	StringTable::LangId lang = StringTable::LangId_EN;
	
	std::vector<std::string> langs;
	__CopyIOSMacPreferredLangs(langs);
	
	for (std::vector<std::string>::const_iterator it = langs.begin(); it != langs.end(); ++it) {
		
		StringTable::LangId selected = StringTable::LangId_MAX;
		
		const std::string &str = *it;
		
		if (string::ncmp(str.c_str(), "en", 2) == 0) {
			selected = StringTable::LangId_EN;
		} else if (string::ncmp(str.c_str(), "zh", 2) == 0) {
			selected = StringTable::LangId_CH;
		} else if (string::ncmp(str.c_str(), "fr", 2) == 0) {
			selected = StringTable::LangId_FR;
		} else if (string::ncmp(str.c_str(), "de", 2) == 0) {
			selected = StringTable::LangId_GR;
		} else if (string::ncmp(str.c_str(), "it", 2) == 0) {
			selected = StringTable::LangId_IT;
		} else if (string::ncmp(str.c_str(), "ja", 2) == 0) {
			selected = StringTable::LangId_JP;
		} else if (string::ncmp(str.c_str(), "ru", 2) == 0) {
			selected = StringTable::LangId_RU;
		} else if (string::ncmp(str.c_str(), "be", 2) == 0) { // Belarussian
			selected = StringTable::LangId_RU;
		} else if (string::ncmp(str.c_str(), "uk", 2) == 0) { // Ukranian
			selected = StringTable::LangId_RU;
		} else if (string::ncmp(str.c_str(), "es", 2) == 0) {
			selected = StringTable::LangId_SP;
		}
		
		if (selected != StringTable::LangId_MAX) {
			lang = selected;
			break;
		}
	}
	
	return lang;
}

#endif
