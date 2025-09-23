
void
InitializeTextureBuffer(memory_arena *ScratchArena, game_texture_buffer *TextureBuffer, u32 MaxTextures)
{
    TextureBuffer->TexturesLoaded = 0;
    TextureBuffer->MaxTextures = MaxTextures;
    TextureBuffer->Textures = PushArray(ScratchArena, TextureBuffer->MaxTextures, game_texture);
}

#define MAX_TEXTURE_LOAD_INSTRUCTIONS   1100

u32 
LoadGridTexture(spritesheet_load_instruction *LoadInstructions, u32 StartingLoadInstructionCount,
                v2s StartingTilePosition, v2s GridTileSize, u32 *TextureIDs)
{
    spritesheet_load_instruction LoadInstruction = {};
    u32 LoadInstructionCount = StartingLoadInstructionCount;

    s32 StartingRow = StartingTilePosition.Y;
    s32 LastRow = StartingRow + GridTileSize.Y;

    s32 StartingColumn = StartingTilePosition.X;
    s32 LastColumn = StartingColumn + GridTileSize.X;

    u32 LookupIndex = 0;

    for (u32 Row = StartingRow;
         Row < LastRow;
         Row++)
    {
        for (u32 Column = StartingColumn;
             Column < LastColumn;
             Column++)
        {
            LoadInstruction.Position.Row = Row;
            LoadInstruction.Position.Column = Column;
            LoadInstruction.TextureID = &TextureIDs[LookupIndex];
            LoadInstructions[LoadInstructionCount++] = LoadInstruction;
            LookupIndex += 1;
        }
    }

    return LoadInstructionCount;
}

extern "C"
LOAD_PIXEL_ART_TEXTURES(LoadPixelArtTextures)
{
    game_state *GameState = (game_state *)Memory->PermanentStoragePartition.Start;
    memory_arena *ScratchArena = &GameState->ScratchArena;
    InitializeArena(ScratchArena, &Memory->TransientStoragePartition);

    game_texture_buffer *TileTextureBuffer = &TextureBuffers[TileTextureBufferIndex];
    InitializeTextureBuffer(ScratchArena, TileTextureBuffer, 1100);
    TileTextureBuffer->TextureWidth = 24;
    TileTextureBuffer->TextureHeight = 24;

    read_file_result TilePackFile = Memory->PlatformReadPNGFile("art/Tile_Final.png");

    if (TilePackFile.ContentsSize > 0)
    {
        spritesheet TileTextureSpriteSheet = {};
        TileTextureSpriteSheet.Data = (u32 *)TilePackFile.Contents;
        TileTextureSpriteSheet.TextureWidth = 24;
        TileTextureSpriteSheet.TextureHeight = 24;
        TileTextureSpriteSheet.TexturesPerRow = 73;
        TileTextureSpriteSheet.TexturesPerColumn = 46;

        spritesheet_section TileSection = {};
        TileSection.XOffset = 0;
        TileSection.YOffset = 0;
        TileSection.SamplingWidth = 24;
        TileSection.SamplingHeight = 24;

        // NOTE: (Ted)  These are tile textures for the selection area.
        //              They aren't colliding tiles.

        // Top Left Corner (Blue)
        spritesheet_position Position = {};
        Position.Row = 8;
        Position.Column = 60;

        spritesheet_load_instruction LoadInstructions[MAX_TEXTURE_LOAD_INSTRUCTIONS];
        u32 LoadInstructionCount = 0;

        spritesheet_load_instruction LoadInstruction = {};

// MARK:    Color Palette
        Position.Row = 0;
        Position.Column = 0;

        for (s32 Column = 0;
             Column < 48;
             Column += 1)
        {
            LoadInstruction.TextureID = &TextureMap->ColorPalette[Column];
            LoadInstruction.Position = Position;
            LoadInstructions[LoadInstructionCount++] = LoadInstruction;
            Position.Column += 1;
        }

// MARK: Text
        Position.Row = 42;
        Position.Column = 32;

        for (s32 Column = 0;
             Column < 26;
             Column += 1)
        {
            LoadInstruction.TextureID = &TextureMap->Alphabet[Column];
            LoadInstruction.Position = Position;
            LoadInstructions[LoadInstructionCount++] = LoadInstruction;
            Position.Column += 1;
        }

        Position.Row = 41; 
        Position.Column = 32; 

        for (s32 Column = 0;
             Column < 26;
             Column += 1)
        {
            LoadInstruction.TextureID = &TextureMap->AlphabetBlack[Column];
            LoadInstruction.Position = Position;
            LoadInstructions[LoadInstructionCount++] = LoadInstruction;
            Position.Column += 1;
        }

        Position.Row = 43;
        Position.Column = 32;

        for (s32 Column = 0;
             Column < 11;
             Column += 1)
        {
            LoadInstruction.TextureID = &TextureMap->Numbers[Column];
            LoadInstruction.Position = Position;
            LoadInstructions[LoadInstructionCount++] = LoadInstruction;
            Position.Column += 1;
        }

        Position.Row = 43;
        Position.Column = 42;
        LoadInstruction.TextureID = &TextureMap->ColonCharacter;
        LoadInstruction.Position = Position;
        LoadInstructions[LoadInstructionCount++] = LoadInstruction;

        Position.Row = 36;
        Position.Column = 32;
        LoadInstruction.TextureID = &TextureMap->MouseCursor;
        LoadInstruction.Position = Position;
        LoadInstructions[LoadInstructionCount++] = LoadInstruction;

        for (s32 Index = 0;
             Index < LoadInstructionCount;
             Index++)
        {
            spritesheet_load_instruction Instruction = LoadInstructions[Index];
            u32 *TextureID = Instruction.TextureID;
            *TextureID = TileTextureBuffer->TexturesLoaded;
            LoadTileTextureFromSpritesheet(ScratchArena, &TileTextureSpriteSheet, TileTextureBuffer, 
                                           TileSection, Instruction.Position);
        }
    }

#if WINDOWS
    free(TilePackFile.Contents);
    free(TilePackFile.Filename.Characters);
#elif MACOS || IOS
    Memory->PlatformFreeFileMemory(TilePackFile.Contents);
    Memory->PlatformFreeFileMemory(TilePackFile.Filename.Characters);
#endif

    Assert(TileTextureBuffer->TexturesLoaded < TileTextureBuffer->MaxTextures);
}

