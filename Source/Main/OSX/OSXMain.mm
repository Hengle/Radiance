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
	
#if !defined(RAD_OPT_SHIP)
	bool setwd = true;
	for (int i = 1; i < argc; ++i) {
		if (!strcmp("-nosetwd", argv[i])) {
			setwd = false;
			break;
		}
	}
	
	if (setwd) {
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
#endif
		
	char wd[1024];
	getcwd(wd, 1024);
	COut(C_Debug) << "Working Dir: " << wd << std::endl;
	
#if defined(RAD_OPT_PC_TOOLS)
	return QtAppMain(argc, (const char **)argv);
#else
	bool launcher = true;
	for (int i = 0; i < argc; ++i) {
		if (!strcmp("-nolauncher", argv[i])) {
			launcher = false;
			break;
		}
	}
	
	if (launcher)
		return App::DoLauncher(argc, (const char**)argv);
	
	return NSApplicationMain(argc, (const char **)argv);
#endif
	
}

void __OSX_SpawnSandboxed(const char *application) {
	NSString *bundleId = [NSString stringWithUTF8String: application];
	
	NSURL *url = [[NSWorkspace sharedWorkspace] URLForApplicationWithBundleIdentifier: bundleId];
	
	NSError *error = 0;
	
	[[NSWorkspace sharedWorkspace] launchApplicationAtURL: url options: NSWorkspaceLaunchAsync|NSWorkspaceLaunchNewInstance configuration: nil error: &error];
	
	if (error) {
		NSString *s = [error localizedDescription];
		if (s) {
			const char *sz = [s cStringUsingEncoding: NSUTF8StringEncoding];
			COut(C_Error) << sz << std::endl;
		}
	}
}

void __OSX_BundlePath(char *dst) {
	NSString *bPath = [[NSBundle mainBundle] bundlePath];
	strcpy(dst, [bPath cStringUsingEncoding:NSASCIIStringEncoding]);
}
	
FILE *__OSX_OpenPersistence(const char *name, const char *mode) {
	static char s_basePath[1024] = {0};
	
	if (s_basePath[0] == 0) {
		NSString *path = NSHomeDirectory();
		strcpy(s_basePath, [path cStringUsingEncoding:NSASCIIStringEncoding]);
	}
	
	String x(s_basePath);
	x += '/';
	x += name;
	
	return fopen(x.c_str, mode);
}
