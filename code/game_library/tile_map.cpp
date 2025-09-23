
tile_map_position
InitAbsTileXAbsTileY(s32 AbsTileX, s32 AbsTileY)
{
    tile_map_position Pos = {};
    Pos.Offset = V2(0.0f, 0.0f);
    Pos.AbsTileX = AbsTileX;
    Pos.AbsTileY = AbsTileY;
    return Pos;
}

u32 GetTileCount(tile_map *TileMap)
{
    return TileMap->CountX*TileMap->CountY;
}

inline void
RecanonicalizeCoord(tile_map *TileMap, s32 *Tile, r32 *TileRel)
{
    // LOOKINTO (Ted): Need to do something that doesn't use the divide/multiply method for recanonicalizing because this can 
    //                 end up rounding back onto the tile you just came from.

    // LOOKINTO (Ted): See if this case ever gets triggered by testing if the offset is greater than one tile but the tile didn't 
    //                 change.  It may explain some of the movement issues the game's been having. 

    s32 Offset = RoundReal32ToInt32(*TileRel / TileMap->TileSideInMeters);
    *Tile += Offset;
    *TileRel -= Offset*TileMap->TileSideInMeters;

    // LOOKINTO: (Ted)    Fix floating pos math so this can be <
    Assert(*TileRel > -0.5001f*TileMap->TileSideInMeters);
    Assert(*TileRel < 0.5001f*TileMap->TileSideInMeters);
}

tile_map_position 
RecanonicalizePosition(tile_map *TileMap, tile_map_position Pos)
{
    tile_map_position Result = Pos;
    RecanonicalizeCoord(TileMap, &Result.AbsTileX, &Result.Offset.X);
    RecanonicalizeCoord(TileMap, &Result.AbsTileY, &Result.Offset.Y);
    return (Result);
}

tile_map_difference 
SubtractInReal32(tile_map *TileMap, tile_map_position *A, tile_map_position *B)
{
    tile_map_difference Result;

    v2 dTileXY = {(r32)A->AbsTileX - (r32)B->AbsTileX,
                  (r32)A->AbsTileY - (r32)B->AbsTileY};
    Result.dXY = TileMap->TileSideInMeters*dTileXY + (A->Offset - B->Offset);

    return (Result);
}

u32 GetTileValue(tile_map *TileMap, s32 Row, s32 Column)
{
    if (Row < 0 || Column < 0)
    {
        return 0;
    }


    u32 TileID = TileMap->Tiles[Row*TileMap->CountX + Column];
    return TileID;
}

u32 GetTileValue(tile_map *TileMap, tile_map_position *Position)
{
    u32 Result = GetTileValue(TileMap, Position->AbsTileY, Position->AbsTileX);
    return (Result);
}

void
SetTileValue(tile_map *TileMap, tile_map_position *Position, 
             u32 NewTileID)
{
    TileMap->Tiles[Position->AbsTileY*TileMap->CountX + Position->AbsTileX] = NewTileID;
} 

void SetTileValue(tile_map *TileMap, s32 Row, s32 Column, 
                  u32 NewTileID)
{
    Assert(Row < TileMap->CountY);
    Assert(Column < TileMap->CountX);
    TileMap->Tiles[Row*TileMap->CountX + Column] = NewTileID;
}

b32 
PositionIsOnSameTile(tile_map_position P1, tile_map_position P2)
{
    if (P1.AbsTileX == P2.AbsTileX &&
        P1.AbsTileY == P2.AbsTileY)
    {
        return true;
    }

    return false;
}

b32
PositionIsApproximatelyEqual(tile_map *TileMap, tile_map_position *P1, tile_map_position *P2)
{
    tile_map_difference Diff = SubtractInReal32(TileMap, P1, P2);

    if (Abs(Diff.dXY.X) < 0.01f &&
        Abs(Diff.dXY.Y) < 0.01f)
    {
        return true;
    }

    return false;
}

v2 GetLeftTopFromPosition(tile_map *TileMap, tile_map_position *Position,
                          r32 Width, r32 Height)
{
    r32 LeftTopX = Position->AbsTileX*TileMap->TileSideInMeters + Position->Offset.X - 0.5f*Width;
    r32 LeftTopY = Position->AbsTileY*TileMap->TileSideInMeters + Position->Offset.Y + 0.5f*Height;
    return V2(LeftTopX, LeftTopY);
}

