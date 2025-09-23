
extern "C"
GAME_GET_STARTUP_CONFIG(GameGetStartupConfig)
{
    game_startup_config Config = {};
    Config.SoundSamplesPerSecond = 48000;
    u32 SoundBufferAudioSeconds = 2;
    Config.SoundOutputBufferArrayCount = Config.SoundSamplesPerSecond*SoundBufferAudioSeconds;
    Config.SoundBytesPerSample = sizeof(s16)*SoundBufferAudioSeconds;
    
    // NOTE: (Ted)  Two seconds of audio.
    Config.SoundBufferSize = Config.SoundSamplesPerSecond*Config.SoundBytesPerSample*2;
    return (Config);
}

void
InitializeRenderCommandsArray(render_commands_array *CommandsArray, memory_arena *Arena, u32 MaxCommands)
{
    CommandsArray->CommandCount = 0;
    CommandsArray->MaxCommands = MaxCommands;
    CommandsArray->Commands = PushArray(Arena, MaxCommands, game_texture_draw_command);
}

extern "C"
GAME_SETUP_RENDERER(GameSetupRenderer)
{
    memory_arena *DrawCommandsArena = &RenderCommands->DrawCommandsArena;
    InitializeArena(DrawCommandsArena, &Memory->RenderCommandsPartition);

    game_texture_draw_command_buffer *DrawCommandsBuffer = &RenderCommands->DrawCommandsBuffer;

    u32 LayerSizes[35];

    for (s32 Index = 0;
         Index < 35;
         Index += 1)
    {
        LayerSizes[Index] = 500;
    }

    LayerSizes[20] = 4000;
    LayerSizes[21] = 4000;
    LayerSizes[25] = 2000;

    LayerSizes[30] = 2000;

// NOTE: (Ted)  Particle Layer
    LayerSizes[33] = 2000;

    LayerSizes[34] = 500;

    for (u32 Index = 0;
         Index < BOTTOM_LAYER_COUNT;
         Index += 1)
    {
        render_commands_array *Layer = &DrawCommandsBuffer->BottomLayers[Index];
        InitializeRenderCommandsArray(Layer, DrawCommandsArena, LayerSizes[Index]);
    }
}

enum store_environment
{
    StoreEnvironmentNone,
    StoreEnvironmentSteam,
    StoreEnvironmentApple
};

#if LEVELEDITOR
void 
InitializeEntityPrototypeSubmenu(level_editor_entity_prototype_submenu *Submenu)
{
    Submenu->EntityPrototypeIndexCount = 0;
    Submenu->SelectedIndex = 0;

    for (s32 Index = 0;
         Index < SUBMENU_MAX;
         Index += 1)
    {
        Submenu->EntityPrototypeIndices[Index] = 0;
    }
}
#endif

extern "C"
GAME_INITIALIZE_MEMORY(GameInitializeMemory)
{
    game_state *GameState = (game_state*)Memory->PermanentStoragePartition.Start;

    srand((u32)(time(NULL)));

    GameState->DeviceSimulatorSettings = DeviceSimulatorSettings;
    GameState->SoundAndGraphicsLoaded = false;

// MARK: Initialize Entity System
    for (s32 EntityIndex = 0;
         EntityIndex < MAX_ENTITIES;
         EntityIndex++)
    {
        GameState->Entities[EntityIndex] = {};
        GameState->EntitiesFreelist[EntityIndex] = EntityIndex;
    }

    CreateEntity(GameState);

    memory_arena *WorldArena = &GameState->WorldArena;

    InitializeArena(WorldArena, Memory->PermanentStoragePartition.Size - sizeof(game_state),
                    (u8*)Memory->PermanentStoragePartition.Start + sizeof(game_state));

    InitializeArena(&GameState->LongtermStorageArena, Memory->LongtermArenaPartition.Size,
                    (u8*)Memory->LongtermArenaPartition.Start);

    memory_arena *LongtermStorageArena = &GameState->LongtermStorageArena;

// MARK: Strings
    s32 MaxCharacters = 200;
    GameState->Framerate = InitializeString(LongtermStorageArena, MaxCharacters, "framerate");

// MARK: Level Editor
#if LEVELEDITOR
    GameState->InLevelEditorMode = false;
    GameState->SelectedEntityPrototypeIndex = 0;
    GameState->DebugDrawingEnabled = false;
#endif

    s32 AudioSettingsOptionCount = 0;
    audio_settings_option *AudioSettingsOptions = GameState->AudioSettingsOptions;
    AudioSettingsOptions[AudioSettingsOptionCount++] = AudioSettingsOptionMusicVolume;
    AudioSettingsOptions[AudioSettingsOptionCount++] = AudioSettingsOptionSFXVolume;
    AudioSettingsOptions[AudioSettingsOptionCount++] = AudioSettingsOptionBack;
    GameState->AudioSettingsOptionCount = AudioSettingsOptionCount;
    GameState->SelectedAudioSettingsOptionIndex = 0;
    GameState->VolumeChanged = false;

    r32 MusicVolume = 1.0f;
    r32 SFXVolume = 1.0f;

    GameState->AudioState.MusicVolume = MusicVolume;
    GameState->AudioState.SFXVolume = SFXVolume;

// MARK: Achievements
    store_environment StoreEnvironment = StoreEnvironmentNone;

#if STEAMSTORE
    // TODO: (Ted)  Once this gets to steam, put the app id here.
    //GameState->SteamAppID = 2287140;
    StoreEnvironment = StoreEnvironmentSteam;
#elif MACAPPSTORE || IOS
    StoreEnvironment = StoreEnvironmentApple;
#endif

    /*
    u32 AchievementIndex = 0;
    GameState->AchievementCount = AchievementIndex;

// LOOKINTO: (Ted)  Consider offloading this to another thread as well.
#if WINDOWS || IOS
    read_file_result AchievementFileReadResult = Memory->PlatformReadEntireFile("achievements.msf");
#elif MACOS
    read_file_result AchievementFileReadResult = Memory->PlatformReadFileFromApplicationSupport("achievements.msf");
#endif

    if (AchievementFileReadResult.ContentsSize > 0)
    {
        game_achievement_save_file *SaveFile = (game_achievement_save_file *)AchievementFileReadResult.Contents;

        for (u32 Index = 0;
             Index < ACHIEVEMENT_COUNT;
             Index++)
        {
            game_achievement_serialized SerializedAchievement = SaveFile->Achievements[Index];
            achievement *Achievement = &GameState->Achievements[Index];
            Achievement->Achieved = SerializedAchievement.Achieved;
            Achievement->AchievedOnCloud = SerializedAchievement.AchievedOnCloud;
        }
    }

#if WINDOWS
    PlatformFreeMemory(AchievementFileReadResult.Contents);
#elif MACOS || IOS
    Memory->PlatformFreeFileMemory(AchievementFileReadResult.Contents);
#endif
    */

    Memory->IsInitialized = true;
}

void
AddEntityPrototypeIndexToSubmenu(level_editor_entity_prototype_submenu *Submenu, s32 PrototypeEntityIndex)
{
    s32 IndexCount = Submenu->EntityPrototypeIndexCount;
    Submenu->EntityPrototypeIndices[IndexCount++] = PrototypeEntityIndex;
    Submenu->EntityPrototypeIndexCount = IndexCount;
}


extern "C"
GAME_POST_CONTENT_LOAD_SETUP(GamePostContentLoadSetup)
{
    game_state *GameState = (game_state*)Memory->PermanentStoragePartition.Start;

    s32 TileSideInPixels = 48;
    r32 TilesHigh = 26;
    TileSideInPixels = s32(RenderCommands->ViewportHeight/TilesHigh);

    s32 TilesWide = s32(RenderCommands->ViewportWidth/TileSideInPixels);
    r32 SpriteScaleFactor = (r32)(TileSideInPixels/24.0);
    r32 MetersToPixels = (r32)TileSideInPixels / 1.0f; 

    screen Screen = {};
    Screen.MetersToPixels = MetersToPixels;
    Screen.SpriteScaleFactor = SpriteScaleFactor;
    GameState->Screen = Screen;

    GameState->RenderTilesHigh = 26; 

#if MACOS
    GameState->RenderTilesHigh = 20; 
#endif

// MARK: Text loading
    u32 *Alphabet = TextureMap->Alphabet;

// MARK: Entity Prototype / Editor Setup
    entity *EntityPrototypes = GameState->EntityPrototypes;

    for (s32 Index = 0;
         Index < 20;
         Index += 1)
    {
        entity Empty = {};
        EntityPrototypes[Index] = Empty;
    }

    s32 PrototypeEntityIndex = 1;
    GameState->EntityPrototypeCount = PrototypeEntityIndex;

    GameState->SoundAndGraphicsLoaded = true;
}
