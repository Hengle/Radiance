/*! \file AppDelegate.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup main
*/

#import <UIKit/UIKit.h>
#import "EAGLView.h"

@class MainViewController;

@interface AppDelegate : UIResponder <UIApplicationDelegate> {
@public
EAGLView *glView;
bool initialized;
bool throttleFramerate;
NSTimer *refreshTimer;
@private
const char **m_argv;
int m_argc;
}

@property (strong, nonatomic) UIWindow *window;
@property (strong, nonatomic) MainViewController *viewController;

- (void)startRefresh;
- (void)stopRefresh;
- (void)gameUpdate;
- (void)showSplash;
- (void)hideSplash;

@end


extern AppDelegate *s_app;