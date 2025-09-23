
#define LOAD_PIXEL_ART_TEXTURES(name) void name(game_memory *Memory, game_texture_buffer *TextureBuffers, game_texture_map *TextureMap)
typedef LOAD_PIXEL_ART_TEXTURES(load_pixel_art_textures);

#define LOAD_SOUNDS(name) void name(game_memory *Memory)
typedef LOAD_SOUNDS(load_sounds);

struct game_startup_config
{
    u32 SoundSamplesPerSecond;
    u32 SoundBytesPerSample; 
    u32 SoundOutputBufferArrayCount;
    u32 SoundBufferSize;
};

#define GAME_GET_STARTUP_CONFIG(name) game_startup_config name()
typedef GAME_GET_STARTUP_CONFIG(game_get_startup_config);

#define GAME_SETUP_RENDERER(name) void name(game_memory *Memory, game_render_commands *RenderCommands)
typedef GAME_SETUP_RENDERER(game_setup_renderer);

#define GAME_INITIALIZE_MEMORY(name) void name(game_memory *Memory, device_simulator_settings DeviceSimulatorSettings)
typedef GAME_INITIALIZE_MEMORY(game_initialize_memory);

#define GAME_POST_CONTENT_LOAD_SETUP(name) void name(game_memory *Memory, game_texture_map *TextureMap, game_render_commands *RenderCommands)
typedef GAME_POST_CONTENT_LOAD_SETUP(game_post_content_load_setup);
