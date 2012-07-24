/*! \file AppDelegate.mm
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup Main
 */

#include "../NativeApp.h"
#include <Engine/App.h>
#include <Engine/COut.h>
#include <Engine/Engine.h>
#include <Engine/Zones.h>
#include <Engine/Input.h>
#include <Engine/Renderer/PC/RBackend.h>
#include <Runtime/Base.h>
#include <Runtime/File.h>
#include <Runtime/Thread.h>
#include <Runtime/Time.h>
#include <Runtime/Runtime.h>

#if defined(RAD_OPT_GL)
#include <Engine/Renderer/GL/GLTable.h>
#endif

#import "AppDelegate.h"

AppDelegate *s_appd = 0;

@interface AppDelegate (Private)
-(void) appMain;
-(const char**)getArgs:(int&)argc;
-(void)freeArgs:(int)argc:(const char**)argv;
-(void)processEvents;
-(void)dispatchEvent:(NSEvent*)event;
@end

@implementation AppDelegate

- (void)dealloc {
    [super dealloc];
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
	s_appd = self;
	[self appMain];
}

- (void)applicationWillTerminate:(NSNotification*)notification {
	App::Get()->ResetDisplayDevice();
	App::Get()->Finalize();
	
	App::DestroyInstance();
	
	rt::Finalize();
}

@end

@implementation AppDelegate (Private)

- (const char **)getArgs: (int&)argc {
	NSProcessInfo *pinfo = [NSProcessInfo processInfo];
	NSArray *nsargv = [pinfo arguments];
	
	argc = (int)[nsargv count];
	const char **argv = 0;
	
	if (argc > 0)
		argv = new const char *[argc];
	
	for (int i = 0; i < argc; ++i)
		argv[i] = strdup([[nsargv objectAtIndex:(NSUInteger)i] cString]);
	
	return argv;
}

- (void)freeArgs: (int)argc: (const char **)argv {
	for (int i = 0; i < argc; ++i)
		free((void*)argv[i]);
}

- (void)appMain {
	// Insert code here to initialize your application
	
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	[NSApp setServicesProvider:self];
	
	// Pull out arguments.
	int argc;
	const char **argv;
	
	argv = [self getArgs:argc]; // these are leaked.
	
	rt::Initialize();
	RAD_DEBUG_ONLY(file::EnforcePortablePaths(false));
	
	COut(C_Info) << "NativeAppMain..." << std::endl;
	COut(C_Info) << "echo command line: ";
	
	for (int i = 0; i < argc; ++i) {
		COut(C_Info) << argv[i] << " ";
	}
	
	COut(C_Info) << std::endl;
	
	App *app = App::Get();
	
	[NSApp activateIgnoringOtherApps:YES];
	
	if (!app->PreInit()) {
		app->ResetDisplayDevice();
		NSRunAlertPanel(@"Error", @"Initialization failed! See log.txt for details.", nil, nil, nil);
		[NSApp terminate:nil];
	}
	
	if (!app->engine->sys->r->ctx.get()) {
		COut(C_Error) << "Rendering system was not initialized (Developer note your custom PreInit method must set the rendering context before returning!" << std::endl;
		NSRunAlertPanel(@"Error", @"Rendering system was not initialized! See log.txt for details.", nil, nil, nil);
		[NSApp terminate:nil];
	}
	
	r::HRBackend rb = app->engine->sys->r.Cast<r::IRBackend>();
	
	if (!rb->VidBind()) {
		app->ResetDisplayDevice();
		NSRunAlertPanel(@"Error", @"Failed to bind rendering device! See log.txt for details.", nil, nil, nil);
		[NSApp terminate:nil];
	}
	
	if (!rb->CheckCaps()) {
		app->ResetDisplayDevice();
		NSRunAlertPanel(@"Error", @"Unsupported video card detected! See log.txt for details.", nil, nil, nil);
		[NSApp terminate:nil];
	}
	
	if (!(app->Initialize() && app->Run())) {
		app->ResetDisplayDevice();
		NSRunAlertPanel(@"Error", @"Initialization failed! See log.txt for details.", nil, nil, nil);
		[NSApp terminate:nil];
	}
	
	while (!app->exit) {
		[self processEvents];
		app->Tick();
		[pool release];
		pool = [[NSAutoreleasePool alloc] init];
	}
	
	[NSApp terminate:nil];
}

-(void)processEvents {
	NSEvent *e;
	
	while ((e=[NSApp nextEventMatchingMask: NSAnyEventMask untilDate: nil inMode: NSDefaultRunLoopMode dequeue: YES])) {
		[self dispatchEvent: e];
	}
}

-(void)dispatchEvent: (NSEvent*) event {
	NSEventType type = [event type];
	
	switch (type) {
	case NSLeftMouseDown:
	case NSLeftMouseUp:
	case NSRightMouseDown:
	case NSRightMouseUp:
	case NSMouseMoved:
	case NSLeftMouseDragged:
	case NSRightMouseDragged:
	case NSKeyDown:
		/*if ([ event modifierFlags ] & NSCommandKeyMask) {
		}*/
	case NSKeyUp:
	case NSFlagsChanged:
	case NSSystemDefined:
	case NSScrollWheel:
		return;
	}
	[NSApp sendEvent: event];
}

@end
