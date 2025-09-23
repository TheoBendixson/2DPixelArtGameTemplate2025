

rectangle2
DrawEditorMenuBackground(game_render_commands *RenderCommands, s32 TextCharacterCount, rectangle2 StartQuad, 
                         u32 *BackgroundTextureIDs, s32 ZLayer, v2 BackgroundRenderSize)
{
    rectangle2 Quad = StartQuad;

    PushTexturedRectangle(RenderCommands, TextureAtlasTypeTiles, Quad, 
                          BackgroundTextureIDs[0], 1.0f, ZLayer, 0.0f);
    Quad = IncrementedXByLength(Quad, BackgroundRenderSize.X);

    for (s32 Index = 0;
         Index < (TextCharacterCount - 2);
         Index += 1)
    {
        PushTexturedRectangle(RenderCommands, TextureAtlasTypeTiles, Quad, 
                              BackgroundTextureIDs[1], 1.0f, ZLayer, 0.0f);
        Quad = IncrementedXByLength(Quad, BackgroundRenderSize.X);
    }

    PushTexturedRectangle(RenderCommands, TextureAtlasTypeTiles, Quad, 
                          BackgroundTextureIDs[2], 1.0f, ZLayer, 0.0f);

    return Quad;
}

