/*! \file GameCenter.h
 \copyright Copyright (c) 2010 Sunside Inc., All Rights Reserved.
 \copyright See Radiance/LICENSE for licensing terms.
 \author Joe Riedel
 */

#include <Runtime/Base.h>
#if defined(RAD_OPT_OSX)
#import <AppKit/AppKit.h>
#endif
#import <GameKit/GameKit.h>


@interface GameCenter :
#if defined(RAD_OPT_IOS)
UIViewController<GKLeaderboardViewControllerDelegate, GKAchievementViewControllerDelegate> {
	UIViewController *gcViewController;
#else
	NSViewController<GKLeaderboardViewControllerDelegate, GKAchievementViewControllerDelegate> {
#endif
	BOOL authenticated;
	BOOL initialized;
}

+ (void)Create;
+ (void)Destroy;
+ (void)NotifyBackground:(BOOL)active;
+ (BOOL)IsAuthenticated;
+ (void)AuthenticationChanged;
+ (void)ShowLeaderboard:(const char*)name;
+ (void)ShowAchievements;
- (void)sendScore:(GKScore*)score;
- (void)saveScore:(GKScore*)score;
- (void)loadAndSendScores;
- (void)sendAchievement:(GKAchievement*)achievement;
- (void)saveAchievement:(GKAchievement*)achievement;
- (void)loadAndSendAchievements;
	
@end
