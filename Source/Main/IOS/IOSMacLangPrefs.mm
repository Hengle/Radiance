// IOSMacLangPrefs.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Application entry point for pc platforms.
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#import <CoreFoundation/CoreFoundation.h>
#include <vector>
#include <string>

// Extract language prefs from CF

void __CopyIOSMacPreferredLangs(std::vector<std::string> &langs) {
	
	CFArrayRef prefs = CFLocaleCopyPreferredLanguages();
	const CFIndex NumLangs = CFArrayGetCount(prefs);
	CFRange cmpRange;
	cmpRange.location = 0;
	cmpRange.length = 2;
	
	for (CFIndex i = 0; i < NumLangs; ++i) {
		
		CFStringRef str = (CFStringRef)CFArrayGetValueAtIndex(prefs, i);
		
		if (str) {
			char buf[256];
			CFStringGetCString(str, buf, 256, kCFStringEncodingASCII);
			langs.push_back(std::string(buf));
		}
	}
}
