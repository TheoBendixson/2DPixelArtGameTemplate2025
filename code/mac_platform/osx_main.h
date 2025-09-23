// OSX Main.h
// Crafting Platformer Game Mac OS 
// by Ted Bendixson
//

#include <simd/simd.h>

// NOTE: (Ted)  The game and the level editor both use the same Mac platform layer 
//              If this starts to get unwieldy, don't hesitate to pull the plug
//              and accept the duplication from keeping the two separate.
#include "../../code/game_library/game_main.h"
#include "../../code/shared_apple_platform/CommonShaderTypes.h"
#include "../../code/shared_apple_platform/apple_sound.h"

const unsigned short LeftArrowKeyCode = 0x7B;
const unsigned short RightArrowKeyCode = 0x7C;
const unsigned short DownArrowKeyCode = 0x7D;
const unsigned short UpArrowKeyCode = 0x7E;

const unsigned short WKeyCode = 0x0D;
const unsigned short AKeyCode = 0x00;
const unsigned short SKeyCode = 0x01;
const unsigned short DKeyCode = 0x02;

const unsigned short FKeyCode = 0x03;
const unsigned short RKeyCode = 0x0F;
const unsigned short LKeyCode = 0x25;
const unsigned short KKeyCode = 0x28;
const unsigned short NKeyCode = 0x2D;
const unsigned short MKeyCode = 0x2E;
const unsigned short EKeyCode = 0x0E;
const unsigned short TKeyCode = 0x11;
const unsigned short ReturnKeyCode = 0x24;
const unsigned short EscapeKeyCode = 0x35;
const unsigned short F1KeyCode = 0x7A;
const unsigned short F2KeyCode = 0x78;
const unsigned short F3KeyCode = 0x63;
const unsigned short F4KeyCode = 0x76;
const unsigned short F5KeyCode = 0x60;
const unsigned short F6KeyCode = 0x61;
const unsigned short F7KeyCode = 0x62;
const unsigned short F8KeyCode = 0x64;
const unsigned short SpacebarKeyCode = 0x31;
const unsigned short DeleteKeyCode = 0x33;
const unsigned short ZKeyCode = 0x06;
const unsigned short YKeyCode = 0x10;

const unsigned short OneKeyCode = 0x12;
const unsigned short TwoKeyCode = 0x13;

struct mac_game_controller
{
    s32 LeftThumbXUsageID;
    s32 LeftThumbYUsageID;
    s32 ButtonAUsageID;
    s32 ButtonBUsageID;
    s32 ButtonXUsageID;
    s32 ButtonYUsageID;

    s32 ButtonLeftShoulder1UsageID;
    s32 ButtonRightShoulder1UsageID;

    s32 ButtonLeftShoulder2UsageID;
    s32 ButtonRightShoulder2UsageID;
    
    s32 ButtonStartUsageID;
    s32 ButtonSelectUsageID;

    // Values
    r32 LeftThumbstickX;
    r32 LeftThumbstickY;
    b32 UsesHatSwitch;

    s32 DPadX;
    s32 DPadY;

    b32 ButtonAState;
    b32 ButtonBState;
    b32 ButtonXState;
    b32 ButtonYState;

    b32 ButtonLeftShoulder1State;
    b32 ButtonRightShoulder1State;

    b32 ButtonLeftShoulder2State;
    b32 ButtonRightShoulder2State;

    b32 ButtonStartState;
    b32 ButtonSelectState;
};

struct mac_debug_time_marker 
{
    u32 OutputPlayCursor;
    u32 OutputWriteCursor;
    u32 OutputLocation;
    u32 OutputByteCount;
    u32 ExpectedFlipPlayCursor;
    u32 FlipWriteCursor;
    u32 FlipPlayCursor;
};

#define MAC_MAX_FILENAME_SIZE 4096

