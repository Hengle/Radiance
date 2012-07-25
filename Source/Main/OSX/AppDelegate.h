/*! \file AppDelegate.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup Main
 */

#import <Cocoa/Cocoa.h>
#import <AppKit/AppKit.h>

@interface AppDelegate : NSObject <NSApplicationDelegate> {
@public
	NSWindow *window;
@private
	int m_mButtons; // tracked through event loop
	int m_modifiers;
	NSPoint m_mPos;
	const int *m_vKeys;
}
@end

extern AppDelegate *s_appd;
