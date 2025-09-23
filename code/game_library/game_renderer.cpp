struct y_component
{
    r32 Min;
    r32 Max;
};

inline
y_component InvertYAxis(int ViewportHeight, 
                        r32 YMin, r32 YMax)
{
    y_component Result = {};
    Result.Min = (r32)(ViewportHeight - YMin);
    Result.Max = (r32)(ViewportHeight - YMax);
    return (Result);
}

void
PushTexturedRectangle(game_render_commands *RenderCommands, texture_atlas_type TextureAtlasType,
                      v2 vMin, v2 vMax, u32 TextureID, r32 Alpha, s32 ZLayer, r32 Rotation)
{
    game_texture_draw_command_buffer *DrawCommandsBuffer = &RenderCommands->DrawCommandsBuffer;

    // TODO: (Ted)    I might need to change this so it only applies to tightly drawn tiles.
    //                If I ever see small lines between tiles, the overlap margin isn't large enough. 
    //
    //              
    v2 AdjustedMin = V2((vMin.X - 0.01f), (vMin.Y - 0.01f));
    v2 AdjustedMax = V2((vMax.X + 0.01f), (vMax.Y + 0.01f));

    game_texture_draw_command Command = {};

    // TODO: (Ted)  Do something more "official" here. This currently handles tiled entities
    //              being drawn with some degree of alpha blending. If you extend the edges, you
    //              can see the tiles blend over each other, and that's ugly.
    if (Alpha > 0.9f)
    {
        Command.vMin = AdjustedMin;
        Command.vMax = AdjustedMax;
    } else
    {
        Command.vMin = vMin;
        Command.vMax = vMax;
    }

    Command.TextureID = TextureID;
    Command.Alpha = Alpha;
    Command.ZLayer = ZLayer;
    Command.Rotation = Rotation;
    Command.TextureAtlasType = TextureAtlasType;

    render_commands_array *CommandsArray = &DrawCommandsBuffer->BottomLayers[ZLayer];
    CommandsArray->Commands[CommandsArray->CommandCount++] = Command;
    Assert(CommandsArray->CommandCount < CommandsArray->MaxCommands);
}

void
PushTexturedRectangle(game_render_commands *RenderCommands, texture_atlas_type TextureAtlasType,
                      rectangle2 Rectangle, u32 TextureID, r32 Alpha, s32 ZLayer, r32 Rotation)
{
    PushTexturedRectangle(RenderCommands, TextureAtlasType, Rectangle.Min, Rectangle.Max, TextureID, 
                          Alpha, ZLayer, Rotation);
}

#if MACOS || IOS

matrix_float3x3 
GenerateTransformMatrix(v2 vMin, v2 vMax, y_component YComponent, r32 Rotation)
{
    r32 ScaleX = vMax.X - vMin.X;
    r32 ScaleY = vMax.Y - vMin.Y; 

    matrix_float3x3 Scale = (matrix_float3x3) {{
        { ScaleX, 0,      0  },
        { 0,      ScaleY, 0  },
        { 0,      0,      1  },
    }};

    r32 TransX = vMin.X + 0.5f*ScaleX;
    r32 TransY = YComponent.Min - 0.5f*ScaleY; 

    matrix_float3x3 Translate = (matrix_float3x3) {{
        { 1,        0,      0 },
        { 0,        1,      0 },
        { TransX,   TransY, 1 },
    }};

    matrix_float3x3 RotateZ = (matrix_float3x3) {{
        { (r32)(cos(Rotation)), -(r32)(sin(Rotation)),   0,  },
        { (r32)(sin(Rotation)), (r32)(cos(Rotation)),    0,  },
        { 0,                    0,                       1   },
    }};

    return matrix_multiply(Translate, matrix_multiply(RotateZ, Scale));
}

void
PlatformProcessTexturedRectangleDrawCommand(game_render_commands *RenderCommands, game_texture_draw_command Command)
{
    v2 vMin = Command.vMin;
    v2 vMax = Command.vMax;

    y_component YComponent = InvertYAxis(RenderCommands->ViewportHeight, vMin.Y, vMax.Y);

    texture_draw_command_instance_buffer *InstanceBuffer = &RenderCommands->InstanceBuffer; 

    texture_draw_command_instance_uniforms *InstanceUniforms = InstanceBuffer->InstanceUniforms;
    texture_draw_command_instance_uniforms NewInstance = {};

    NewInstance.Transform = GenerateTransformMatrix(vMin, vMax, YComponent, Command.Rotation);
    NewInstance.TextureID = Command.TextureID;
    NewInstance.Alpha = Command.Alpha;

    InstanceUniforms[InstanceBuffer->InstanceCount] = NewInstance;
    InstanceBuffer->InstanceCount++;
    Assert(InstanceBuffer->InstanceCount < InstanceBuffer->InstanceMax);
}

#elif WINDOWS

void
PlatformProcessTexturedRectangleDrawCommand(game_render_commands *RenderCommands, game_texture_draw_command Command)
{
    v2 vMin = Command.vMin;
    v2 vMax = Command.vMax;

    y_component YComponent = InvertYAxis(RenderCommands->ViewportHeight, vMin.Y, vMax.Y);

// MARK: Generate a transform matrix that scales and translates the geometry.
    r32 ScaleX = vMax.X - vMin.X;
    r32 ScaleY = vMax.Y - vMin.Y; 

    matrix3x3 Scale = { ScaleX, 0,      0,      
                        0,      ScaleY, 0,       
                        0,      0,      1 };

    r32 TransX = vMin.X + 0.5f*ScaleX;
    r32 TransY = YComponent.Min - 0.5f*ScaleY; 

    matrix3x3 Translate = { 1,      0,      0,
                            0,      1,      0,
                            TransX, TransY, 1 };

    r32 Rotation = Command.Rotation;

    matrix3x3 RotateZ = { (r32)(cos(Rotation)), -(r32)(sin(Rotation)),   0,  
                          (r32)(sin(Rotation)), (r32)(cos(Rotation)),    0,  
                          0,                    0,                       1 };

    matrix3x3 Transform = RotateZ * Scale * Translate;

    renderer_instance Instance = {};

    Instance.TextureID = Command.TextureID;

    Instance.TransformRow1[0] = Transform.m[0][0];
    Instance.TransformRow1[1] = Transform.m[0][1];
    Instance.TransformRow1[2] = Transform.m[0][2];

    Instance.TransformRow2[0] = Transform.m[1][0];
    Instance.TransformRow2[1] = Transform.m[1][1];
    Instance.TransformRow2[2] = Transform.m[1][2];

    Instance.TransformRow3[0] = Transform.m[2][0];
    Instance.TransformRow3[1] = Transform.m[2][1];
    Instance.TransformRow3[2] = Transform.m[2][2];

    Instance.Alpha = Command.Alpha;

    texture_draw_command_instance_buffer *InstanceBuffer = &RenderCommands->InstanceBuffer; 
    InstanceBuffer->Instances[InstanceBuffer->InstanceCount] = Instance;
    InstanceBuffer->InstanceCount += 1;
    Assert(InstanceBuffer->InstanceCount < InstanceBuffer->InstanceMax);
}

#endif

void 
DrawMonospaceLineOfText(game_render_commands *RenderCommands, text_character_lookup_array *TextCharacters,
                        v2 LetterMin, v2 TextRenderSize, u32 TextCharacterZLayer)
{
    r32 Spacing = 4.0f;

    for (u32 CharacterIndex = 0;
         CharacterIndex < TextCharacters->Count;
         CharacterIndex++)
    {
        text_character TextCharacter = TextCharacters->TextCharacters[CharacterIndex];

        if (TextCharacter.Type == TextCharacterTypeTextureLookup)
        {
            rectangle2 Quad = Rectangle2(LetterMin, (LetterMin + TextRenderSize));
            PushTexturedRectangle(RenderCommands, TextureAtlasTypeTiles, Quad, 
                                  TextCharacter.TextureID, 1.0f, TextCharacterZLayer, 0.0f);
        }

        LetterMin.X += TextRenderSize.X + Spacing;
    }
}

rectangle2
CalculateMouseCursorRect(v2 MousePos, v2 VTileSideInPixels)
{
    rectangle2 Rect = {};
    v2 Offset = V2(VTileSideInPixels.X*0.31f,
                   VTileSideInPixels.Y*0.16f);
    Rect.Min = MousePos  - Offset;
    Rect.Max = Rect.Min + (VTileSideInPixels);
    return Rect;
}

void
DrawMouseCursor(game_render_commands *RenderCommands, game_texture_map *TextureMap,
                v2 MousePos, v2 VTileSideInPixels, u32 ZLayer)
{
    rectangle2 Rect = CalculateMouseCursorRect(MousePos, VTileSideInPixels);
    PushTexturedRectangle(RenderCommands, TextureAtlasTypeTiles, Rect, 
                          TextureMap->MouseCursor, 1.0f, ZLayer, 0.0f);

    b32 DebugDrawingEnabled = false;

    if (DebugDrawingEnabled)
    {
        r32 TenthTile = VTileSideInPixels.X*0.1f;
        v2 VTenthTile = V2(TenthTile, TenthTile);
        Rect = {};
        Rect.Min = MousePos - VTenthTile*0.5f;
        Rect.Max = Rect.Min + VTenthTile;

        PushTexturedRectangle(RenderCommands, TextureAtlasTypeTiles, Rect, 
                              TextureMap->ColorPalette[2], 1.0f, (ZLayer - 1), 0.0f);
    }
}

