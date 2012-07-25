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
#import <AppKit/NSWorkspace.h>

AppDelegate *s_appd = 0;

// See Carbon/HIToolbox/Events.h
// Missing or unconvertable characters are question marks (?), helps to see if something is missing.

static int s_vkeys_en[256] = {
/*0x00*/ 'a', 's', 'd', 'f', 'h', 'g', 'z', 'x',
/*0x08*/ 'c', 'v', '?', 'b', 'q', 'w', 'e', 'r',
/*0x10*/ 'y', 't', '1', '2', '3', '4', '6', '5',
/*0x18*/ '=', '9', '7', '-', '8', '0', ']', 'o',
/*0x20*/ 'u', '[', 'i', 'p', kKeyCode_Return, 'l', 'j', '\'',
/*0x28*/ 'k', ';', '\\', ',', '/', 'n', 'm', '.',
/*0x30*/ kKeyCode_Tab, kKeyCode_Space, '`', kKeyCode_Backspace, '?', kKeyCode_Escape, '?', kKeyCode_LCommand,
/*0x38*/ kKeyCode_LShift, kKeyCode_CapsLock, kKeyCode_LAlt, kKeyCode_LCtrl, kKeyCode_RShift, kKeyCode_RAlt, kKeyCode_RCtrl,'?',
/*0x40*/ '?', kKeyCode_KP_Period, '?', kKeyCode_KP_Multiply, '?', kKeyCode_KP_Plus, '?', kKeyCode_NumLock,
/*0x48*/ '?', '?', '?', kKeyCode_KP_Divide, kKeyCode_KP_Enter, '?', kKeyCode_KP_Minus, '?',
/*0x50*/ '?', kKeyCode_KP_Equals, kKeyCode_KP0, kKeyCode_KP1, kKeyCode_KP2, kKeyCode_KP3, kKeyCode_KP4, kKeyCode_KP5,
/*0x58*/ kKeyCode_KP6, kKeyCode_KP7, '?', kKeyCode_KP8, kKeyCode_KP9, '?', '?', '?',
/*0x60*/ kKeyCode_F5, kKeyCode_F6, kKeyCode_F7, kKeyCode_F3, kKeyCode_F8, kKeyCode_F9, '?', kKeyCode_F11,
/*0x68*/ '?', kKeyCode_F13, '?', kKeyCode_F14, '?', kKeyCode_F10, '?', kKeyCode_F12,
/*0x70*/ '?', kKeyCode_F15, kKeyCode_Help, kKeyCode_Home, kKeyCode_PageUp, kKeyCode_Delete, kKeyCode_F4, kKeyCode_End,
/*0x78*/ kKeyCode_F2, kKeyCode_PageDown, kKeyCode_F1, kKeyCode_Left, kKeyCode_Right, kKeyCode_Down, kKeyCode_Up, '?'
};

@interface AppDelegate (Private)
-(void)appMain;
-(const char**)getArgs:(int&)argc;
-(void)freeArgs:(int)argc:(const char**)argv;
-(void)processEvents;
-(void)dispatchEvent:(NSEvent*)event;
-(void)handleKeyEvent:(NSEvent*)event;
-(void)handleModifierKeys:(NSEvent*)event;
-(void)postModifierKeys:(int)x type:(InputEvent::Type)type;
-(void)handleMouseButtons:(NSEvent*)event;
@end

@implementation AppDelegate

- (void)dealloc {
    [super dealloc];
}

- (void)launchAboutPage:(id)sender {
	[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"http://www.sunsidegames.com/crow"]];
};

- (void)launchSupportPage:(id)sender {
	[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"http://www.sunsidegames.com/crow"]];
};

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
	m_mButtons = 0;
	m_modifiers = 0;
	m_mPos.x = 0;
	m_mPos.y = 0;
	m_vKeys = s_vkeys_en;
	s_appd = self;
	[self appMain];
}

- (void)applicationWillTerminate:(NSNotification*)notification {
	
	App::Get()->exit = true;
	
	App::Get()->ResetDisplayDevice();
	App::Get()->Finalize();
	
	App::DestroyInstance();
	
	rt::Finalize();
}

- (void)showHelp:(id)sender {
	COut(C_Debug) << "showHelp" << std::endl;
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
	App *app = App::Get();
	
	switch (type) {
			
	case NSLeftMouseDown:
	case NSLeftMouseUp:
	case NSRightMouseDown:
	case NSRightMouseUp:
		break; // these also come through NSSystemDefined and give us more mouse buttons.
			
	case NSMouseMoved: 
	case NSLeftMouseDragged:
		case NSRightMouseDragged:
		{
			m_mPos = [event locationInWindow];
			if (window) {
				m_mPos = [[window contentView] convertPoint: m_mPos fromView: nil];
				NSRect r = [[window contentView] bounds];
				m_mPos.y = r.size.height - m_mPos.y; // why?
			}
			InputEvent e;
			e.type = InputEvent::T_MouseMove;
			e.data[0] = (int)m_mPos.x;
			e.data[1] = (int)m_mPos.y;
			e.data[2] = m_mButtons;
			e.time = xtime::ReadMilliseconds();
			app->PostInputEvent(e);
		} break;
			
	case NSKeyDown:
		if ([event modifierFlags] & NSCommandKeyMask)// ignore command key combinations, they won't get NSKeyUp messages.
			break;
		[self handleKeyEvent: event];
		break;
	case NSKeyUp:
		[self handleKeyEvent: event];
		break;
			
	case NSFlagsChanged:
		[self handleModifierKeys: event];
		break;
			
	case NSSystemDefined:
		[self handleMouseButtons: event];
		break;
			
	case NSScrollWheel:
		{
			NSPoint p = [event locationInWindow];
			InputEvent e;
			e.type = InputEvent::T_MouseWheel;
			e.data[0] = (int)p.x;
			e.data[1] = (int)p.y;
			e.data[2] = (int)[event deltaY]; // TODO: scaling?
			e.time = xtime::ReadMilliseconds();
			app->PostInputEvent(e);
		} break;
	default:
		[NSApp sendEvent: event];
		break;
	}
}

-(void)handleKeyEvent: (NSEvent*)event {
	bool keyDown = [event type] == NSKeyDown;
	//NSUInteger nsModifers = [event modifierFlags];
	unsigned short vkey = [event keyCode];
	
	InputEvent e;
	e.type = keyDown ? InputEvent::T_KeyDown : InputEvent::T_KeyUp;
	e.repeat = 0;
	e.data[0] = m_vKeys[vkey&0xff];
	e.time = xtime::ReadMilliseconds();
	App::Get()->PostInputEvent(e);
}

-(void)handleModifierKeys: (NSEvent*)event {
	int modifiers = (int)[event modifierFlags];
	int down = modifiers & ~m_modifiers;
	int up = m_modifiers & ~modifiers;
	
	if (down)
		[self postModifierKeys: down type: InputEvent::T_KeyDown];
	if (up)
		[self postModifierKeys: up type: InputEvent::T_KeyUp];
	
	m_modifiers = modifiers;
}

-(void)postModifierKeys: (int)keys type: (InputEvent::Type)type {
	int flags[6] = {
		NSAlternateKeyMask, kKeyCode_LAlt,
		NSControlKeyMask, kKeyCode_LCtrl,
		NSShiftKeyMask, kKeyCode_LShift
	};
	
	xtime::TimeVal millis = xtime::ReadMilliseconds();
	
	for (int i = 0; i < 6; i += 2) {
		if (keys & flags[i]) {
			InputEvent e;
			e.type = type;
			e.repeat = 0;
			e.data[0] = flags[i+1];
			e.time = millis;
			App::Get()->PostInputEvent(e);
		}
	}
}

-(void)handleMouseButtons: (NSEvent*)event {
	
	if ([event subtype] != 7)
		return;
	
	xtime::TimeVal millis = xtime::ReadMilliseconds();
	
	const int ButtonTypes[3] = {
		kMouseButton_Left,
		kMouseButton_Right,
		kMouseButton_Middle
	};
	
	int buttons = (int)[event data2];
	int changed = buttons ^ m_mButtons;
	
	for (int i = 0; i < 3; ++i) {
		int mask = (1<<i);
		
		if ((changed&mask)==0)
			continue;
		
		InputEvent e;
		e.type = (buttons&mask) ? InputEvent::T_MouseDown : InputEvent::T_MouseUp;
		e.data[0] = (int)m_mPos.x;
		e.data[1] = (int)m_mPos.y;
		e.data[2] = ButtonTypes[i];
		e.time = millis;
		App::Get()->PostInputEvent(e);
	}
	
	m_mButtons = buttons;
}

@end
