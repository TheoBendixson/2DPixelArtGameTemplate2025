
enum animation_cycle_direction
{
    AnimationCycleDirection_Forward,
    AnimationCycleDirection_Backward
};

struct animation_tracker
{
    s32 FrameCounter;
    s32 AnimationIndex;
    animation_cycle_direction CycleDirection;
};

enum lerp_animation_state
{
    LerpAnimationStateNotStarted,
    LerpAnimationStateAnimating
};

struct position_lerp_animation
{
    lerp_animation_state AnimationState;
    tile_map_position StartP;
    tile_map_position DestinationP;
    r32 LerpPercentage;
};

