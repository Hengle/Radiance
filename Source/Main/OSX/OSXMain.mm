/*! \file OSXMain.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup Main
*/

#include <Engine/App.h>

#if defined(RAD_OPT_PC_TOOLS)
#include "../QtAppMain.h"
#include <string.h>
#include <unistd.h>
#import <CoreFoundation/CoreFoundation.h>
#endif

#import <AppKit/NSWorkspace.h>
#import <Cocoa/Cocoa.h>
#include <stdio.h>

extern "C" int main(int argc, char *argv[]) {
	
#if !defined(RAD_TARGET_GOLDEN)
	bool setwd = true;
	for (int i = 1; i < argc; ++i) {
		if (!strcmp("-nosetwd", argv[i])) {
			setwd = false;
			break;
		}
	}
	
	if (setwd) {
		// (tools only): Set working directory to the same folder that our .app bundle is in.
		char bundlepath[1024];
		CFURLRef url = CFBundleCopyBundleURL(CFBundleGetMainBundle());
		if (CFURLGetFileSystemRepresentation(url, 1, (UInt8 *)bundlepath, 1024)) {
			strcat(bundlepath, "/Contents/Resources");
			chdir(bundlepath);
		}
		CFURLRef url2 = CFURLCreateCopyDeletingLastPathComponent(0, url);
		if (CFURLGetFileSystemRepresentation(url2, 1, (UInt8 *)bundlepath, 1024)) {
			chdir(bundlepath);
		}
		CFRelease(url2);
		CFRelease(url);
	}
		
	char wd[1024];
	getcwd(wd, 1024);
	COut(C_Info) << "Working Dir: " << wd << std::endl;
#endif

#if defined(RAD_OPT_PC_TOOLS)
	return QtAppMain(argc, (const char **)argv);
#else
	return NSApplicationMain(argc, (const char **)argv);
#endif
	
}

