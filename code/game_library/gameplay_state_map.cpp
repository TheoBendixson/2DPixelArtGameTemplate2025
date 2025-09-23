
gameplay_state
GetTileValue(gameplay_state_map *GameplayStateMap, tile_map_position *Pos)
{
    Assert(Pos->AbsTileX < GameplayStateMap->CountX &&
           Pos->AbsTileY < GameplayStateMap->CountY);
    gameplay_state State = GameplayStateMap->Tiles[Pos->AbsTileY*GameplayStateMap->CountX + Pos->AbsTileX];
    return State;
}

void
SetTileValue(gameplay_state_map *GameplayStateMap, tile_map_position *Pos, gameplay_state NewState)
{
    Assert(Pos->AbsTileX < GameplayStateMap->CountX &&
           Pos->AbsTileY < GameplayStateMap->CountY);
    GameplayStateMap->Tiles[Pos->AbsTileY*GameplayStateMap->CountX + Pos->AbsTileX] = NewState;
}

gameplay_state_map *
InitializeGameplayStateMap(memory_arena *Arena, s32 CountX, s32 CountY)
{
    gameplay_state_map *Result = PushStruct(Arena, gameplay_state_map); 
    Result->CountX = CountX;
    Result->CountY = CountY;

    s32 TileCount = (CountX*CountY);
    Result->Tiles = PushArray(Arena, TileCount, gameplay_state);

    for (s32 Index = 0;
         Index < TileCount;
         Index += 1)
    {
        gameplay_state EmptyState = {};
        EmptyState.EntityID = 0;
        Result->Tiles[Index] = EmptyState;
    }

    return Result;
}

b32
PositionHasEntityOfType(gameplay_state_map *GameplayStateMap, entity *Entities, tile_map_position *SearchP, entity_type Type)
{
    gameplay_state State = GetTileValue(GameplayStateMap, SearchP);

    if (State.EntityID > 0)
    {
        entity *Entity = TryToGetEntity(Entities, State.EntityID);

        if (Entity->Type == Type)
        {
            return true;
        }
    }

    return false;
}

