
extern "C"
GAME_SETUP_VERTEX_BUFFER(GameSetupVertexBuffer)
{
    r32 Min = -0.5f;
    r32 Max = 0.5f;

    game_texture_vertex TextureQuadVertices[] =
    {
        { { Min, Min }, { 0.0f, 0.0f } },
        { { Max, Min }, { 1.0f, 0.0f } },
        { { Max, Max }, { 1.0f, 1.0f } },

        { { Min, Min }, { 0.0f, 0.0f } },
        { { Min, Max }, { 0.0f, 1.0f } },
        { { Max, Max }, { 1.0f, 1.0f } }
    };

    game_texture_vertex_buffer *VertexBuffer = &RenderCommands->VertexBuffer;
    game_texture_vertex *VBSource = TextureQuadVertices;
    game_texture_vertex *VBDest = VertexBuffer->Vertices;

    for (u32 Index = 0; Index < 6; Index++)
    {
        *VBDest++ = *VBSource++;
    }

    VertexBuffer->VertexCount = 6;
}
