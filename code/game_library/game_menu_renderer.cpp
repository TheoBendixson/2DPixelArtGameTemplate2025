
struct decomposed_integer
{
    u32 HundredsPlace;
    u32 TensPlace;
    u32 OnesPlace;
};

decomposed_integer
DecomposedIntegerFromInteger(s32 IntegerValue)
{
    decomposed_integer Result = {};
    Result.HundredsPlace = (IntegerValue/100) % 10;
    Result.TensPlace = (IntegerValue/10) % 10;
    Result.OnesPlace = (IntegerValue) % 10;
    return Result;
}

s32
DecomposedIntegerCharacterCount(decomposed_integer DecomposedInteger)
{
    s32 Count = 1;

    if (DecomposedInteger.TensPlace > 0)
    {
        Count += 1;
    }

    if (DecomposedInteger.HundredsPlace > 0)
    {
        Count += 1;
    }

    return Count;
}

v2
DrawDecomposedInteger(game_render_commands *RenderCommands, u32 *Numbers, decomposed_integer DecomposedInteger,
                      v2 NumberMin, v2 VTileSideInPixels, s32 ZLayer)
{
    if (DecomposedInteger.HundredsPlace > 0)
    {
        rectangle2 NumberRect = Rectangle2(NumberMin, (NumberMin + VTileSideInPixels));
        PushTexturedRectangle(RenderCommands, TextureAtlasTypeTiles, NumberRect, 
                              Numbers[DecomposedInteger.HundredsPlace], 1.0f, ZLayer, 0.0f);
        NumberMin.X += VTileSideInPixels.X;
    }

    if (DecomposedInteger.TensPlace > 0)
    {
        rectangle2 NumberRect = Rectangle2(NumberMin, (NumberMin + VTileSideInPixels));
        PushTexturedRectangle(RenderCommands, TextureAtlasTypeTiles, NumberRect, 
                              Numbers[DecomposedInteger.TensPlace], 1.0f, ZLayer, 0.0f);
        NumberMin.X += VTileSideInPixels.X;
    }

    {
        rectangle2 NumberRect = Rectangle2(NumberMin, (NumberMin + VTileSideInPixels));
        PushTexturedRectangle(RenderCommands, TextureAtlasTypeTiles, NumberRect, 
                              Numbers[DecomposedInteger.OnesPlace], 1.0f, ZLayer, 0.0f);
        NumberMin.X += VTileSideInPixels.X;
    }

    return NumberMin;
}

void
DrawThreeByThree(game_render_commands *RenderCommands, v2 StartMin, v2 TileSideInPixels,
                 u32 *ThreeByThree, u32 ZLayer)
{
    v2 Min = StartMin;
    u32 TextureIndex = 0;

    for (u32 Row = 0;
         Row < 3;
         Row += 1)
    {
        Min.X = StartMin.X;

        for (u32 Column = 0;
             Column < 3;
             Column += 1)
        {

            PushTexturedRectangle(RenderCommands, TextureAtlasTypeTiles, Rectangle2(Min, Min + TileSideInPixels), 
                                  ThreeByThree[TextureIndex], 1.0f, ZLayer, 0.0f);

            TextureIndex += 1;
            Min.X += TileSideInPixels.X;
        }

        Min.Y += TileSideInPixels.Y;
    }
}

u32
GetThreeByThreeTextureRow(s32 Row, s32 RowCount)
{
    u32 TextureRow = 0;

    if (RowCount == 2)
    {
        if (Row == 0)
        {
            TextureRow = 0;
        }

        if (Row == 1)
        {
            TextureRow = 2;
        }

    } else
    {
        if (Row > 0 &&
            Row < (RowCount -1))
        {
            TextureRow = 1;
        }

        if (Row == (RowCount -1))
        {
            TextureRow = 2;
        }
    }

    return TextureRow;
}

// NOTE: (Ted)  This is always taller and wider than three.
void
DrawExpandedThreeByThree(game_render_commands *RenderCommands, v2 TileSideInPixels, 
                         v2 StartMin, u32 *ThreeByThree, u32 ZLayer, 
                         s32 RowCount, s32 ColumnCount)
{
    v2 Min = StartMin;

    for (u32 Row = 0;
         Row < RowCount;
         Row += 1)
    {
        u32 TextureRow = GetThreeByThreeTextureRow(Row, RowCount);

        PushTexturedRectangle(RenderCommands, TextureAtlasTypeTiles, Rectangle2(Min, Min + TileSideInPixels), 
                              ThreeByThree[TextureRow*3], 1.0f, ZLayer, 0.0f);

        Min.Y += TileSideInPixels.Y;
    }

    Min = StartMin;
    Min.X += TileSideInPixels.X;

    v2 MiddleTilesStart = Min;
    s32 MidColumnCount = ColumnCount - 2;

    for (u32 Row = 0;
         Row < RowCount;
         Row += 1)
    {
        u32 TextureRow = GetThreeByThreeTextureRow(Row, RowCount);
        Min.X = MiddleTilesStart.X;

        for (u32 Column = 0;
             Column < MidColumnCount;
             Column += 1)
        {
            PushTexturedRectangle(RenderCommands, TextureAtlasTypeTiles, Rectangle2(Min, Min + TileSideInPixels), 
                                  ThreeByThree[1 + TextureRow*3], 1.0f, ZLayer, 0.0f);
            Min.X += TileSideInPixels.X;
        }

        Min.Y += TileSideInPixels.Y;
    }

    Min.Y = MiddleTilesStart.Y;

    for (u32 Row = 0;
         Row < RowCount;
         Row += 1)
    {
        u32 TextureRow = GetThreeByThreeTextureRow(Row, RowCount);
        PushTexturedRectangle(RenderCommands, TextureAtlasTypeTiles, Rectangle2(Min, Min + TileSideInPixels), 
                              ThreeByThree[2 + TextureRow*3], 1.0f, ZLayer, 0.0f);
        Min.Y += TileSideInPixels.Y;
    }
} 

void
DrawTileCoordinateText(game_render_commands *RenderCommands, v2 StartPosition, game_texture_map *TextureMap,
                       u32 LetterTextureID, decomposed_integer TileValue, v2 TextRenderSize, s32 ZLayer)
{
    rectangle2 Quad = Rectangle2(StartPosition, (StartPosition + TextRenderSize)); 
    PushTexturedRectangle(RenderCommands, TextureAtlasTypeTiles, Quad, LetterTextureID, 1.0f, ZLayer, 0.0f);

    Quad = IncrementedXByLength(Quad, TextRenderSize.X);

    PushTexturedRectangle(RenderCommands, TextureAtlasTypeTiles, Quad, TextureMap->ColonCharacter, 1.0f, 
                          ZLayer, 0.0f);

    Quad = IncrementedXByLength(Quad, TextRenderSize.X);

    u32 NumberLookups[3];
    NumberLookups[0] = TileValue.HundredsPlace;
    NumberLookups[1] = TileValue.TensPlace;
    NumberLookups[2] = TileValue.OnesPlace;

    for (s32 Column = 0;
         Column < 3;
         Column += 1)
    {
        u32 LookupIndex = NumberLookups[Column];
        PushTexturedRectangle(RenderCommands, TextureAtlasTypeTiles, Quad, TextureMap->Numbers[LookupIndex], 1.0f, 
                              ZLayer, 0.0f);
        Quad = IncrementedXByLength(Quad, (TextRenderSize.X + TextRenderSize.X*0.1f));
    } 
}

