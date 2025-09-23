// game_main.h
//
// Pixel Art Game Template
//
// 2025 Ted Bendixson
// 

#if MACOS
#include <simd/simd.h>
#include <string.h>
#include <float.h>
#endif

#if WINDOWS
#include "math.h"
#endif

#define Minimum(A, B) ((A < B) ? (A) : (B));
#define Maximum(A, B) ((A > B) ? (A) : (B));

#define TILE_DRAW_BUFFER_INDEX		0

#include "../core_libraries/base_types.h"
#include "game_math.h"
#include "../core_libraries/string.h"
#include "../core_libraries/file_io.h"

#define PLATFORM_QUIT_GAME(name) void name()
typedef PLATFORM_QUIT_GAME(platform_quit_game);

#include "platform_logging.h"

#include "game_achievement.h"
#include "game_memory.h"
#include "game_texture_map.h"
#include "game_texture.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>
void DebugPrintf(game_memory *Memory, char *Format, ...){
#ifndef NDEBUG
	va_list Args;
	va_start(Args, Format);

	char Str[1024] = "";

#if WINDOWS
	vsprintf_s(Str, Format, Args);
# elif MACOS
	vsnprintf(Str, 1024, Format, Args);
#endif

	Str[ArrayCount(Str) - 1] = 0; //Safety null-termination

	Memory->PlatformLogMessage(Str);

	va_end(Args);
#endif
}

#include "tile_map.h"

#include "gameplay_state_map.h"

#include "animation.h"
#include "game_entity.h"
#include "game_input.h"
#include "memory_arena.h"

string *
InitializeString(memory_arena *Arena, s32 CharacterMax, char *Characters)
{
    string *Result = PushStruct(Arena, string);
    Result->CharacterMax = CharacterMax;
    Result->Characters = PushArray(Arena, CharacterMax, char);
    LoadString(Result, Characters);
    return Result;
}

#include "game_renderer.h"
#include "game_sound.h"
#include "game_runtime.h"

#include "camera.h"

struct game_settings_file_header
{
    f32 MusicVolume;
    f32 SFXVolume;
};

#include "device_simulator.h"
#include "game_startup.h"
#include "spritesheet.h"

#define MAX_ENTITIES   1000

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_JPEG
#define STBI_NO_BMP
#define STBI_NO_TGA
#define STBI_NO_PSD
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_GIF
#define STBI_NO_PNM
#include "../core_libraries/stb_image.h"

#define REPEATABLE_KEY_COUNT   24 

enum repeatable_input_key
{
    RepeatableKey_None,
    RepeatableKey_Left,
    RepeatableKey_Right,
    RepeatableKey_Up,
    RepeatableKey_Down,
    RepeatableKey_Enter,
    RepeatableKey_Q,
    RepeatableKey_E,
    RepeatableKey_R,
    RepeatableKey_F1,
    RepeatableKey_F2,
    RepeatableKey_F3,
    RepeatableKey_F5,
    RepeatableKey_F6,
    RepeatableKey_F7,
    RepeatableKey_F8,
    RepeatableKey_F9,
    RepeatableKey_F13,
    RepeatableKey_F14,
    RepeatableKey_F15,
    RepeatableKey_Escape,
    RepeatableKey_F11,
    RepeatableKey_MouseLeftClick,
    RepeatableKey_MouseRightClick,
};

enum audio_settings_option
{
    AudioSettingsOptionMusicVolume,
    AudioSettingsOptionSFXVolume,
    AudioSettingsOptionBack
};

#define ENTITY_PROTOTYPE_MAX    40
#define SUBMENU_MAX    20

struct level_editor_entity_prototype_submenu
{
    s32 EntityPrototypeIndices[SUBMENU_MAX];
    s32 EntityPrototypeIndexCount;
    s32 SelectedIndex;
};

struct game_state
{
    memory_arena ScratchArena;
    memory_arena SoundsArena;
    memory_arena WorldArena;
    memory_arena LongtermStorageArena;

    device_simulator_settings DeviceSimulatorSettings;
 
// MARK: Loading Phases
    b32 SoundAndGraphicsLoaded;

// MARK: Main Menu
    audio_settings_option AudioSettingsOptions[3];
    v2 AudioSettingsOptionLocations[3];
    s32 SelectedAudioSettingsOptionIndex;
    s32 NextSelectedAudioSettingsOptionIndex;
    s32 AudioSettingsOptionCount;
    b32 VolumeChanged;

    r32 dMusicVolume;
    r32 dSFXVolume;

// MARK: Audio
    game_audio_state AudioState;
    u64 BackgroundMusicInstanceId; // NOTE:  This pointer should always be valid.

// MARK: Entity System
    entity Entities[MAX_ENTITIES];
    u32 EntitiesFreelist[MAX_ENTITIES];
    s64 EntityFreelistCursor;
    u32 EntityGenerations[MAX_ENTITIES];

    gameplay_state_map *GameplayStateMap;

    screen Screen;
    s32 RenderTilesHigh;

// MARK: Text
    string *Framerate;

// MARK: Debug Visualization Modes
    string *NullStringTitle;

    b32 ShowFramerate;

// MARK: In-game Input
    repeatable_input_key LastPressedRepeatableKey; // NOTE:   0=Left 1=Right 2=Up 3=Down 4=Undo
    s32 RepeatableKeyTimer; // NOTE:   Every time this is 0 the LastPressedRepeatableKey is counted as pressed.
    b32 RepeatableKeyIsRepeating;

// MARK: Achievements
#if STEAMSTORE
    b32 SteamIsInitialized;
    s64 SteamAppID;
#elif MACAPPSTORE || IOS
    b32 GameCenterInitialized;
#endif

    achievement Achievements[ACHIEVEMENT_COUNT];
    u32 AchievementCount;

    entity EntityPrototypes[ENTITY_PROTOTYPE_MAX];
    s32 EntityPrototypeCount;


#if LEVELEDITOR
    b32 InLevelEditorMode;
    s32 SelectedEntityPrototypeIndex;
    b32 DebugDrawingEnabled;
#endif
    
};
