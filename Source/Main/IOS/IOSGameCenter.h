/*! \file IOSGameCenter.h
 \copyright Copyright (c) 2010 Sunside Inc., All Rights Reserved.
 \copyright See Radiance/LICENSE for licensing terms.
 \author Joe Riedel
 */

#import <GameKit/GameKit.h>

@interface GameCenter : UIViewController<GKLeaderboardViewControllerDelegate, GKAchievementViewControllerDelegate> {
	UIViewController *gcViewController;
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
