
void
ProcessRenderLayers(game_render_commands *RenderCommands)
{
    game_texture_draw_command_buffer *DrawCommandsBuffer = &RenderCommands->DrawCommandsBuffer;

    for (u32 LayerIndex = 0;
         LayerIndex < BOTTOM_LAYER_COUNT;
         LayerIndex += 1)
    {
        render_commands_array *LayerCommands = &DrawCommandsBuffer->BottomLayers[LayerIndex];

        for (u32 Index = 0; 
             Index < LayerCommands->CommandCount; 
             Index++)
        {
            game_texture_draw_command Command = LayerCommands->Commands[Index];
            PlatformProcessTexturedRectangleDrawCommand(RenderCommands, Command);
        }
    }
}

void ToggleFullscreen(game_render_commands *RenderCommands)
{
    if (RenderCommands->Windowed)
    {
        RenderCommands->Windowed = false;
    } else
    {
        RenderCommands->Windowed = true;
    }
}
