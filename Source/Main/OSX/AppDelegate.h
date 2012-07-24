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
}
@end

extern AppDelegate *s_appd;
