
struct SteamCallbackThingy
{
    game_state *GameState;

    private:
        STEAM_CALLBACK(SteamCallbackThingy, OnUserStatsReceived, UserStatsReceived_t);
        STEAM_CALLBACK(SteamCallbackThingy, OnAchievementStored, UserAchievementStored_t);
};

void
SteamCallbackThingy::OnUserStatsReceived(UserStatsReceived_t *pCallback)
{

#if WINDOWS
        WindowsDebugPrintf("Steam on User Stats Received Callback \n");
#elif MACOS
        NSLog(@"Steam on User Stats Received Callback\n");
#endif

    if (GameState->SteamAppID == pCallback->m_nGameID &&
        k_EResultOK == pCallback->m_eResult)
    {
#if WINDOWS
        WindowsDebugPrintf("Steam on User Stats Received Callback Successful \n");
#elif MACOS
        NSLog(@"Steam on User Stats Received Callback Successful \n");
#endif
        GameState->SteamIsInitialized = true;

        b32 UpdatedAchievementStats = false;

        for (u32 AchievementIndex = 0;
             AchievementIndex < GameState->AchievementCount;
             AchievementIndex++)
        {
            achievement Achievement = GameState->Achievements[AchievementIndex];
            bool AchievedOnSteamSevers = false;
            SteamUserStats()->GetAchievement(Achievement.CloudIdentifier, &AchievedOnSteamSevers);

            if (AchievedOnSteamSevers)
            {
                Achievement.Achieved = true;
                Achievement.AchievedOnCloud = true;
            } else if (Achievement.Achieved &&
                       !Achievement.AchievedOnCloud)
            {
                SteamUserStats()->SetAchievement(Achievement.CloudIdentifier);
                UpdatedAchievementStats = true;
            }

            GameState->Achievements[AchievementIndex] = Achievement;
        }

        if (UpdatedAchievementStats)
        {
            SteamUserStats()->StoreStats();
        }
    }
}

void
SteamCallbackThingy::OnAchievementStored(UserAchievementStored_t *pCallback)
{
    if (GameState->SteamAppID == pCallback->m_nGameID)
    {
        const char *AchievementName = pCallback->m_rgchAchievementName;

        for (u32 Index = 0;
             Index < GameState->AchievementCount;
             Index++)
        {
            achievement *Achievement = &GameState->Achievements[Index];

            if (Achievement->Achieved &&
                !Achievement->AchievedOnCloud)
            {
                b32 NameMatches = true;

                char *SteamName = (char *)AchievementName;
                char *OurName = (char *)Achievement->CloudIdentifier;

                while (*SteamName != '\0' &&
                       *OurName != '\0')
                {
                    if (*SteamName != *OurName)
                    {
                        NameMatches = false;
                    }

                    SteamName += 1;
                    OurName += 1;
                }

                if (NameMatches)
                {
                    Achievement->AchievedOnCloud = true;
                }
            }
        }

        // LOOKINTO: (Ted)  Consider syncing this with the file system.
        //                  It might not matter anyway because we get all of these at the start of the game
#if WINDOWS
        WindowsDebugPrintf("Steam Achievement Unlocked \n");
#elif MACOS
        NSLog(@"Steam Achievement Unlocked \n");
#endif
    }
}

PLATFORM_UNLOCK_ACHIEVEMENT(PlatformUnlockAchievement)
{
    if (AchievementUnlockState.AchievementTriggered)
    {
        SteamUserStats()->SetAchievement(AchievementUnlockState.AchievementCloudIdentifier);
    }
   
    if (AchievementUnlockState.ShouldStoreAchievements)
    {
        SteamUserStats()->StoreStats();
    }
}

struct steam_action_handles
{
    b32 SteamInputInitialized;
    InputDigitalActionHandle_t Up;
    InputDigitalActionHandle_t Down;
    InputDigitalActionHandle_t MoveLeft;
    InputDigitalActionHandle_t MoveRight;
    InputDigitalActionHandle_t Cancel;
    InputDigitalActionHandle_t Craft;
    InputDigitalActionHandle_t Escape;
    InputDigitalActionHandle_t Jump;
    InputDigitalActionHandle_t SelectFromWorld;
    InputDigitalActionHandle_t ChangeSelectionMode;
};

void
InitializeSteamAndRequestUserStats(steam_action_handles *SteamActionHandles)
{
    b32 SteamInitialized = SteamAPI_Init();

    b32 CanRequestSteamUserStats = true;

    if (!SteamInitialized ||
        SteamUserStats() == NULL ||
        SteamUser() == NULL ||
        !SteamUser()->BLoggedOn())
    {
        CanRequestSteamUserStats = false;
    }

    if (CanRequestSteamUserStats)
    {
        SteamUserStats()->RequestCurrentStats();
    }

    if (SteamInitialized)
    {
        SteamActionHandles->SteamInputInitialized = SteamInput()->Init(0);
        SteamActionHandles->Up = SteamInput()->GetDigitalActionHandle("up");
        SteamActionHandles->Down = SteamInput()->GetDigitalActionHandle("down");
        SteamActionHandles->MoveLeft = SteamInput()->GetDigitalActionHandle("move_left");
        SteamActionHandles->MoveRight = SteamInput()->GetDigitalActionHandle("move_right");
        SteamActionHandles->Cancel = SteamInput()->GetDigitalActionHandle("cancel");
        SteamActionHandles->Craft = SteamInput()->GetDigitalActionHandle("craft");
        SteamActionHandles->Escape = SteamInput()->GetDigitalActionHandle("escape");
        SteamActionHandles->Jump = SteamInput()->GetDigitalActionHandle("jump");
        SteamActionHandles->SelectFromWorld = SteamInput()->GetDigitalActionHandle("select_from_world");
        SteamActionHandles->ChangeSelectionMode = SteamInput()->GetDigitalActionHandle("change_selection_mode");
    }
}

void
RunSteamInput(game_controller_input *Controller, steam_action_handles *SteamActionHandles)
{
    InputHandle_t InputHandles[STEAM_INPUT_MAX_COUNT] ;
    SteamInput()->GetConnectedControllers(InputHandles);

    for (u32 InputIndex = 0;
         InputIndex < STEAM_INPUT_MAX_COUNT;
         InputIndex++)
    {
        InputHandle_t InputHandle = InputHandles[InputIndex];

        if (InputHandle != 0)
        {
            InputDigitalActionData_t MoveRightActionData = 
                SteamInput()->GetDigitalActionData(InputHandle, SteamActionHandles->MoveRight); 
            InputDigitalActionData_t MoveLeftActionData = 
                SteamInput()->GetDigitalActionData(InputHandle, SteamActionHandles->MoveLeft);
            InputDigitalActionData_t UpActionData = 
                SteamInput()->GetDigitalActionData(InputHandle, SteamActionHandles->Up);
            InputDigitalActionData_t DownActionData = 
                SteamInput()->GetDigitalActionData(InputHandle, SteamActionHandles->Down);
            InputDigitalActionData_t CancelActionData = 
                SteamInput()->GetDigitalActionData(InputHandle, SteamActionHandles->Cancel);
            InputDigitalActionData_t CraftActionData = 
                SteamInput()->GetDigitalActionData(InputHandle, SteamActionHandles->Craft);
            InputDigitalActionData_t EscapeActionData = 
                SteamInput()->GetDigitalActionData(InputHandle, SteamActionHandles->Escape);
            InputDigitalActionData_t JumpData = 
                SteamInput()->GetDigitalActionData(InputHandle, SteamActionHandles->Jump);
            InputDigitalActionData_t SelectFromWorldData = 
                SteamInput()->GetDigitalActionData(InputHandle, SteamActionHandles->SelectFromWorld);
            InputDigitalActionData_t ChangeSelectionModeData = 
                SteamInput()->GetDigitalActionData(InputHandle, SteamActionHandles->ChangeSelectionMode);

            ProcessButton(&Controller->Right, MoveRightActionData.bState);
            ProcessButton(&Controller->Left, MoveLeftActionData.bState);
            ProcessButton(&Controller->Up, UpActionData.bState);
            ProcessButton(&Controller->Down, DownActionData.bState);
            ProcessButton(&Controller->B, CancelActionData.bState);
            ProcessButton(&Controller->Y, CraftActionData.bState);
            ProcessButton(&Controller->X, SelectFromWorldData.bState);
            ProcessButton(&Controller->Select, ChangeSelectionModeData.bState);
            ProcessButton(&Controller->A, JumpData.bState);
        }
    }
}
