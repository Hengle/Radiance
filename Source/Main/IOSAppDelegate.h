//
//  AppDelegate.h
//  OpenGLTest
//

#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <QuartzCore/CADisplayLink.h>
#import "IOSEAGLView.h"
#import "IOSRootViewController.h"

#include <Foundation/Foundation.h>
#include <Foundation/NSThread.h>
#include <Runtime/Runtime.h>
#include <Runtime/File.h>
#include <Engine/App.h>
#include <Engine/Engine.h>
#include <Engine/Sound/Sound.h>
#include <Engine/Game/Game.h>
#include <Engine/Renderer/IOS/RBackend.h>
#include <Engine/Renderer/GL/GLState.h>
#import <AudioToolbox/AudioToolbox.h>

#define USE_DEPTH_BUFFER 1
#define USE_DISPLAY_LINK
//#define GAMETHREAD
//#define FRAMELIMIT
#define FRAMERATE (1.f/30.f)

#if defined(GAMETHREAD)
#include <Runtime/Thread.h>
#include <Runtime/Thread/Locks.h>
class GameThread;
#endif

@interface IOSAppDelegate : UIResponder <UIAccelerometerDelegate, UIApplicationDelegate> {
@public
    UIWindow *window;
    EAGLView *glView;
	RootViewController *viewController;
	float totalTime;
	float totalFrames;
	float frameRate;
	bool initialized;
	bool inFrame;
	bool throttle;
	bool running;
	volatile bool frameEnabled;
	xtime::SecondsTimer<> *frameTimer;
#if defined(GAMETHREAD)
	GameThread *gameThread;
	id displayLink;
	volatile bool doHideSplash;
#elif defined(USE_DISPLAY_LINK)
	id displayLink;
#elif defined(FRAMELIMIT)
	int lastFrameTime;
#endif
	NSTimer *refreshTimer;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet EAGLView *glView;
@property (nonatomic, retain) RootViewController *viewController; 

- (void)startRefresh;
- (void)stopRefresh;
- (void)gameUpdate;
- (void)showSplash;
- (void)hideSplash;

#if defined(GAMETHREAD)
- (void)postFrame;
#endif

@end

#if defined(GAMETHREAD)
class GameThread : public thread::Thread
{
public:
	
	typedef boost::mutex Mutex;
	typedef boost::lock_guard<Mutex> Lock;
	typedef zone_list<InputEvent, ZEngineT>::type InputEventList;
	
	Mutex m;
	InputEventList events;
	
	
	GameThread(IOSAppDelegate *app);	
	~GameThread();	
	void Exit();
	void SwapBuffers();
	void PostFrame();
	
protected:
	
	virtual int ThreadProc();	
private:
	
	thread::Semaphore frame;
	IOSAppDelegate *m_app;
	volatile bool m_swap;
	volatile bool m_exit;
};
#endif

extern IOSAppDelegate *s_app;
