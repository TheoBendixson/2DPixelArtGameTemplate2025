void
UpdateAnimationTracker(animation_tracker *AnimationTracker, u8 FrameUpdateFrequency,
                       u8 MaxAnimationFrames)
{
    AnimationTracker->FrameCounter++;

    if (AnimationTracker->FrameCounter >= FrameUpdateFrequency)
    {
        AnimationTracker->FrameCounter = 0;
        AnimationTracker->AnimationIndex++;

        if (AnimationTracker->AnimationIndex >= MaxAnimationFrames)
        {
            AnimationTracker->AnimationIndex = 0;
        }
    }
}

void
UpdateAnimationTrackerCycling(animation_tracker *AnimationTracker, u8 FrameUpdateFrequency,
                              u8 MaxAnimationFrames)
{
    AnimationTracker->FrameCounter += 1;

    if (AnimationTracker->FrameCounter >= FrameUpdateFrequency)
    {
        AnimationTracker->FrameCounter = 0;

        if (AnimationTracker->CycleDirection == AnimationCycleDirection_Forward)
        {
            AnimationTracker->AnimationIndex += 1;

            if (AnimationTracker->AnimationIndex >= MaxAnimationFrames)
            {
                AnimationTracker->CycleDirection = AnimationCycleDirection_Backward;
                AnimationTracker->AnimationIndex -= 2;
            }
        } else if (AnimationTracker->CycleDirection == AnimationCycleDirection_Backward)
        {
            AnimationTracker->AnimationIndex -= 1;

            if (AnimationTracker->AnimationIndex < 0)
            {
                AnimationTracker->CycleDirection = AnimationCycleDirection_Forward;
                AnimationTracker->AnimationIndex = 1;
            }
        }
    }
}

b32
UpdateAnimationTrackerAndQueryComplete(animation_tracker *AnimationTracker, s32 FrameUpdateFrequency,
                                       s32 MaxAnimationFrames, animation_cycle_direction AnimationDirection)
{
    AnimationTracker->FrameCounter++;

    if (AnimationTracker->FrameCounter >= FrameUpdateFrequency)
    {
        AnimationTracker->FrameCounter = 0;

        if (AnimationDirection == AnimationCycleDirection_Forward)
        {
            AnimationTracker->AnimationIndex++;

            if (AnimationTracker->AnimationIndex >= MaxAnimationFrames)
            {
                return true;
            }

        } else if (AnimationDirection == AnimationCycleDirection_Backward)
        {
            AnimationTracker->AnimationIndex--;

            if (AnimationTracker->AnimationIndex == 0)
            {
                return true;
            }
        }
    }

    return false;
}

void
ResetAnimationTracker(animation_tracker *AnimationTracker)
{
    AnimationTracker->FrameCounter = 0;
    AnimationTracker->AnimationIndex = 0;
}

tile_map_position
GetInterpolatedPosition(tile_map *TileMap, position_lerp_animation Animation)
{
    // LERP
    // r32 interpolated = (start_value + (end_value - start_value) *pct);
    //
    // Start value is zero offset from present P
    //
    // Diff is the tile map difference between the start and end position
    //
    tile_map_difference Diff = SubtractInReal32(TileMap, &Animation.StartP, 
                                                &Animation.DestinationP);
  
    r32 Percentage = Animation.LerpPercentage;

    v2 InterpolatedOffset = {};
    InterpolatedOffset.X = Diff.dXY.X*Percentage;
    InterpolatedOffset.Y = Diff.dXY.Y*Percentage;

    tile_map_position InterpolatedPos = Animation.StartP;
    InterpolatedPos.Offset -= InterpolatedOffset;
    InterpolatedPos = RecanonicalizePosition(TileMap, InterpolatedPos);

    return InterpolatedPos;
}

