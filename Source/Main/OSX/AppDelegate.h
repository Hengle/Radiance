/*! \file AppDelegate.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup Main
 */

#import <Cocoa/Cocoa.h>
#import <AppKit/AppKit.h>

@interface BorderlessKeyWindow : NSWindow {}
- (id)initWithContentRect:(NSRect)contentRect styleMask:(NSUInteger)windowStyle backing:(NSBackingStoreType)bufferingType defer:(BOOL)deferCreation;
- (BOOL) canBecomeKeyWindow;
- (BOOL) canBecomeMainWindow;
- (void) resignKeyWindow;
- (void) becomeKeyWindow;
@end

@interface AppDelegate : NSObject <NSApplicationDelegate> {
@public
	BorderlessKeyWindow *window;
	bool fullscreen;
	bool reclaimWindowLevel;
@private
	int m_mButtons; // tracked through event loop
	int m_modifiers;
	NSPoint m_mPos;
	const int *m_vKeys;
}
-(int)checkWindowLevel:(NSInteger)level;
-(void)notifyGameCenterDialogDone;
@end

extern AppDelegate *s_appd;
