/*! \file AppDelegate.mm
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup main
*/

#import "AppDelegate.h"
#import <AudioToolbox/AudioToolbox.h>
#import "Flurry/Flurry.h"
#import "MainViewController.h"
#import "iRate.h"
#include <Runtime/Runtime.h>
#include <Engine/App.h>
#include <Engine/Engine.h>
#include <Engine/Sound/Sound.h>
#include <Engine/Game/Game.h>
#include <Engine/Renderer/GL/RBackend.h>
#include <Engine/Renderer/GL/GLState.h>
#include "../AppleGameCenter.h"
#import "UIDevice-Hardware.h"

#define TARGET_FRAME_RATE 30

void AudioSessionCallback(void *ud, UInt32 state);
void UncaughtExceptionHandler(NSException *exception);

AppDelegate *s_app;

@interface AppDelegate (Private)
-(const char**)getArgs:(int&)argc;
-(void)freeArgs:(int)argc args:(const char**)argv;
- (bool)initGame;
- (void)moviePlaybackFinished:(NSNotification *)notification;
- (void)alertView:(UIAlertView*)alertView didDismissWithButtonIndex:(NSInteger)buttonIndex;
@end

@implementation AppDelegate

- (void)dealloc {
	[self stopRefresh];
	App::Get()->Finalize();
	App::DestroyInstance();
	rt::Finalize();
	[_window release];
	[_viewController release];
    [super dealloc];
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {

	s_app = self;
	initialized = false;
	throttleFramerate = false;
	refreshTimer = nil;
	m_moviePlayer = nil;
	
	m_argv = [self getArgs: m_argc];
	
	rt::Initialize();
		
	COut(C_Info) << "NativeAppMain..." << std::endl;
	COut(C_Info) << "echo command line: ";
	
	for (int i = 0; i < m_argc; ++i) {
		COut(C_Info) << m_argv[i] << " ";
	}
	
	COut(C_Info) << std::endl;
	
	[GameCenter Create];

    self.window = [[[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]] autorelease];
    // Override point for customization after application launch.
	if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone) {
	    self.viewController = [[[MainViewController alloc] initWithNibName:@"ViewController_iPhone" bundle:nil] autorelease];
	} else {
	    self.viewController = [[[MainViewController alloc] initWithNibName:@"ViewController_iPad" bundle:nil] autorelease];
	}
	self.window.rootViewController = self.viewController;
    [self.window makeKeyAndVisible];
	
	m_moviePlayer = [[MPMoviePlayerController alloc] init];
	m_moviePlayer.view.frame = self.viewController.view.frame;
	[self.viewController.view addSubview: m_moviePlayer.view];
	m_moviePlayer.view.hidden = YES;
	m_moviePlayer.controlStyle = MPMovieControlStyleNone;
	m_moviePlayer.fullscreen = YES;
	m_moviePlayer.shouldAutoplay = NO;
	m_moviePlayer.scalingMode = MPMovieScalingModeFill;
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(moviePlaybackFinished:) name: MPMoviePlayerPlaybackDidFinishNotification object:m_moviePlayer];
	
	UInt32 category = kAudioSessionCategory_SoloAmbientSound;
	AudioSessionInitialize(NULL, NULL, AudioSessionCallback, NULL);
	AudioSessionSetProperty(kAudioSessionProperty_AudioCategory, sizeof(category), &category);
	
	NSSetUncaughtExceptionHandler(&UncaughtExceptionHandler);
	NSString *flurryKey = [NSString stringWithUTF8String: (App::Get()->flurryAPIKey.get())];
	[Flurry startSession:flurryKey];
	[Flurry setSessionReportsOnPauseEnabled:TRUE];
	
	[iRate sharedInstance].appStoreID = 575240929;
	[iRate sharedInstance].appStoreGenreID = iRateAppStoreGameGenreID;
	[iRate sharedInstance].daysUntilPrompt = 1;
	[iRate sharedInstance].usesUntilPrompt = 4;
	[iRate sharedInstance].remindPeriod = 3;
	
	//[iRate sharedInstance].previewMode = YES;
	
    return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application {
	// Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
	// Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
	
	[[UIApplication sharedApplication] setIdleTimerDisabled:NO];
	[self stopRefresh];
	App::Get()->NotifyBackground(true);
	[GameCenter NotifyBackground:TRUE];
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
	// Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later. 
	// If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
	// Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
	// Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
	
	[GameCenter NotifyBackground:FALSE];
	App::Get()->NotifyBackground(false);
	[[UIApplication sharedApplication] setIdleTimerDisabled:YES];
	if (initialized)
		[self startRefresh];
}

- (void)applicationWillTerminate:(UIApplication *)application {
	// Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
}

- (void)startRefresh {
	[self stopRefresh];
	/*if (throttleFramerate) {
		refreshTimer = [NSTimer scheduledTimerWithTimeInterval:(1.f / TARGET_FRAME_RATE) target:self selector:@selector(gameUpdate) userInfo:nil repeats:YES];
	} else*/ {
		refreshTimer = [NSTimer scheduledTimerWithTimeInterval:0 target:self selector:@selector(gameUpdate) userInfo:nil repeats:YES];
	}
}

- (void)stopRefresh {
	if (refreshTimer)
		[refreshTimer invalidate];
    refreshTimer = nil;
}

- (void)gameUpdate {
	[glView bindGL];
	
	if (!initialized) {
		if (![self initGame]) {
			[self stopRefresh];
			return;
		}
		initialized = true;
		[self hideSplash];
	}
	
	[glView bindFramebuffer];
	App::Get()->Tick();
}

- (void)showSplash {
	NSString *sz = 0;
	
	UIDeviceFamily deviceFamily = [[UIDevice currentDevice] deviceFamily];
	int deviceType = [[UIDevice currentDevice] platformType];
	
	bool iPhoneiPod = (deviceFamily == UIDeviceFamilyiPhone) || (deviceFamily == UIDeviceFamilyiPod);
	bool hd = false;
	
	if (iPhoneiPod) {
		hd = ((deviceFamily == UIDeviceFamilyiPhone) && (deviceType >= UIDevice4iPhone)) ||
			((deviceFamily == UIDeviceFamilyiPod) && (deviceType >= UIDevice4GiPod));
	} else {
		hd = (deviceFamily == UIDeviceFamilyiPad) && (deviceType >= UIDevice3GiPad);
	}
	
	if (iPhoneiPod && hd) {
		if ((deviceFamily == UIDeviceFamilyiPhone) && (deviceType >= UIDevice5iPhone)) {
			sz = @"Default-Landscape-568h@2x.png";
		} else {
			sz = @"Default-Landscape@2x.png";
		}
	} else if (!iPhoneiPod) {
		if (hd) {
			sz = @"Default-Landscape@2x~ipad.png";
		} else {
			sz = @"Default-Landscape~ipad.png";
		}
	} else {
		sz = @"Default-Landscape.png";
	}

	UIImageView *splashView = [[UIImageView alloc] initWithFrame:_window.frame];
	splashView.contentScaleFactor = [[UIScreen mainScreen] scale];
	splashView.image = [UIImage imageNamed:sz];
	
	UIViewController *splashViewController = [[MainViewController alloc] init];
	splashViewController.view = splashView;
	splashViewController.modalTransitionStyle = UIModalTransitionStyleCrossDissolve;
	
	[self.viewController presentModalViewController:splashViewController animated:NO];
}

- (void)hideSplash {
	[self.viewController dismissModalViewControllerAnimated:YES];
}

- (void)playFullscreenMovie:(const char*)path {
	[m_moviePlayer stop];
	NSMutableString *moviePath = [[NSMutableString alloc] initWithString:[[NSBundle mainBundle] bundlePath]];
	[moviePath appendString:@"/Base/"];
	NSString *nsPath = [[NSString alloc] initWithUTF8String:path];
	[moviePath appendString:nsPath];
	[nsPath release];
	NSURL *url = [NSURL fileURLWithPath:moviePath];
	m_moviePlayer.movieSourceType = MPMovieSourceTypeFile;
	m_moviePlayer.contentURL = url;
	m_moviePlayer.currentPlaybackTime = 0.0;
	m_moviePlayer.view.hidden = NO;
	[m_moviePlayer play];
}

- (void)alertViewPlainText:(const char*)title text:(const char*)text {
	UIAlertView *alert = [[UIAlertView alloc] initWithTitle:[NSString stringWithUTF8String:title] message:[NSString stringWithUTF8String:text] delegate:self cancelButtonTitle:@"Cancel" otherButtonTitles:@"Done",nil];
	
	[alert setAlertViewStyle:UIAlertViewStylePlainTextInput];
	
	UITextField *nameField = [alert textFieldAtIndex: 0];
	nameField.autocapitalizationType = UITextAutocapitalizationTypeNone;
	nameField.autocorrectionType = UITextAutocorrectionTypeNo;
	[alert show];
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

- (void)freeArgs: (int)argc args:(const char **)argv {
	for (int i = 0; i < argc; ++i)
		free((void*)argv[i]);
}

- (bool)initGame {

	App *app = App::Get(m_argc, m_argv);
	if (!(app->PreInit() && app->InitWindow()))
		return false;
		
	r::HRBackend rb = app->engine->sys->r.Cast<r::IRBackend>();
	
	if (!app->engine->sys->r->ctx.get()) {
		COut(C_Error) << "Rendering system was not initialized (Developer note your custom InitWindow method must set the rendering context before returning!" << std::endl;
		return false;
	}
	
	if (!rb->VidBind())
		return false;
		
	return app->Initialize() && app->Run();
}

- (void)moviePlaybackFinished:(NSNotification *)notification {
	App::Get()->MovieFinished();
	m_moviePlayer.view.hidden = YES;
}

- (void)alertView:(UIAlertView*)alertView didDismissWithButtonIndex:(NSInteger)buttonIndex {
	if (buttonIndex == 1) {
		UITextField *nameField = [alertView textFieldAtIndex: 0];
		String str = CStr([nameField.text cStringUsingEncoding:NSUTF8StringEncoding]);
		App::Get()->PlainTextDialogResult(false, str.c_str);
	} else {
		App::Get()->PlainTextDialogResult(true, "");
	}
}

@end

void AudioSessionCallback(void *ud, UInt32 state) {
	if (!s_app || !s_app->initialized)
		return;
	
	if (kAudioSessionBeginInterruption == state) {
		AudioSessionSetActive(NO);
		App::Get()->engine->sys->alDriver->Enable(false);
	} else if (kAudioSessionEndInterruption == state) {
		UInt32 category = kAudioSessionCategory_SoloAmbientSound;
		AudioSessionSetProperty(kAudioSessionProperty_AudioCategory, sizeof(category), &category);
		AudioSessionSetActive(YES);
		App::Get()->engine->sys->alDriver->Enable();
	}
}

void UncaughtExceptionHandler(NSException *exception) {
	[Flurry logError:@"Uncaught" message:@"Crash!" exception:exception];
}
