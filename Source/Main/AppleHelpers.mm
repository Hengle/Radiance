/*! \file AppleHelpers.mm
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup main
*/

#include <Runtime/Base.h>
#include <Engine/StringTable.h>

#import <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>

StringTable::LangId AppleGetLangId() {
	
	CFArrayRef prefs = CFLocaleCopyPreferredLanguages();
	const CFIndex NumLangs = CFArrayGetCount(prefs);
	CFRange cmpRange;
	cmpRange.location = 0;
	cmpRange.length = 2;
	
#define CFCMP(_cfs, _cstr) CFStringCompareWithOptions(_cfs, CFSTR(_cstr), cmpRange, 0)
	
	StringTable::LangId lang = StringTable::LangId_EN;
	
	for (CFIndex i = 0; i < NumLangs; ++i) {
		CFStringRef str = (CFStringRef)CFArrayGetValueAtIndex(prefs, i);
		
		if (!str)
			continue;
			
		StringTable::LangId selected = StringTable::LangId_MAX;
		
		if (CFCMP(str, "en") == 0) {
			selected = StringTable::LangId_EN;
		} else if (CFCMP(str, "zh") == 0) {
			selected = StringTable::LangId_CH;
		} else if (CFCMP(str, "fr") == 0) {
			selected = StringTable::LangId_FR;
		} else if (CFCMP(str, "de") == 0) {
			selected = StringTable::LangId_GR;
		} else if (CFCMP(str, "it") == 0) {
			selected = StringTable::LangId_IT;
		} else if (CFCMP(str, "ja") == 0) {
			selected = StringTable::LangId_JP;
		} else if (CFCMP(str, "ru") == 0) {
			selected = StringTable::LangId_RU;
		} else if (CFCMP(str, "be") == 0) { // Belarussian
			selected = StringTable::LangId_RU;
		} else if (CFCMP(str, "uk") == 0) { // Ukranian
			selected = StringTable::LangId_RU;
		} else if (CFCMP(str, "es") == 0) {
			selected = StringTable::LangId_SP;
		}
		
		if (selected != StringTable::LangId_MAX) {
			lang = selected;
			break;
		}
	}
	
	CFRelease(prefs);
	return lang;
}

String AppleGetFileSystemRoot() {
	NSString *path = [[NSBundle mainBundle] bundlePath];
	String s([path cStringUsingEncoding:NSASCIIStringEncoding]);
#if defined(RAD_OPT_OSX)
	s += "/Contents/Resources";
#endif
	return s;
}

FILE *AppleOpenPersistence(const char *name, const char *mode) {
	static char s_basePath[1024] = {0};
	
	if (s_basePath[0] == 0) {
	
#if defined(RAD_OPT_OSX)
		NSString *path = NSHomeDirectory();
#else
		NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
		NSString *path = [paths objectAtIndex:0];
#endif
		strcpy(s_basePath, [path cStringUsingEncoding:NSASCIIStringEncoding]);
	}
	
	String x(CStr(s_basePath));
	x += '/';
	x += name;
	
	return fopen(x.c_str, mode);
}

