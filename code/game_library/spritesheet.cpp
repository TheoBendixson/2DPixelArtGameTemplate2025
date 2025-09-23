
// NOTE: (Ted)  Only use this for the player character sprite sheet. It has problems with other types of
//              spreadsheets, and I don't want to go through the trouble of making this behave more generically.
void 
LoadTileTextureFromSpritesheet(memory_arena *ScratchArena, spritesheet *SpriteSheet, game_texture_buffer *TextureBuffer, 
                               spritesheet_section SpriteSheetSection, spritesheet_position Position)
{
    game_texture Texture = TextureBuffer->Textures[TextureBuffer->TexturesLoaded];
    Texture.Width = SpriteSheetSection.SamplingWidth;
    Texture.Height = SpriteSheetSection.SamplingHeight;
    u32 TotalPixels = Texture.Width*Texture.Height;
    Texture.Data = PushArray(ScratchArena, TotalPixels, u32);
    u32 *PixelDest = (u32 *)Texture.Data;
    u32 *PixelSource = SpriteSheet->Data;

    u32 TextureWidth = SpriteSheet->TextureWidth;
    u32 TextureHeight = SpriteSheet->TextureHeight;
    u32 RowSizeInPixels = SpriteSheet->TexturesPerRow*TextureWidth;

    s32 StartingXOffsetInPixels = Position.Column*TextureWidth;
    u32 DestRowCount = 0;

    for (s32 Row = (Position.Row*TextureHeight + SpriteSheetSection.SamplingHeight -1); 
         ((Row >= (Position.Row*TextureHeight)) && (Row >= 0)); 
         Row--)
    {
        u32 *DestRow = PixelDest + (DestRowCount*Texture.Width);

        for (s32 Column = SpriteSheetSection.XOffset; 
             Column < SpriteSheetSection.SamplingWidth; 
             Column++)
        {
            *DestRow++ = PixelSource[StartingXOffsetInPixels + (Row*RowSizeInPixels) + Column];
        }

        DestRowCount++;
    }

   TextureBuffer->Textures[TextureBuffer->TexturesLoaded] = Texture; 
   TextureBuffer->TexturesLoaded++;
}

struct spritesheet_load_instruction
{
    u32 *TextureID;
    spritesheet_position Position;
};
