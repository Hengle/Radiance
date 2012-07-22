/*! \file OSXMain.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup Main
*/

#if defined(RAD_OPT_PC_TOOLS)
#include "../QtAppMain.h"
#include <string.h>
#include <unistd.h>
#import <CoreFoundation/CoreFoundation.h>
#else
#import <Cocoa/Cocoa.h>
#endif

extern "C" int main(int argc, char *argv[]) {
	
#if defined(RAD_OPT_PC_TOOLS)
	if (argc < 2 || strcmp(argv[1], "-nosetwd")) {
		// Set working directory to the same folder that our .app bundle is in.
		char bundlepath[1024];
		CFURLRef url = CFBundleCopyBundleURL(CFBundleGetMainBundle());
		CFURLRef url2 = CFURLCreateCopyDeletingLastPathComponent(0, url);
		if (CFURLGetFileSystemRepresentation(url2, 1, (UInt8 *)bundlepath, 1024)) {
			chdir(bundlepath);
		}
		CFRelease(url);
		CFRelease(url2);
	}
	return QtAppMain(argc, (const char **)argv);
#else
	return NSApplicationMain(argc, (const char **)argv);
#endif
	
}
