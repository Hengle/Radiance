// IOSMain.mm
// Copyright (c) 2010 Pyramind Labs LLC, All Rights Reserved
// Application entry point for ios platforms.
// Author: Joe Riedel (joeriedel@hotmail.com)
// See Radiance/LICENSE for licensing terms.

#import <UIKit/UIKit.h>
#import "IOSAppDelegate.h"

namespace {
	
int s_argc = 0;
const char **s_argv = 0;
	
}

////////////////////////////////////////////////////////////////////////////////

int __Argc() { return s_argc; }
const char **__Argv() { return s_argv; }

int main(int argc, char *argv[]) 
{
	DebugString("Main entered...\n");
	s_argc = argc;
	s_argv = (const char**)argv;
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    int retVal = UIApplicationMain(argc, argv, nil, NSStringFromClass([IOSAppDelegate class]));
    [pool release];
    return retVal;
}


