
struct button_state 
{
    int TransitionCount;
    b32 EndedDown;
    
    // NOTE: (Pere) Platform layer should guarantee that when EndedDown changes relative to
    //              the previous frame, TransitionCount should be odd.
};

b32 
ButtonWentDown(button_state *Button)
{
    b32 Result = (Button->EndedDown && (Button->TransitionCount % 2));
    return Result;
}

b32 
ButtonWentUp(button_state *Button)
{
    b32 Result = (!Button->EndedDown && (Button->TransitionCount % 2));
    return Result;
}

struct game_controller_input 
{
    union 
    {
        button_state Buttons[14];
        struct 
        {
            button_state Up;
            button_state Down;
            button_state Left;
            button_state Right;
            button_state A; // (Space)
            button_state B; // 
            button_state X;
            button_state Y;
            button_state LeftShoulder1; // (F4)
            button_state LeftShoulder2; // (Z / F3)
            button_state RightShoulder1; // (E)
            button_state RightShoulder2; // (F2)
            button_state Start; // (Enter)
            button_state Select; // (R)
        };
    };

    v2 RightThumb;
};

struct keyboard_input
{
    union
    {
	    button_state Buttons[26 + 10 + 20 + 13];
        struct
        {
            button_state Letters[26];
            button_state Numbers[10];
            button_state F[20]; // NOTE: (Pere) [0] is unused for convenience.

            button_state Up;
            button_state Down;
            button_state Left;
            button_state Right;

            button_state Space;
            button_state Enter;
            button_state Delete;
            button_state Backspace;
            button_state Tab;
            button_state Shift;
            button_state Alt;
            button_state Control;
            button_state Escape;
        };
    };
};

enum game_input_mode
{
    GameInputMode_KeyboardMouse,
    GameInputMode_GameController
};

struct game_input
{
    game_input_mode PrimaryInputMode;
    keyboard_input Keyboard;
    game_controller_input Controller;
    r32 dtForFrame;
    r32 FrameRateMultiplier;

    // NOTE: (Pere)  Platform layer should ignore mouse presses when window is out of focus.
    button_state MouseButtons[5]; // NOTE: (Pere)  0=left 1=middle 2=right (3=prev 4=next)
    v2 MousePos; // NOTE: (Pere)  Positive Y Down.
    v2 WindowDim;
    s32 MouseWheel;

    b32 HasFocus;
    b32 HasFocusPrev;

#if LEVELEDITOR
    b32 SaveLevelThisFrame;
    b32 OpenLevelThisFrame;
#endif
};

inline
void ProcessButton(button_state *Button, b32 IsDown)
{
    if(!Button->EndedDown != !IsDown)
    {
        Button->EndedDown = IsDown;
        Button->TransitionCount++;
    }
}


