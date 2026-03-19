# How To Make Games With This Template

This is a bare-bones 2D sprite-based game engine for Windows and Mac OS. It is not object-oriented. It has a procedural C flavor and is influenced by the handmade game development style of Casey Muratori and Jonathan Blow. There is no Unity, no Godot, no CMake, no framework. Just C++, Direct3D 11, and Metal.

This guide explains how to use the template to make a game, and includes a section for developers who want to go deeper and understand how the engine itself works.

---

## The Big Picture

The codebase is split into two layers:

- **Platform layer** — owns the window, OS event loop, GPU renderer, audio output, and file I/O. You should rarely touch this.
- **Game library** (`code/game_library/`) — where you write your game. It is compiled as a unity build rooted at `game_main.cpp`.

The platform layer calls into the game library on every frame through four entry points. Your game logic lives inside those entry points.

---

## Entry Points

All entry points are declared via macros in `game_runtime.h` and `game_startup.h`. The ones you will use most are:

### `GameUpdate`
```cpp
GAME_UPDATE(GameUpdate)  // expands to: void GameUpdate(game_memory *Memory, game_input *Input, game_render_commands *RenderCommands)
```
Called once per frame. Put all game logic here: input handling, entity movement, state transitions, collision. Do **not** issue draw commands here.

### `GameRender`
```cpp
GAME_RENDER(GameRender)  // expands to: void GameRender(game_memory *Memory, game_texture_map *TextureMap, game_input *Input, game_render_commands *RenderCommands)
```
Called once per frame after `GameUpdate`. Issue all draw commands here using `PushTexturedRectangle()`. At the end of `GameRender`, call `ProcessRenderLayers(RenderCommands)` to flush everything to the GPU.

### `GameGetSoundSamples`
```cpp
GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)  // expands to: void GameGetSoundSamples(game_memory *Memory, game_sound_output_buffer *SoundOutputBuffer, game_input *Input)
```
Called by the audio thread. The engine's mixer runs here and fills `SoundOutputBuffer`. You typically do not need to modify this unless you are building a custom audio system.

### Startup Entry Points

These run once at startup, not every frame:

| Macro | Purpose |
|---|---|
| `GAME_INITIALIZE_MEMORY` | Initialize `game_state` and memory arenas |
| `GAME_SETUP_RENDERER` | Set up render layer capacities |
| `GAME_POST_CONTENT_LOAD_SETUP` | Runs after textures and sounds are loaded |
| `LOAD_PIXEL_ART_TEXTURES` | Load sprites from PNG spritesheets |
| `LOAD_SOUNDS` | Load audio files |

---

## game_state

`game_state` is the root of all game data. It lives at the base of `PermanentStoragePartition` — a block of memory pre-allocated at startup. You access it at the top of every entry point:

```cpp
game_state *GameState = (game_state *)Memory->PermanentStoragePartition.Start;
```

Its definition is in `game_main.h`. Key fields:

```cpp
struct game_state
{
    memory_arena ScratchArena;         // Temporary per-frame scratch space
    memory_arena WorldArena;           // Entities, tile maps, game world data
    memory_arena LongtermStorageArena; // Things that outlive level loads

    game_audio_state AudioState;       // All audio state

    entity Entities[MAX_ENTITIES];     // Flat entity array (1000 slots)
    u32    EntitiesFreelist[MAX_ENTITIES];
    s64    EntityFreelistCursor;
    u32    EntityGenerations[MAX_ENTITIES];

    gameplay_state_map *GameplayStateMap; // 2D grid parallel to the tile map

    screen Screen;                     // Camera position and pixel scaling

    // ... UI, input repeat timers, achievements, etc.
};
```

### Initializing game_state

Add fields to the struct in `game_main.h`, then initialize them in `GameInitializeMemory` (`game_startup.cpp`):

```cpp
extern "C"
GAME_INITIALIZE_MEMORY(GameInitializeMemory)
{
    game_state *GameState = (game_state *)Memory->PermanentStoragePartition.Start;

    // Initialize your new fields here
    GameState->MyCounter = 0;

    // Memory arenas are initialized from the remaining space in the partition
    memory_arena *WorldArena = &GameState->WorldArena;
    InitializeArena(WorldArena,
                    Memory->PermanentStoragePartition.Size - sizeof(game_state),
                    (u8 *)Memory->PermanentStoragePartition.Start + sizeof(game_state));
}
```

### Allocating from Arenas

All dynamic allocation uses bump allocators — there are no individual `free()` calls. Use the `PushStruct` and `PushArray` macros:

```cpp
// Allocate a single struct
MyThing *Thing = PushStruct(&GameState->WorldArena, MyThing);

// Allocate an array of 64 floats
r32 *Floats = PushArray(&GameState->WorldArena, 64, r32);
```

---

## Drawing Sprites

### Coordinate System

The renderer uses **screen-space pixel coordinates**. The origin `(0, 0)` is the **top-left** corner of the window. X increases to the right, Y increases downward. All draw calls use pixel values.

### PushTexturedRectangle

To draw a sprite, call `PushTexturedRectangle()` inside `GameRender`:

```cpp
// Using v2 min/max corners directly
PushTexturedRectangle(RenderCommands,
                      TextureAtlasTypeTiles,  // which texture atlas
                      vMin,                   // top-left corner (v2, in pixels)
                      vMax,                   // bottom-right corner (v2, in pixels)
                      TextureID,              // u32 texture index
                      1.0f,                   // alpha (0.0–1.0)
                      ZLayer,                 // depth layer (0–34)
                      0.0f);                  // rotation in radians

// Using a rectangle2
rectangle2 Rect = Rectangle2(V2(100.f, 100.f), V2(124.f, 124.f));
PushTexturedRectangle(RenderCommands, TextureAtlasTypeTiles, Rect,
                      TextureMap->MouseCursor, 1.0f, 34, 0.0f);
```

`rectangle2` is a convenience struct with `Min` and `Max` fields. `Rectangle2(min, max)` constructs one.

### Z-Layers

There are 35 layers (0–34). Layer 0 is the furthest back; layer 34 is the front. A typical layout:

| Layer | Use |
|---|---|
| 0–1 | Background fills |
| 2–19 | Tiles, terrain |
| 20–21 | Entities, characters |
| 27 | Foreground tiles |
| 33 | Particles |
| 34 | UI, mouse cursor |

### Texture IDs and game_texture_map

`TextureID` is a `u32` that indexes into the loaded texture array. `game_texture_map` (defined in `game_texture_map.h`) holds named `u32` fields for each sprite so you do not have to remember raw numbers:

```cpp
struct game_texture_map
{
    u32 MissingAsset;
    u32 Alphabet[26];
    u32 Numbers[11];
    u32 MouseCursor;
    u32 ColorPalette[48];
    // Add your sprites here
};
```

Access them through the `TextureMap` pointer passed into `GameRender`:

```cpp
PushTexturedRectangle(RenderCommands, TextureAtlasTypeTiles, Rect,
                      TextureMap->ColorPalette[5], 1.0f, 0, 0.0f);
```

### Adding a New Sprite

1. Add your sprite to the PNG spritesheet in `resources/art/`.
2. Add a `u32` field to `game_texture_map` in `game_texture_map.h`:
   ```cpp
   u32 MyNewSprite;
   ```
3. Register it in `LoadPixelArtTextures()` (`game_texture_loading.cpp`):
   ```cpp
   spritesheet_position Position = {};
   Position.Row    = 10;  // Row in the spritesheet grid
   Position.Column = 5;   // Column in the spritesheet grid

   texture_load_instruction LoadInstruction = {};
   LoadInstruction.TextureID = &TextureMap->MyNewSprite;
   LoadInstruction.Position  = Position;
   LoadInstructions[LoadInstructionCount++] = LoadInstruction;
   ```

### Flushing Draw Commands

At the end of `GameRender`, always call:

```cpp
ProcessRenderLayers(RenderCommands);
```

This converts all queued draw commands from all 35 layers into the GPU instance buffer, in back-to-front order.

---

## Playing Sound Effects

### Playing a Sound

Call `PlaySound()` from anywhere that has access to `GameState->AudioState`. It is typically called in `GameUpdate` in response to game events:

```cpp
PlaySound(&GameState->AudioState, SoundID_TestBlip, 1.0f);  // (AudioState, SoundID, Volume)
```

`PlaySound` returns a `playing_sound *` that you can use to modify the sound while it plays:

```cpp
playing_sound *Sound = PlaySound(&GameState->AudioState, SoundID_TestBlip);
SoundSetPitch(Sound, 1.5f);                    // Speed it up
SoundChangeVolumeByTime(Sound, 0.5f, 0.2f);    // Fade volume over 0.2 seconds
FadeOutAndKillSound(Sound);                     // Fade out and stop
```

### Adding a Sound

1. Place your audio file in `resources/sounds/`.
2. Add an entry to the `loaded_sound_id` enum in `game_sound.h`:
   ```cpp
   enum loaded_sound_id
   {
       SoundID_TestBlip = 0,
       SoundID_MyExplosion = 1
   };
   ```
3. Load it in `LoadSounds()` (`game_sound_loading.cpp`) using the platform file I/O functions.

---

## Platform Services

The `game_memory *Memory` pointer passed to every entry point provides access to platform services through function pointers:

```cpp
// Read a raw binary file
read_file_result File = Memory->PlatformReadEntireFile("data/myfile.bin");

// Read a PNG image (used by the texture loader)
read_file_result PNG = Memory->PlatformReadPNGFile("art/MySheet.png");

// Write a file (useful for save data, settings)
Memory->PlatformWriteEntireFile("settings.sav", sizeof(MyData), &MyData);

// Log a message to the debugger output
Memory->PlatformLogMessage("Something happened");

// Quit the game
Memory->PlatformQuitGame();
```

All file paths are relative to the build output directory (e.g., `build/windows/` on Windows).

---

## Entities

Entities are stored in a flat array with a freelist for reuse. They use a generation counter for safe handle invalidation.

```cpp
// Create an entity
entity *Player = CreateEntity(GameState);

// Access an entity by index (check generation before use)
entity *E = &GameState->Entities[Index];

// Mark entity for reuse (returns it to freelist)
DeleteEntity(GameState, EntityIndex);
```

The `entity` struct (defined in `game_entity.h`) uses a fat-struct layout: all entities share the same struct, and `BehaviorFlags` or `entity_type` controls what fields are active.

---

## Building

**Windows** (from `code/windows_platform/` in a Visual Studio developer prompt):
```bat
build.bat        # Full debug build
b.bat            # Fast rebuild (skips asset packing and shader compilation)
run.bat          # Launch the game
```

**Mac OS** (from `code/mac_platform/`):
```sh
./build.sh       # Full debug build
./run.sh         # Run the app bundle
```

---

## Advanced: How the Engine Works

This section is for developers who want to understand the engine at a lower level — and use this project as a starting point for building their own.

### The Unity Build

There is no CMake, no Makefile, and no incremental linker. The entire game library is compiled from a single file: `game_library/game_main.cpp`. It `#include`s every other `.cpp` file in the library. This means the compiler sees the whole program at once, which enables global inlining and produces small, fast binaries. It also means include order matters.

### Memory Architecture

All memory is allocated up front at startup in `windows_main.cpp` or `osx_main.mm` using `VirtualAlloc` / `mmap`. The platform fills in a `game_memory` struct with partitions (start pointer + size). The game library uses bump allocators (`memory_arena`) to allocate from these partitions. There is no heap, no `malloc`, no garbage collector.

Arenas grow forward and never shrink. The `TransientStoragePartition` is reset each frame — useful for per-frame scratch allocations. The `PermanentStoragePartition` persists for the life of the game.

### The Rendering Pipeline

1. During `GameRender`, game code calls `PushTexturedRectangle()`. This writes a `game_texture_draw_command` (containing `vMin`, `vMax`, `TextureID`, `Alpha`, `Rotation`, `ZLayer`) into one of 35 `render_commands_array` buckets.

2. At the end of `GameRender`, `ProcessRenderLayers()` iterates all 35 buckets in order (0 to 34) and calls `PlatformProcessTexturedRectangleDrawCommand()` for each command. This writes a `texture_draw_command_instance_uniforms` entry (containing `vMin`, `vMax`, `Rotation`, `TextureID`, `Alpha`) into a flat instance buffer.

3. The platform layer calls `TransferInstanceBufferToGPU()`, which maps the D3D11 / Metal buffer and `memcpy`s the instance data to the GPU.

4. The GPU renders all sprites in a single `DrawInstanced` call (D3D11) or `drawPrimitives:instanceCount:` call (Metal). Each instance is one sprite quad.

5. The vertex shader receives `vMin`, `vMax`, and `Rotation` per instance and computes the full scale-rotate-translate transform on the GPU, then converts to normalized device coordinates. There is no CPU matrix multiplication.

The pixel art shader uses a technique to preserve hard pixel edges at non-integer scale factors by snapping UV coordinates to the nearest texel boundary using `fwidth`.

### The Platform / Game Boundary

The game library is compiled as a DLL (Windows) or dylib/framework (Mac). The platform layer loads it and calls the exported entry points. All communication goes through three structs: `game_memory` (services and memory), `game_input` (keyboard, mouse, controller), and `game_render_commands` (draw state). The game library never calls OS APIs directly.

This boundary also makes hot-reloading possible: on Windows, the build script compiles a new DLL and the platform can reload it without restarting the process, preserving all game state in `game_memory`.

### The Shader Compilation Step

On Windows, `fxc.exe` compiles `pixel_art_shader.hlsl` into bytecode and writes it as a C header (`pixel_art_vertex_shader.h`, `pixel_art_fragment_shader.h`). These headers are `#include`d directly inside `windows_main.cpp` at the point where the shader is created. This means the compiled shader bytecode is embedded in the executable — no shader files need to ship with the game.

### Input Model

`game_input` contains a `game_controller_input` (analog sticks, gamepad buttons) and a `keyboard_input` (per-key `WasDown` / `IsDown` state). Input is edge-detected by comparing the current and previous frame states. The engine provides `WasPressed()` and `IsHeld()` helpers.

### Audio Model

The platform creates a secondary audio thread that periodically calls `GameGetSoundSamples`. This function runs the software mixer: it iterates all active `playing_sound` entries in `game_audio_state`, resamples each loaded sound to the output sample rate, applies volume and pitch envelopes, and mixes them into a stereo `s16` output buffer. The platform feeds that buffer directly to the OS audio API (WASAPI on Windows, AudioUnit/AVAudioEngine on Mac).
