/*! \file IOSGameCenter.mm
 \copyright Copyright (c) 2010 Sunside Inc., All Rights Reserved.
 \copyright See Radiance/LICENSE for licensing terms.
 \author Joe Riedel
 */

#import "IOSGameCenter.h"
#import "IOSAppDelegate.h"
#import "FlurryAnalytics/FlurryAnalytics.h"
#include <Engine/Game/GameNetwork.h>

bool __IOS_IPad();

using namespace gn;

class GCNetwork : public GameNetwork
{
public:
	
	static GCNetwork *s_instance;
	
	GCNetwork(GameNetworkEventQueue *queue);
	
	virtual void AuthenticateLocalPlayer();	
	virtual void SendScore(const char *leaderboardId, int score);	
	virtual void ShowLeaderboard(const char *leaderboardId);
	virtual void SendAchievement(const char *achievementId, float percent);
	virtual void ShowAchievements();
	virtual void LogEvent(const char *eventName, const world::Keys *optionalKeys, bool timed);
	virtual void EndTimedEvent(const char *eventName, const world::Keys *optionalKeys);
	virtual void LogError(const char *error, const char *message);
	
protected:
	
	virtual RAD_DECLARE_GET(localPlayer, const LocalPlayer::Ref&);
	virtual RAD_DECLARE_GET(sessionReportOnAppClose, bool) { return m_reportOnClose; }
	virtual RAD_DECLARE_SET(sessionReportOnAppClose, bool) 
	{
		m_reportOnClose = value;
		[FlurryAnalytics setSessionReportsOnCloseEnabled: ((value) ? TRUE : FALSE)];
	}
	virtual RAD_DECLARE_GET(sessionReportOnAppPause, bool) { return m_reportOnPause; }
	virtual RAD_DECLARE_SET(sessionReportOnAppPause, bool) 
	{
		m_reportOnPause = value;
		[FlurryAnalytics setSessionReportsOnPauseEnabled: ((value) ? TRUE : FALSE)];
	}
	
private:
	
	bool m_reportOnClose;
	bool m_reportOnPause;
	LocalPlayer::Ref m_localPlayer;
};

// Trying my hand at putting the { on the opening line instead of it's own. It's kinda growing on me.

@interface GameCenter ()

@property (nonatomic, retain) UIViewController *gcViewController;

- (void) Close;
+ (void) Initialize;

@end

@implementation GameCenter

static GameCenter *s_gameCenter = 0;

@synthesize gcViewController;

- (id) init {

	if (self = [super init]) {
		self->authenticated = FALSE;
		self->initialized = FALSE;
	}
	
	return self;
}

+ (void) Create {
	if (!s_gameCenter)
		s_gameCenter = [[GameCenter alloc] init];
}

+ (void) Destroy {
	if (s_gameCenter) {
		[s_gameCenter Close];
		[s_gameCenter release];
	}
}

+ (void) NotifyBackground:(BOOL)active {
	
	
}

+ (BOOL) IsAuthenticated {
	if (s_gameCenter)
		return s_gameCenter->authenticated;
	return FALSE;
}

+ (void) Initialize {

	if (!s_gameCenter || s_gameCenter->initialized)
		return;
	
	s_gameCenter->initialized = TRUE;
	s_gameCenter->gcViewController = [[UIViewController alloc] init];
	
	NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
	[nc addObserver:self selector:@selector(AuthenticationChanged) name:GKPlayerAuthenticationDidChangeNotificationName object:nil];
	
	[s_gameCenter loadAndSendScores];
	[s_gameCenter loadAndSendAchievements];
}

+ (void) AuthenticationChanged {
	if (s_gameCenter)
		s_gameCenter->authenticated = [GKLocalPlayer localPlayer].isAuthenticated;
	
	if (GCNetwork::s_instance)
		GCNetwork::s_instance->OnAuthenticated.Trigger(gn::NR_Success);
}

+ (void) ShowLeaderboard:(const char*)name {
	if ([GameCenter IsAuthenticated] == FALSE)
		return;
	
	GKLeaderboardViewController *leaderboardController = [[[GKLeaderboardViewController alloc] init] autorelease];      
    if (leaderboardController != nil) {  
        leaderboardController.leaderboardDelegate = s_gameCenter;
		leaderboardController.category = [NSString stringWithUTF8String:name];
		
        UIWindow* window = [UIApplication sharedApplication].keyWindow;  
        [window addSubview: s_gameCenter->gcViewController.view];  
        [s_gameCenter->gcViewController presentModalViewController: leaderboardController animated: YES];
		
		if (GCNetwork::s_instance)
			GCNetwork::s_instance->OnShowLeaderboard.Trigger(true);
    } 
}

+ (void) ShowAchievements {
	if ([GameCenter IsAuthenticated] == FALSE)
		return;
	
	GKAchievementViewController *achievementsController = [[[GKAchievementViewController alloc] init] autorelease];      
    if (achievementsController != nil) {  
        achievementsController.achievementDelegate = s_gameCenter;
		
        UIWindow* window = [UIApplication sharedApplication].keyWindow;  
        [window addSubview: s_gameCenter->gcViewController.view];  
        [s_gameCenter->gcViewController presentModalViewController: achievementsController animated: YES];
		
		if (GCNetwork::s_instance)
			GCNetwork::s_instance->OnShowAchievements.Trigger(true);
    } 
}

- (void) Close {

	if (gcViewController)
		[gcViewController release];
	
}

- (void) leaderboardViewControllerDidFinish:(GKLeaderboardViewController *)viewController {
	[gcViewController dismissModalViewControllerAnimated:YES];
	
	if (__IOS_IPad()) {
		[gcViewController.view removeFromSuperview];
	}
	else {
		[viewController.view.superview removeFromSuperview];
	}
	
	if (GCNetwork::s_instance)
		GCNetwork::s_instance->OnShowLeaderboard.Trigger(false);
}

- (void)achievementViewControllerDidFinish:(GKAchievementViewController *)viewController {
	[gcViewController dismissModalViewControllerAnimated:YES];
	
	if (__IOS_IPad()) {
		[gcViewController.view removeFromSuperview];
	}
	else {
		[viewController.view.superview removeFromSuperview];
	}
	
	if (GCNetwork::s_instance)
		GCNetwork::s_instance->OnShowAchievements.Trigger(false);
}

static NSString *gcRetryFilePath() {  
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);  
    return [NSString stringWithFormat:@"%@/GCRetry.dat",[paths objectAtIndex:0]];  
}

- (void)sendScore:(GKScore *)score {  
	if (score) {  
        [score reportScoreWithCompletionHandler:^(NSError *error) {   
            if (error != nil){  
                // failed to send the score, save it off we'll retry later.
				COut(C_Debug) << "GameCenter: Failed to send score, Archiving..." << std::endl;
                [self saveScore: score];  
            }
			else {
				COut(C_Debug) << "GameCenter: Score sent!" << std::endl;
			}
        }];   
    }  
}

- (void)saveScore:(GKScore *)score {  
    NSString *savePath = gcRetryFilePath();  
	 
    NSMutableArray *scores = [[[NSMutableArray alloc] init] autorelease];  
    NSMutableDictionary *dict; 
	
    if ([[NSFileManager defaultManager] fileExistsAtPath:savePath]) { 
		// already have a file here load it.
        dict = [[[NSMutableDictionary alloc] initWithContentsOfFile:savePath] autorelease];  
		
        NSData *blob = [dict objectForKey: @"Scores"];  
        if (blob) {  
            NSKeyedUnarchiver *unarchiver = [[NSKeyedUnarchiver alloc] initForReadingWithData: blob];  
            scores = [unarchiver decodeObjectForKey: @"Scores"];  
            [unarchiver finishDecoding];  
            [unarchiver release];  
            [dict removeObjectForKey: @"Scores"]; // remove it so we can add it back again later  
        }  
    }
	else {  // no scores file, create a new one
		
        dict = [[[NSMutableDictionary alloc] init] autorelease]; 
		
    }  
	
    [scores addObject:score];  
	
	// Save file
    NSMutableData *blob = [NSMutableData data];   
    NSKeyedArchiver *archiver = [[NSKeyedArchiver alloc] initForWritingWithMutableData: blob];  
    [archiver encodeObject: scores forKey: @"Scores"];  
    [archiver finishEncoding]; 
    [dict setObject: blob forKey: @"Scores"];  
    [dict writeToFile: savePath atomically: YES];  
    [archiver release];
}  

- (void)loadAndSendScores {  
    NSString *savePath = gcRetryFilePath();  
	
    if (![[NSFileManager defaultManager] fileExistsAtPath: savePath]) {  
        return;  
    }  
	
    NSMutableDictionary *dict = [NSMutableDictionary dictionaryWithContentsOfFile: savePath];  
    NSData *blob = [dict objectForKey: @"Scores"];  
	
    if (!blob) {
        return; 
    }
	
	// Unarchive the score objects and send them again.
	// They may get written back to the file if it fails.
	
    NSKeyedUnarchiver *unarchiver = [[NSKeyedUnarchiver alloc] initForReadingWithData: blob];  
    NSArray *scores = [unarchiver decodeObjectForKey: @"Scores"];  
    [unarchiver finishDecoding];  
    [unarchiver release];  
	
    [dict removeObjectForKey: @"Scores"];  
    [dict writeToFile: savePath atomically: YES];  
	
    for (GKScore *score in scores) {  // resend scores
        [self sendScore:score];  
    }  
}

- (void)sendAchievement:(GKAchievement *)achievement {  
	if (achievement) {  
        [achievement reportAchievementWithCompletionHandler:^(NSError *error) {   
            if (error != nil){  
                // failed to send the score, save it off we'll retry later.
				COut(C_Debug) << "GameCenter: Failed to send achievement, Archiving..." << std::endl;
                [self saveAchievement: achievement];  
            }
			else {
				COut(C_Debug) << "GameCenter: Achievement sent!" << std::endl;
			}
        }];   
    }  
}

- (void)saveAchievement:(GKAchievement *)achievement {  
    NSString *savePath = gcRetryFilePath();  
	
    NSMutableArray *achievements = [[[NSMutableArray alloc] init] autorelease];  
    NSMutableDictionary *dict; 
	
    if ([[NSFileManager defaultManager] fileExistsAtPath:savePath]) { 
		// already have a file here load it.
        dict = [[[NSMutableDictionary alloc] initWithContentsOfFile:savePath] autorelease];  
		
        NSData *blob = [dict objectForKey: @"Achievements"];  
        if (blob) {  
            NSKeyedUnarchiver *unarchiver = [[NSKeyedUnarchiver alloc] initForReadingWithData: blob];  
            achievements = [unarchiver decodeObjectForKey: @"Achievements"];  
            [unarchiver finishDecoding];  
            [unarchiver release];  
            [dict removeObjectForKey: @"Achievements"]; // remove it so we can add it back again later  
        }  
    }
	else {  // no scores file, create a new one
		
        dict = [[[NSMutableDictionary alloc] init] autorelease]; 
		
    }  
	
    [achievements addObject: achievement];  
	
	// Save file
    NSMutableData *blob = [NSMutableData data];   
    NSKeyedArchiver *archiver = [[NSKeyedArchiver alloc] initForWritingWithMutableData: blob];  
    [archiver encodeObject: achievements forKey: @"Achievements"];  
    [archiver finishEncoding]; 
    [dict setObject: blob forKey: @"Achievements"];  
    [dict writeToFile: savePath atomically: YES];  
    [archiver release];
}  

- (void)loadAndSendAchievements {  
    NSString *savePath = gcRetryFilePath();  
	
    if (![[NSFileManager defaultManager] fileExistsAtPath: savePath]) {  
        return;  
    }  
	
    NSMutableDictionary *dict = [NSMutableDictionary dictionaryWithContentsOfFile: savePath];  
    NSData *blob = [dict objectForKey: @"Achievements"];  
	
    if (!blob) {
        return; 
    }
	
	// Unarchive the score objects and send them again.
	// They may get written back to the file if it fails.
	
    NSKeyedUnarchiver *unarchiver = [[NSKeyedUnarchiver alloc] initForReadingWithData: blob];  
    NSArray *achievements = [unarchiver decodeObjectForKey: @"Achievements"];  
    [unarchiver finishDecoding];  
    [unarchiver release];  
	
    [dict removeObjectForKey: @"Achievements"];  
    [dict writeToFile: savePath atomically: YES];  
	
    for (GKAchievement *achievement in achievements) {  // resend scores
        [self sendAchievement: achievement];  
    }  
}


@end

class GCLocalPlayer : public LocalPlayer
{
protected:
	virtual RAD_DECLARE_GET(authenticated, bool)
	{
		return [GameCenter IsAuthenticated] ? true : false;
	}
	
	virtual RAD_DECLARE_GET(id, const char *)
	{
		if ([GameCenter IsAuthenticated])
		{
			return [[[GKLocalPlayer localPlayer] playerID] UTF8String];
		}
		
		return "null";
	}
};

GCNetwork *GCNetwork::s_instance = 0;

GCNetwork::GCNetwork(GameNetworkEventQueue *queue) : m_reportOnClose(true), m_reportOnPause(true)
{
	RAD_ASSERT(!s_instance);
	
	if (queue)
		BindEventQueue(*queue);
	m_localPlayer.reset(new GCLocalPlayer());
	
	s_instance = this;
}
	
void GCNetwork::AuthenticateLocalPlayer()
{
	[GameCenter Initialize];
	[[GKLocalPlayer localPlayer] authenticateWithCompletionHandler:nil];
}

void GCNetwork::SendScore(const char *leaderboardId, int score)
{
	if (!s_gameCenter)
		return;
	
	NSString *category = [NSString stringWithUTF8String: leaderboardId];
	GKScore *gkscore = [[[GKScore alloc] initWithCategory: category] autorelease];
	if (gkscore) {
		gkscore.value = (int64_t)score;
		[s_gameCenter sendScore: gkscore];
	}
}

void GCNetwork::ShowLeaderboard(const char *leaderboardId)
{
	[GameCenter ShowLeaderboard: leaderboardId];
}

void GCNetwork::SendAchievement(const char *achievementId, float percent)
{
	if (!s_gameCenter)
		return;
	
	NSString *identifier = [NSString stringWithUTF8String: achievementId];
	GKAchievement *achievement = [[[GKAchievement alloc] initWithIdentifier: identifier] autorelease];
	if (achievement) {
		if (percent < 0.f)
			percent = 0.f;
		if (percent > 100.f)
			percent = 100.f;
		achievement.percentComplete = (double)percent;
		[s_gameCenter sendAchievement: achievement];
	}
}

void GCNetwork::ShowAchievements()
{
	[GameCenter ShowAchievements];
}

namespace {

NSDictionary *NSDictionaryFromKeys(const world::Keys &keys)
{
	NSMutableDictionary *dict = [NSMutableDictionary dictionaryWithCapacity: keys.pairs.size()];
	
	for (world::Keys::Pairs::const_iterator it = keys.pairs.begin(); it != keys.pairs.end(); ++it)
	{
		NSString *key = [NSString stringWithUTF8String: (it->first.c_str())];
		NSString *value = [NSString stringWithUTF8String: (it->second.c_str())];
		[dict setObject: value forKey: key];
	}
	
	return dict;
}

}

void GCNetwork::LogEvent(const char *eventName, const world::Keys *optionalKeys, bool timed)
{
	NSString *nsEventName = [NSString stringWithUTF8String: eventName];
	NSDictionary *dict = 0;
	
	if (optionalKeys)
		dict = NSDictionaryFromKeys(*optionalKeys);
	
	if (timed)
	{
		if (dict)
		{
			[FlurryAnalytics logEvent: nsEventName withParameters: dict timed: YES];
		}
		else
		{
			[FlurryAnalytics logEvent: nsEventName timed: YES];
		}
	}
	else
	{
		if (dict)
		{
			[FlurryAnalytics logEvent: nsEventName withParameters: dict];
		}
		else
		{
			[FlurryAnalytics logEvent: nsEventName];
		}
	}
}
	
void GCNetwork::EndTimedEvent(const char *eventName, const world::Keys *optionalKeys)
{
	NSString *nsEventName = [NSString stringWithUTF8String: eventName];
	NSDictionary *dict = nil;
	
	if (optionalKeys)
		dict = NSDictionaryFromKeys(*optionalKeys);
	
	[FlurryAnalytics endTimedEvent: nsEventName withParameters: dict];
}

void GCNetwork::LogError(const char *error, const char *message)
{
	NSString *nsError = [NSString stringWithUTF8String: error];
	NSString *nsMessage = [NSString stringWithUTF8String: message];
	
	[FlurryAnalytics logError: nsError message: nsMessage exception: nil];
}

const LocalPlayer::Ref &GCNetwork::RAD_IMPLEMENT_GET(localPlayer)
{
	return m_localPlayer;
}

GameNetwork::Ref GameNetwork::Create(GameNetworkEventQueue *queue)
{
	return GameNetwork::Ref(new GCNetwork(queue));
}