
global_variable s32 AchievementIndices[2];

PLATFORM_UNLOCK_ACHIEVEMENT(PlatformUnlockAchievement)
{
    NSMutableArray *achievements = [[NSMutableArray alloc] init];
    AchievementIndices[0] = -1;
    AchievementIndices[1] = -1;

    if (AchievementUnlockState.AchievementTriggered)
    {
        NSString *achievementID = [NSString stringWithCString: AchievementUnlockState.AchievementCloudIdentifier 
                                                     encoding: NSUTF8StringEncoding];
        GKAchievement *achievement = [[GKAchievement alloc] initWithIdentifier: achievementID];
        achievement.percentComplete = 100.0;
        [achievements addObject: achievement];
        AchievementIndices[0] = AchievementUnlockState.AchievementIndex;
    }

    if (AchievementUnlockState.ExtraDopeAchievementTriggered)
    {
        NSString *achievementID = [NSString stringWithCString: AchievementUnlockState.ExtraDopeCloudIdentifier 
                                                     encoding: NSUTF8StringEncoding];
        GKAchievement *achievement = [[GKAchievement alloc] initWithIdentifier: achievementID];
        achievement.percentComplete = 100.0;
        [achievements addObject: achievement];
        AchievementIndices[1] = AchievementUnlockState.ExtraDopeAchievementIndex;
    }
   
    if (AchievementUnlockState.ShouldStoreAchievements)
    {
        [GKAchievement reportAchievements: achievements 
                    withCompletionHandler: ^(NSError * _Nullable error)
        {
            if (error == nil)
            {
                for (u32 Index = 0; Index < 2; Index++)
                {
                    s32 AchievementIndex = AchievementIndices[Index];

                    if (AchievementIndex >= 0)
                    {
                        achievement *Achievement = &Achievements[AchievementIndex];
                        Achievement->AchievedOnCloud = true;
                    }
                }
            }
        }];
    }
}

void
LoadGameCenterAchievementsFromTheCloud(game_state *GameState)
{
    [GKAchievement loadAchievementsWithCompletionHandler: ^(NSArray<GKAchievement *> *achievements, 
                                                            NSError *achievementLoadError)
    {
        if (achievementLoadError == nil)
        {
            for (GKAchievement *GameCenterAchivement in achievements)
            {
                NSLog(@"Achievement with ID:%@ achieved", GameCenterAchivement.identifier);

                for (u32 Index = 0; Index < GameState->AchievementCount; Index++)
                {
                    achievement *LocalAchievement = &GameState->Achievements[Index];
                    NSString *LocalCloudID = [NSString stringWithCString: LocalAchievement->CloudIdentifier
                                                                encoding: NSUTF8StringEncoding];

                    if ([GameCenterAchivement.identifier isEqualToString: LocalCloudID] &&
                        GameCenterAchivement.completed)
                    {
                        LocalAchievement->Achieved = true;
                        LocalAchievement->AchievedOnCloud = true;
                    }
                }
            }
        }
    }];
}
