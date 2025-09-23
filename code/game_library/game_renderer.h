
#define MAX_SUPPORTED_TEXTURE_TYPES 13

enum game_source_texture_type
{
    GameSourceTextureTypeTiles
};

enum texture_atlas_type
{
    TextureAtlasTypeTiles
};

struct game_texture_description
{
    u32 TextureWidth;
    u32 TextureHeight;
    texture_atlas_type SourceTexture;
};

struct float_color
{
    r32 Red, Green, Blue, Alpha;
};

struct game_texture_vertex
{
    v2 Position;
    v2 UV;
};

#if MACOS || IOS
struct texture_draw_command_instance_uniforms
{
    matrix_float3x3 Transform;
    u32 TextureID;
    r32 Alpha;
};

struct texture_draw_command_instance_buffer
{
    texture_draw_command_instance_uniforms *InstanceUniforms;
    u32 InstanceCount;
    u32 InstanceMax;
};
#endif

struct game_texture_vertex_buffer
{
    game_texture_vertex *Vertices;
    u32 Size;
    u32 VertexCount;
    u32 Max;
};

#if WINDOWS

struct renderer_instance
{
    r32 TransformRow1[3];
    r32 TransformRow2[3];
    r32 TransformRow3[3];
    u32 TextureID;
    r32 Alpha;
};

struct texture_draw_command_instance_buffer
{
    renderer_instance *Instances;
    u32 Size;
    u32 InstanceCount;
    u32 InstanceMax;
};

#endif

struct game_texture_draw_command
{
    v2 vMin; 
    v2 vMax; 
    u32 TextureID; 
    r32 Alpha; 

    // NOTE: (Ted)  Rotation is acround the z-axis, which points toward.
    //              the viewer.
    r32 Rotation;
    u32 ZLayer;
    texture_atlas_type TextureAtlasType;
};

struct render_commands_array
{
    game_texture_draw_command *Commands;
    u32 CommandCount;
    u32 MaxCommands;
    texture_atlas_type TextureAtlasType;
    s32 DrawnCount;
    b32 BufferDrawn;
};

struct sorted_render_command_layer
{
    render_commands_array Commands;
    u32 CurrentZLayer;
};

#define BOTTOM_LAYER_COUNT  35

struct game_texture_draw_command_buffer
{
    render_commands_array BottomLayers[BOTTOM_LAYER_COUNT];
};

struct game_render_commands
{
    memory_arena DrawCommandsArena; 

    s32 ViewportWidth;
    s32 ViewportHeight;

    r32 SpriteScaleFactor;
    float_color ClearColor;
    b32 Windowed;

    game_texture_draw_command_buffer DrawCommandsBuffer;
    game_texture_vertex_buffer VertexBuffer;
    texture_draw_command_instance_buffer InstanceBuffer;

    game_texture_description SupportedTextureTypes[MAX_SUPPORTED_TEXTURE_TYPES];
    s32 TextureTypeIndices[MAX_SUPPORTED_TEXTURE_TYPES];
    u32 SupportedTextureTypeCount;

#if MACOS || IOS
    // NOTE: (Ted)  This is used in triple-buffering on Apple platforms.
    u32 FrameIndex;
#endif

};
