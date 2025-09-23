
u64 EncodeEntityID(u32 Index, u32 Generation) 
{
    return (((u64)Index << 32) | ((u64)Generation));
}

void 
DecodeEntityID(u64 ID, u32 *OutIndex, u32 *OutGeneration) 
{
    *OutIndex = (u32)(ID >> 32);
    *OutGeneration = (u32)ID;
}

// MARK: Standard Entities
u64 
CreateEntity(game_state *GameState) 
{
    u32 Index = GameState->EntitiesFreelist[GameState->EntityFreelistCursor++];
    entity *Entity = &GameState->Entities[Index];
    assert(Entity->ID == 0); // This slot better be unused!
    memset(Entity, 0, sizeof(entity));
    GameState->EntityGenerations[Index] += 1;
    Entity->ID = EncodeEntityID(Index, GameState->EntityGenerations[Index]);
    return Entity->ID;
}

b32 
DestroyEntity(game_state *GameState, u64 ID) 
{
    u32 Index, Generation;
    DecodeEntityID(ID, &Index, &Generation);
    entity *Entity = &GameState->Entities[Index];

    if (Entity->ID != ID) 
    {
        return false;
    }

    GameState->EntityFreelistCursor -= 1;
    GameState->EntitiesFreelist[GameState->EntityFreelistCursor] = Index;
    Entity->ID = 0;
    return true;
}

entity *
TryToGetEntity(entity *Entities, u64 ID) 
{
    u32 Index, Generation;
    DecodeEntityID(ID, &Index, &Generation);
    entity *Entity = &Entities[Index];
    
    if (Entity->ID != ID) 
    {
        return nullptr;
    }

    return Entity;
}

entity *
GetEntity(entity *Entities, u32 Index)
{
    entity *Entity = 0;

    if((Index > 0) && (Index < MAX_ENTITIES))
    {
        Entity = &Entities[Index];
    }

    return (Entity);
}

void
CopyEntity(entity *Source, entity *Dest)
{
    Dest->BehaviorFlags = Source->BehaviorFlags;
    Dest->Type = Source->Type;
}

entity *
CreateEntityFromEntityPrototype(game_state *GameState, u32 EntityPrototypeIndex)
{
    entity PrototypeEntity = GameState->EntityPrototypes[EntityPrototypeIndex];

    u64 EntityID = CreateEntity(GameState);
    entity *Entity = TryToGetEntity(GameState->Entities, EntityID);
    CopyEntity(&PrototypeEntity, Entity);
    
    return Entity;
}

b32
EntityExistsAndMatchesType(entity *Entities, u64 EntityID, entity_type MatchingEntityType)
{
    if (EntityID > 0)
    {
        entity *Entity = TryToGetEntity(Entities, EntityID);

        if (Entity != nullptr &&
            Entity->Type == MatchingEntityType)
        {
            return true;
        }
    }

    return false;
}

v2
GetEntityPositionInScreenPixels(tile_map *TileMap, tile_map_position *EntityP, screen *Screen)
{
    v2 PositionInScreenPixels = {};
    tile_map_difference Diff = SubtractInReal32(TileMap, EntityP, &Screen->CameraP);
    PositionInScreenPixels.X = Screen->Center.X + Screen->MetersToPixels*Diff.dXY.X;
    PositionInScreenPixels.Y = Screen->Center.Y - Screen->MetersToPixels*Diff.dXY.Y;
    return PositionInScreenPixels;
}

r32
ScaledSizeUsingPixels(r32 MetersToPixels, r32 SpriteScaleFactor,
                      u32 Pixels)
{
    return (1.0f/MetersToPixels)*Pixels*SpriteScaleFactor;
}

// NOTE: (Ted)  When using this, fill in the relevant behavior flags here.
u32
GetInitialBehaviorFlagsForEntityType(entity_type Type)
{
    u32 BehaviorFlags = 0;
    return BehaviorFlags;
}

