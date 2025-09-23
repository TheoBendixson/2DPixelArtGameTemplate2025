
struct achievement
{
    b32 Achieved;
    b32 AchievedOnCloud;
    const char *CloudIdentifier;
};

#define ACHIEVEMENT_COUNT   7

struct achievement_unlock_state
{
    b32 AchievementTriggered;
    const char *AchievementCloudIdentifier;
    u32 AchievementIndex;

    b32 ShouldStoreAchievements;
};

struct game_achievement_serialized
{
    b32 Achieved;
    b32 AchievedOnCloud;
};

struct game_achievement_save_file
{
    game_achievement_serialized Achievements[ACHIEVEMENT_COUNT];
};

#define PLATFORM_UNLOCK_ACHIEVEMENT(name) void name(achievement_unlock_state AchievementUnlockState, achievement *Achievements)
typedef PLATFORM_UNLOCK_ACHIEVEMENT(platform_unlock_achievement);
