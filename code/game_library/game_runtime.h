
#define GAME_SETUP_MEMORY(name) void name(game_memory *Memory)
typedef GAME_SETUP_MEMORY(game_setup_memory);

#define GAME_SETUP_MEMORY_PARTITIONS(name) void name(game_memory *Memory)
typedef GAME_SETUP_MEMORY_PARTITIONS(game_setup_memory_partitions);

#define GAME_CLEAR_TRANSIENT_MEMORY(name) void name(game_memory_partition *TransientStoragePartition)
typedef GAME_CLEAR_TRANSIENT_MEMORY(game_clear_transient_memory);

#define GAME_SETUP_VERTEX_BUFFER(name) void name(game_render_commands *RenderCommands)
typedef GAME_SETUP_VERTEX_BUFFER(game_setup_vertex_buffer);

#define GAME_UPDATE(name) void name(game_memory *Memory, game_input *Input, game_render_commands *RenderCommands)
typedef GAME_UPDATE(game_update);

#define GAME_RENDER(name) void name(game_memory *Memory, game_texture_map *TextureMap, game_input *Input, game_render_commands *RenderCommands)
typedef GAME_RENDER(game_render);

#define GAME_GET_SOUND_SAMPLES(name) void name(game_memory *Memory, game_sound_output_buffer *SoundOutputBuffer, game_input *Input)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);
