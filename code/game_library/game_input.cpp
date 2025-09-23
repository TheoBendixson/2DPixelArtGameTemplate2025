
// Returns true whether the given key just went down, or it repeated (i.e. started registering
// again periodically because it's been held down for enough time).
b32 
PressedRepeatableKey(game_state *GameState, repeatable_input_key Key)
{
    b32 Result = (Key && Key == GameState->LastPressedRepeatableKey &&
                  GameState->RepeatableKeyTimer == 0);
    return Result;
}

tile_map_position 
MousePosToTilePos(screen Screen, v2 MousePos, tile_map *TileMap, 
                  b32 TrimOffset = true)
{
    v2 diffInPixels = MousePos - Screen.Center;
    v2 diffInMeters = diffInPixels/Screen.MetersToPixels;
    tile_map_position Result = Screen.CameraP;
    
    Result.Offset.X += diffInMeters.X;
    Result.Offset.Y -= diffInMeters.Y;

    Result = RecanonicalizePosition(TileMap, Result);

    if (TrimOffset)
    {
        Result.Offset = V2(0,0);
    }

    return Result;
}
