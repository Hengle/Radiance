//
//  AppDelegate.m
//

#import "IOSAppDelegate.h"
#import "IOSRootViewController.h"
#import "IOSGameCenter.h"
#import "FlurryAnalytics/FlurryAnalytics.h"
#include <stdio.h>	

IOSAppDelegate *s_app=0;

namespace {
	
bool s_postQuit = false;

bool Initialize(App &app)
{
	if (!app.PreInit())
		return false;
	
	r::gls.Bind(r::gls.New(true));
	r::HRBackend rb = app.engine->sys->r.Cast<r::IRBackend>();
	RAD_VERIFY(rb);
	rb->VidBind();
	
	if (!app.Initialize())
		return false;
	
	app.Run();
	return true;
}

void Finalize(App &app)
{
	app.Finalize();
}
	
}

void __PostQuit()
{
	s_postQuit = true;
}

bool __PostQuitPending()
{
	return s_postQuit;
}

void __IOS_ScreenSize(int &w, int &h);

void __IOS_BundlePath(char *dst)
{
	NSString *bPath = [[NSBundle mainBundle] bundlePath];
	strcpy(dst, [bPath cStringUsingEncoding:NSASCIIStringEncoding]);
}

bool __IOS_IPhone()
{
	static bool x = UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone;
	return x;
}

extern int s_screenWidth;
extern int s_screenHeight;

bool __IOS_IPhone4()
{
	static bool x = (__IOS_IPhone()) && ((s_screenWidth==960&&s_screenHeight==640)||(s_screenWidth==640&&s_screenHeight==960));
	return x;
}

bool __IOS_IPad()
{
	static bool x = UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad;
	return x;
}

bool __IOS_IPad3()
{
	static bool x = (__IOS_IPad()) && (s_screenWidth>1024);
	return x;
}

// defined in App implementation.
bool __IOSAPP_AllowHD();

float __IOS_ContentScale()
{
	static float s = 0.f;
	
	if (s < 1.f)
	{
		if (__IOSAPP_AllowHD())
		{
			s = [[UIScreen mainScreen] scale];
			NSLog(@"__IOS_ContentScale(): %f", s);
		}
		else
		{
			s = 1.f;
		}
	}
	
	return s;
}

#if defined(GAMETHREAD)
GameThread::GameThread(IOSAppDelegate *app) : m_app(app), m_exit(false), m_swap(false)
{
}

GameThread::~GameThread()
{
	Exit();
	frame.Put();
	Join();
}

void GameThread::Exit()
{ 
	m_exit = true; 
}

int GameThread::ThreadProc()
{
	NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
	
//	[NSThread setThreadPriority:1.0];
#if defined(FRAMELIMIT)
	int lastFrameTime = xtime::ReadMilliseconds();
#endif
	
	while (!m_exit)
	{
#if defined(FRAMELIMIT)
		int frameTime = xtime::ReadMilliseconds();
		if ((frameTime-lastFrameTime) < 1000/31)
			continue;			
		lastFrameTime = frameTime;
#endif
		
		InputEventList x;
		
		{
			Lock L(m);
			x.swap(events);
		}
		
		for(InputEventList::const_iterator it = x.begin(); it != x.end(); ++it)
		{
			App::Get()->game->PostInputEvent(*it);
		}
		
		[m_app gameUpdate];
		int z = frame.Get(thread::Infinite, true);
		if (m_exit)
			break;
		if (z < 2) // get one more.
		{
			frame.Get(thread::Infinite, false);
			if (m_exit)
				break;
		}
		if (m_swap)
			[m_app->glView presentScene];
		m_swap = false;
	}
	
	[pool release];
	
	return 0;
}

void GameThread::SwapBuffers()
{
	m_swap = true;
}

void GameThread::PostFrame()
{
	frame.Put();
}
	
#endif

void __IOS_SwapBuffers()
{
#if defined(UPDATETHREAD)
	if (s_app->gameThread)
		s_app->gameThread->SwapBuffers();
#else
	[s_app->glView presentScene];
#endif
}

void __IOS_ProcessEvents()
{
	NSRunLoop *currentRunLoop = [NSRunLoop currentRunLoop];
    NSDate *destDate = [[NSDate alloc] init];
	// Create an autorelease pool
	//	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
	// Calculate the new date
	NSDate *newDate = [[NSDate alloc] initWithTimeInterval:1.0f/60 sinceDate:destDate];
	[destDate release];
	
	// Run the runloop to process OS events
	[currentRunLoop runUntilDate:newDate];
	
	[newDate release];
	
	// Drain the pool
	//	[pool drain];
}

void __IOS_Throttle(bool throttle)
{
#if !defined(GAMETHREAD)
	s_app->throttle = throttle;
	if (s_app->running)
		[s_app startRefresh];
#endif
}

void __IOS_BindFramebuffer()
{
	[s_app->glView bindFramebuffer];
}

void __IOS_SoundSessionCallback(void *ud, UInt32 state)
{
	if (!s_app || !s_app->initialized)
		return;
	
	if (kAudioSessionBeginInterruption == state)
	{
		AudioSessionSetActive(NO);
		App::Get()->engine->sys->soundDevice->Unbind();
	}
	else if (kAudioSessionEndInterruption == state)
	{
		UInt32 category = kAudioSessionCategory_SoloAmbientSound;
		AudioSessionSetProperty(kAudioSessionProperty_AudioCategory, sizeof(category), &category);
		AudioSessionSetActive(YES);
		App::Get()->engine->sys->soundDevice->Bind();
	}
}

FILE *__IOS_OpenPersistence(const char *name, const char *mode)
{
	static char s_basePath[1024] = {0};
	
	if (s_basePath[0] == 0)
	{
		NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
		NSString *path = [[paths objectAtIndex:0] retain];
		strcpy(s_basePath, [path cStringUsingEncoding:NSASCIIStringEncoding]);
		[path release];
	}
	
	String x(s_basePath);
	x += '/';
	x += name;
	
	return fopen(x.c_str(), mode);
}

// A class extension to declare private methods
@interface IOSAppDelegate ()

#if defined(USE_DISPLAY_LINK)
@property (nonatomic, assign) id displayLink;
#endif
@property (nonatomic, assign) NSTimer *refreshTimer;


@end

////////////////////////////////////////////////////////////////////////////////

@implementation IOSAppDelegate

@synthesize window;
@synthesize glView;
@synthesize viewController;

#if defined(USE_DISPLAY_LINK)
@synthesize displayLink;
#endif
@synthesize refreshTimer;

void uncaughtExceptionHandler(NSException *exception) 
{
	[FlurryAnalytics logError:@"Uncaught" message:@"Crash!" exception:exception];
}

- (void)applicationDidFinishLaunching:(UIApplication *)application 
{
#if defined(GAMETHREAD)
	[NSThread setThreadPriority:1.0];
#endif
	s_app = self;
	initialized = false;
	
	rt::Initialize();
	RAD_DEBUG_ONLY(file::EnforcePortablePaths(false));
	frameTimer = new xtime::SecondsTimer<>();
	totalTime = 0.f;
	totalFrames = 0.f;
	frameRate = 0.f;
	frameEnabled = false;
	inFrame = false;
	running = false;
#if defined(GAMETHREAD)
	gameThread = 0;
	throttle = true;
	displayLink = nil;
	doHideSplash = false;
#elif defined(USE_DISPLAY_LINK)
	displayLink = nil;
	throttle = true;
#elif defined(FRAMELIMIT)
	lastFrameTime = 0;
	throttle = false;
#else
	throttle = false;
#endif
	
	refreshTimer = nil;
	
	CGRect b = [[UIScreen mainScreen] bounds];
	COut(C_Debug) << "Window(" << b.origin.x << ", " << b.origin.y << "," << b.size.width << "," << b.size.height << ")" << std::endl;
	
	self.window = [[UIWindow alloc] initWithFrame:b];
    self.viewController = [[RootViewController alloc] initWithNibName:@"MainWindow" bundle:nil];
	self.window.rootViewController = self.viewController;
    [self.window makeKeyAndVisible];
	
	UInt32 category = kAudioSessionCategory_SoloAmbientSound;
	AudioSessionInitialize(NULL, NULL, __IOS_SoundSessionCallback, NULL);
	AudioSessionSetProperty(kAudioSessionProperty_AudioCategory, sizeof(category), &category);
	
	//	[[UIAccelerometer sharedAccelerometer] setUpdateInterval:interval];
	//	[[UIAccelerometer sharedAccelerometer] setDelegate:self];
	
	[GameCenter Create];
	
	NSSetUncaughtExceptionHandler(&uncaughtExceptionHandler);
	NSString *flurryKey = [NSString stringWithUTF8String: (App::Get()->flurryAPIKey.get())];
	[FlurryAnalytics startSession:flurryKey];
	[FlurryAnalytics setSessionReportsOnPauseEnabled:TRUE];
}

- (void)applicationWillResignActive:(UIApplication *)application 
{
	[[UIApplication sharedApplication] setIdleTimerDisabled:NO];
	frameEnabled = false;
	[self stopRefresh];
	App::Get()->NotifyBackground(true);
	[GameCenter NotifyBackground:TRUE];
}

- (void)applicationDidBecomeActive:(UIApplication *)application 
{
	[GameCenter NotifyBackground:FALSE];
	App::Get()->NotifyBackground(false);
	[[UIApplication sharedApplication] setIdleTimerDisabled:YES];
	frameEnabled = true;
	if (initialized)
		[self startRefresh];
}

- (void)applicationWillTerminate:(UIApplication *)application
{
	[GameCenter Destroy];
	[self dealloc];
}

- (void)gameUpdate
{	// NOTE! Called from GameThread, be careful.
	if (!frameEnabled || inFrame)
		return;
	
#if defined(FRAMELIMIT)
	if (lastFrameTime == 0)
		lastFrameTime = xtime::ReadMicroseconds();
	
	int frameTime = xtime::ReadMicroseconds();
	if ((frameTime-lastFrameTime) < (int)(xtime::Constants<float>::MicrosPerSecond() * FRAMERATE * 0.5f))
		return;
	
	lastFrameTime = frameTime;
#endif
	
	inFrame = true;
	
	[glView bindGL];
	
	bool firstFrame = !initialized;
	
	if (!initialized)
	{
		Initialize(*App::Get());
		initialized = true;
	}
	
	float dt = 1.0f/60.0f;
	
	if (frameTimer->IsTiming())
	{
		frameTimer->Stop();
		dt = frameTimer->Elapsed();
		totalTime += dt;
		totalFrames += 1.0f;
		if (totalTime >= 5.0f)
		{
			frameRate = totalFrames / totalTime;
			totalTime = 0.0f;
			totalFrames = 0.0f;
			COut(C_Debug) << "FPS: " << frameRate << std::endl;
			App::DumpMemStats(C_Debug);
		}
	}
	
	frameTimer->Start();
	
	[glView bindFramebuffer];
	
	App::Get()->Tick();
	
	if (firstFrame)
		[self hideSplash];
	
	inFrame = false;
}

#if defined(GAMETHREAD)
- (void)postFrame
{
	if (doHideSplash)
	{
		[self.viewController dismissModalViewControllerAnimated:YES];
		doHideSplash = false;
	}
	
	if (gameThread)
		gameThread->PostFrame();
}
#endif

- (void)startRefresh
{
	[self stopRefresh];
	
	if (throttle)
	{
#if defined(GAMETHREAD)
		gameThread = new GameThread(self);
		gameThread->Run();
		displayLink = [NSClassFromString(@"CADisplayLink") displayLinkWithTarget:self selector:@selector(postFrame)];
		[displayLink setFrameInterval:1];
		[displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
#elif defined(USE_DISPLAY_LINK)
		displayLink = [NSClassFromString(@"CADisplayLink") displayLinkWithTarget:self selector:@selector(gameUpdate)];
		[displayLink setFrameInterval:2];
		[displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
#else
		refreshTimer = [NSTimer scheduledTimerWithTimeInterval:FRAMERATE target:self selector:@selector(gameUpdate) userInfo:nil repeats:YES];
#endif
	}
	else
	{
		refreshTimer = [NSTimer scheduledTimerWithTimeInterval:0 target:self selector:@selector(gameUpdate) userInfo:nil repeats:YES];
	}
	
	running = true;
}


- (void)stopRefresh 
{
	running = false;
	
	if (refreshTimer)
		[refreshTimer invalidate];
    refreshTimer = nil;
	
#if defined(GAMETHREAD)
	[displayLink invalidate];
	displayLink = nil;
	if (gameThread)
		delete gameThread;
	gameThread = 0;
#elif defined(USE_DISPLAY_LINK)
	[displayLink invalidate];
	displayLink = nil;
#endif
}

- (void)showSplash
{
	NSString *sz = 0;
	
	if (__IOS_IPhone4())
		sz = @"Default@2x-Landscape.png";
	else if (__IOS_IPad())
		sz = @"Default-Landscape~ipad.png";
	else
		sz = @"Default-Landscape.png";

	UIImageView *splashView = [[UIImageView alloc] initWithFrame:window.frame];
	splashView.contentScaleFactor = __IOS_ContentScale();
	splashView.image = [UIImage imageNamed:sz];
	
	UIViewController *splashViewController = [[RootViewController alloc] init];
	splashViewController.view = splashView;
	splashViewController.modalTransitionStyle = UIModalTransitionStyleCrossDissolve;
	
	[self.viewController presentModalViewController:splashViewController animated:NO];
	
}
	
- (void)hideSplash
{
#if defined(GAMETHREAD)
	doHideSplash = true;
#else
	[self.viewController dismissModalViewControllerAnimated:YES];
#endif
}

- (void)accelerometer:(UIAccelerometer*)accelerometer didAccelerate:(UIAcceleration*)acceleration
{
	//	app->SetAccelerometer(Vec3(acceleration.x, acceleration.y, acceleration.z));
}

- (void)dealloc 
{
	[self stopRefresh];
	Finalize(*App::Get());
	App::DestroyInstance();
	delete frameTimer;
	rt::Finalize();
	[window release];
	[glView release];
	[viewController release];
	[super dealloc];
}

bool CloudStorage::Enabled()
{
	return false;
}

CloudFile::Vec CloudStorage::Resolve(const char *name)
{
	return CloudFile::Vec();
}

void CloudStorage::ResolveConflict(const CloudFile::Ref &version)
{
}

bool CloudStorage::StartDownloadingLatestVersion(const char *name)
{
	return true;
}

CloudFile::Status CloudStorage::FileStatus(const char *name)
{
	return CloudFile::Ready;
}

@end
