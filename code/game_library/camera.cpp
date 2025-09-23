
v2 CameraAdjustedLeftTop(screen *Screen, tile_map_difference *DiffToCameraPoint,
                         r32 Width, r32 Height)
{
    r32 MetersToPixels = Screen->MetersToPixels;
    r32 GroundPointX = Screen->Center.X + MetersToPixels*DiffToCameraPoint->dXY.X;
    r32 GroundPointY = Screen->Center.Y - MetersToPixels*DiffToCameraPoint->dXY.Y;

    v2 LeftTop = { GroundPointX - 0.5f*MetersToPixels*Width, 
                   GroundPointY - 0.5f*MetersToPixels*Height };

    return LeftTop;
}

v2 
CamAdjustedLeftTopInPixelsOneTileEntity(tile_map *TileMap, tile_map_position *Position, 
                                        screen *Screen, r32 MetersToPixels)
{
    v2 PositionInScreenPixels = GetEntityPositionInScreenPixels(TileMap, Position, Screen);
    v2 LeftTop = { PositionInScreenPixels.X - 0.5f*MetersToPixels*TileMap->TileSideInMeters, 
                   PositionInScreenPixels.Y - 0.5f*MetersToPixels*TileMap->TileSideInMeters };
    return LeftTop;
}

