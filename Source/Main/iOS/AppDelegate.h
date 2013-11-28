/*! \file AppDelegate.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup main
*/

#import <UIKit/UIKit.h>
#import "EAGLView.h"
#import <MediaPlayer/MPMoviePlayerController.h>

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
MPMoviePlayerController *m_moviePlayer;
}

@property (strong, nonatomic) UIWindow *window;
@property (strong, nonatomic) MainViewController *viewController;

- (void)startRefresh;
- (void)stopRefresh;
- (void)gameUpdate;
- (void)showSplash;
- (void)hideSplash;
- (void)playFullscreenMovie:(const char*)path;
- (void)alertViewPlainText:(const char*)title text:(const char*)text;

@end


extern AppDelegate *s_app;