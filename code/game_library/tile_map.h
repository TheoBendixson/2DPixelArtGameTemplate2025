
struct tile_map_difference
{
    v2 dXY;
};

struct tile_map_position
{
    s32 AbsTileX;
    s32 AbsTileY;
    v2 Offset;
};

struct tile_map
{
    s32 CountX;
    s32 CountY;
    u32 *Tiles;
    r32 TileSideInMeters;
};

struct tile_offset
{
    s32 X;
    s32 Y;
};

u32 GetTileCount(tile_map *TileMap);

void RecanonicalizeCoord(tile_map *TileMap, u32 *Tile, r32 *TileRel);

tile_map_position RecanonicalizePosition(tile_map *TileMap, tile_map_position Pos);

tile_map_difference SubtractInReal32(tile_map *TileMap, tile_map_position *A, tile_map_position *B);

u32 GetTileValue(tile_map *TileMap, s32 Row, s32 Column);

u32 GetTileValue(tile_map *TileMap, tile_map_position *Position);

void SetTileValue(tile_map *TileMap, tile_map_position *Position, 
                  u32 NewTileID);

void SetTileValue(tile_map *TileMap, s32 Row, s32 Column, 
                  u32 NewTileID);

v2 GetLeftTopFromPosition(tile_map *TileMap, tile_map_position *Position,
                          r32 Width, r32 Height);
