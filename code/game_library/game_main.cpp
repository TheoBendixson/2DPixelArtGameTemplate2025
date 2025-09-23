
#include "game_vertex_loading.cpp"
#include "intrinsics.h"
#include "spritesheet.cpp"
#include "game_texture_loading.cpp"
#include "game_sound_loading.cpp"
#include "tile_map.cpp"
#include "entity.cpp"
#include "gameplay_state_map.cpp"
#include "game_sound.cpp"
#include "animation.cpp"
#include "game_texture_map.cpp"
#include "camera.cpp"

#include "game_startup.cpp"
#include "game_memory.cpp"
#include "game_renderer.cpp"
#include "game_draw_commands.cpp"
#include "game_menu_renderer.cpp"
#include "game_input.cpp"
#include "game_achievement.cpp"
#include "game_text.cpp"
#include "text_renderer.cpp"

#if LEVELEDITOR
#include "world_editor.cpp"
#endif

extern "C"
GAME_UPDATE(GameUpdate)
{
    game_state *GameState = (game_state*)Memory->PermanentStoragePartition.Start;

    if (!GameState->SoundAndGraphicsLoaded)
    {
        return;
    }

    game_controller_input *Controller = &Input->Controller;
    keyboard_input *Keyboard = &Input->Keyboard;
	
// MARK: Process Repeatable Keys
    {
        b32 KeysDown[REPEATABLE_KEY_COUNT] = 
            { false,
              (Keyboard->Left.EndedDown || 
               Keyboard->Letters['A' - 'A'].EndedDown),
              (Keyboard->Right.EndedDown || 
               Keyboard->Letters['D' - 'A'].EndedDown),
              (Keyboard->Up.EndedDown || 
               Keyboard->Letters['W' - 'A'].EndedDown),
              (Keyboard->Down.EndedDown || 
               Keyboard->Letters['S' - 'A'].EndedDown),
              Keyboard->Enter.EndedDown || Controller->A.EndedDown,
              Keyboard->Letters['Q' - 'A'].EndedDown,
              Keyboard->Letters['E' - 'A'].EndedDown,
              Keyboard->Letters['R' - 'A'].EndedDown,
              Keyboard->F[1].EndedDown || Controller->LeftShoulder1.EndedDown,
              Keyboard->F[2].EndedDown,
              Keyboard->F[3].EndedDown,
              Keyboard->F[5].EndedDown,
              Keyboard->F[6].EndedDown,
              Keyboard->F[7].EndedDown,
              Keyboard->F[8].EndedDown,
              Keyboard->F[9].EndedDown,
              Keyboard->F[13].EndedDown,
              Keyboard->F[14].EndedDown,
              Keyboard->F[15].EndedDown,
              Keyboard->Escape.EndedDown,
              Keyboard->F[11].EndedDown,
              Input->MouseButtons[0].EndedDown,
              Input->MouseButtons[2].EndedDown
        };

        b32 KeysWentDown[REPEATABLE_KEY_COUNT] = 
            { false,
              (ButtonWentDown(&Keyboard->Left) || 
               ButtonWentDown(&Keyboard->Letters['A' - 'A'])),
              (ButtonWentDown(&Keyboard->Right) || 
               ButtonWentDown(&Keyboard->Letters['D' - 'A'])),
              (ButtonWentDown(&Keyboard->Up) || 
               ButtonWentDown(&Keyboard->Letters['W' - 'A'])),
              (ButtonWentDown(&Keyboard->Down) || 
               ButtonWentDown(&Keyboard->Letters['S' - 'A'])),
              (ButtonWentDown(&Keyboard->Enter)),
              ButtonWentDown(&Keyboard->Letters['Q' - 'A']), 
              ButtonWentDown(&Keyboard->Letters['E' - 'A']), 
              ButtonWentDown(&Keyboard->Letters['R' - 'A']), 
              ButtonWentDown(&Keyboard->F[1]),
              ButtonWentDown(&Keyboard->F[2]),
              ButtonWentDown(&Keyboard->F[3]),
              ButtonWentDown(&Keyboard->F[5]),
              ButtonWentDown(&Keyboard->F[6]),
              ButtonWentDown(&Keyboard->F[7]),
              ButtonWentDown(&Keyboard->F[8]),
              ButtonWentDown(&Keyboard->F[9]),
              ButtonWentDown(&Keyboard->F[13]),
              ButtonWentDown(&Keyboard->F[14]),
              ButtonWentDown(&Keyboard->F[15]),
              ButtonWentDown(&Keyboard->Escape),
              ButtonWentDown(&Keyboard->F[11]),
              ButtonWentDown(&Input->MouseButtons[0]),
              ButtonWentDown(&Input->MouseButtons[2])
            };

        // Reset timer if a new key was pressed.
        b32 WentDown = false;

        for (s32 Index = 0; 
             Index < (s32)REPEATABLE_KEY_COUNT; 
             Index++)
        {
            if (KeysWentDown[Index])
            {
                GameState->LastPressedRepeatableKey = (repeatable_input_key)Index;
                GameState->RepeatableKeyTimer = 0;
                GameState->RepeatableKeyIsRepeating = false;
                WentDown = true;
            }
        }

        // Update the pressed key.
        if (!WentDown && GameState->LastPressedRepeatableKey)
        {
            if (KeysDown[GameState->LastPressedRepeatableKey])
            {
                s32 InitialTime = 12;
                s32 RepeatTime = 12;

                GameState->RepeatableKeyTimer++;

                if (GameState->RepeatableKeyIsRepeating)
                {
                    // NOTE: (Pere)  Every time this timer is 0 it counts as a key press/repeat.
                    GameState->RepeatableKeyTimer %= RepeatTime; 
                } else if (GameState->RepeatableKeyTimer > InitialTime)
                {
                    GameState->RepeatableKeyIsRepeating = true;
                    GameState->RepeatableKeyTimer = 0;
                }
            }else
            { // Stopped pressing
                GameState->LastPressedRepeatableKey = RepeatableKey_None;
                GameState->RepeatableKeyTimer = 0;
                GameState->RepeatableKeyIsRepeating = false;
            }
        }
    }

    if (PressedRepeatableKey(GameState, RepeatableKey_F11))
    {
        ToggleFullscreen(RenderCommands);
    }

    if (PressedRepeatableKey(GameState, RepeatableKey_MouseLeftClick))
    {
        PlaySound(&GameState->AudioState, SoundID_TestBlip, 1.0f);
    }

    gameplay_state_map *GameplayStateMap = GameState->GameplayStateMap;

    r32 TilesHigh = (r32)GameState->RenderTilesHigh;
    r32 TileSideInPixels = s32(RenderCommands->ViewportHeight/TilesHigh);

    r32 ScreenWidth = (r32)RenderCommands->ViewportWidth;
    r32 ScreenHeight = (r32)RenderCommands->ViewportHeight;
    s32 RowAmount = (s32)(ScreenHeight/TileSideInPixels)/2 + 2;
    s32 ColumnAmount = (s32)(ScreenWidth/TileSideInPixels)/2 + 2;

// MARK: Update Entities
    for (u32 EntityIndex = 1; 
         EntityIndex < MAX_ENTITIES; 
         EntityIndex++)
    {
        entity SlotTestEntity = GameState->Entities[EntityIndex];

        if (SlotTestEntity.ID == 0)
        {
            continue;
        }

        entity *Entity = TryToGetEntity(GameState->Entities, SlotTestEntity.ID);

        if (Entity != nullptr)
        {

        }
    }

// MARK: World Editor
#if LEVELEDITOR

    if (PressedRepeatableKey(GameState, RepeatableKey_F2))
    {
        GameState->InLevelEditorMode = !GameState->InLevelEditorMode;
    }

    if (PressedRepeatableKey(GameState, RepeatableKey_F3))
    {
        GameState->DebugDrawingEnabled = !GameState->DebugDrawingEnabled;
    }

    if (Input->OpenLevelThisFrame)
    {
        Memory->PlatformOpenFileDialog(Memory->FileResultPartition.Start);
    }

    if(GameState->InLevelEditorMode)
    {
        if (Input->MouseWheel)
        {
            // TODO: Wheel through entity prototypes
        }

        if (PressedRepeatableKey(GameState, RepeatableKey_MouseLeftClick))
        {
            screen *Screen = &GameState->Screen;
            tile_map_position CameraP = Screen->CameraP;

            // TODO: Place Entity Prototypes however that is done.
        }

        if (PressedRepeatableKey(GameState, RepeatableKey_MouseRightClick))
        {
            screen *Screen = &GameState->Screen;
            tile_map_position CameraP = Screen->CameraP;

            // TODO: Remove entity prototype at position
        }
    }
#endif

// MARK: Game
    s32 TilesWide = s32(RenderCommands->ViewportWidth/TileSideInPixels);
    r32 SpriteScaleFactor = (r32)(TileSideInPixels/24.0);

    r32 MetersToPixels = (r32)TileSideInPixels / 1.0f; 
    GameState->Screen.MetersToPixels = MetersToPixels;
    GameState->Screen.SpriteScaleFactor = SpriteScaleFactor;

    RenderCommands->SpriteScaleFactor = SpriteScaleFactor;

    u32 EnvironmentAnimationFrequency = (u32)(8*Input->FrameRateMultiplier);

    r32 TileSideInMeters = 1.0f;

    v2 ScreenCenter = V2(0.5f*(r32)RenderCommands->ViewportWidth, 
                         0.5f*(r32)RenderCommands->ViewportHeight);

    GameState->Screen.Center = ScreenCenter;
}

extern "C"
GAME_RENDER(GameRender)
{
    if (!Memory->IsInitialized)
    {
        return;
    }

    game_state *GameState = (game_state*)Memory->PermanentStoragePartition.Start;

    if (!GameState->SoundAndGraphicsLoaded)
    {
        float_color ClearColor = {};
        ClearColor.Red = (32.0f/255.0f);
        ClearColor.Green = (103.0f/255.0f);
        ClearColor.Blue = (73.0f/255.0f);
        ClearColor.Alpha = 1.0f;
        RenderCommands->ClearColor = ClearColor;
        return;
    }

    r32 TilesHigh = (r32)GameState->RenderTilesHigh;
    r32 TileSideInPixels = (RenderCommands->ViewportHeight/TilesHigh);
    v2 VTileSideInPixels = V2(TileSideInPixels, TileSideInPixels);

    for (u32 LayerIndex = 0;
         LayerIndex < BOTTOM_LAYER_COUNT;
         LayerIndex += 1)
    {
        render_commands_array *LayerCommands = &RenderCommands->DrawCommandsBuffer.BottomLayers[LayerIndex];
        LayerCommands->CommandCount = 0;
    }

    RenderCommands->InstanceBuffer.InstanceCount = 0;

    // TODO: (Ted)  Maybe change this to the best background color, but only if you really need
    //              to save GPU effort.
    float_color ClearColor = {};
    ClearColor.Alpha = 1.0f;
    ClearColor.Red = (87.0f/255.0f);
    ClearColor.Green = (114.0f/255.0f);
    ClearColor.Blue = (119.0f/255.0f);
    RenderCommands->ClearColor = ClearColor;

// MARK: Game
    if (GameState->ShowFramerate)
    {
        // MARK: Draw Menu Text
        text_character_lookup_array TextCharacters = {};
        PushString(GameState->Framerate, TextureMap->Alphabet, &TextCharacters);

        u32 TextCharacterZLayer = 33;
        v2 VTextSizeInPixels = V2(80.0f, 80.0f);
        v2 QuarterTile = VTextSizeInPixels*0.25f;

        v2 LetterMin = {};
        LetterMin.X = QuarterTile.X;
        LetterMin.Y = QuarterTile.Y;

        DrawMonospaceLineOfText(RenderCommands, &TextCharacters, LetterMin,  
                                QuarterTile, TextCharacterZLayer);

        text_character_lookup_array Digits = {};
        u32 *Numbers = TextureMap->Numbers;

        char FramerateString[10]; 
        FloatingToAlpha(Input->dtForFrame, FramerateString, 6);

        for (u32 Index = 0; 
             Index < 10; 
             Index++)
        {
            char Character = FramerateString[Index];

            if (Character == '\0')
            {
                break;
            }

            u32 TextureID = 0;

            if (Character == '0')
            {
                TextureID = Numbers[0];
            } 
            else if (Character == '1')
            {
                TextureID = Numbers[1];
            }
            else if (Character == '2')
            {
                TextureID = Numbers[2];
            }
            else if (Character == '3')
            {
                TextureID = Numbers[3];
            }
            else if (Character == '4')
            {
                TextureID = Numbers[4];
            }
            else if (Character == '5')
            {
                TextureID = Numbers[5];
            }
            else if (Character == '6')
            {
                TextureID = Numbers[6];
            }
            else if (Character == '7')
            {
                TextureID = Numbers[7];
            }
            else if (Character == '8')
            {
                TextureID = Numbers[8];
            }
            else if (Character == '9')
            {
                TextureID = Numbers[9];
            } 
            else if (Character == '.')
            {
                TextureID = Numbers[NUMERIC_PERIOD];
            }

            PushTextureIDLookup(&Digits, TextureID);
        }

        LetterMin.X = QuarterTile.X;
        LetterMin.Y += QuarterTile.Y;

        DrawMonospaceLineOfText(RenderCommands, &Digits, LetterMin,  
                                QuarterTile, TextCharacterZLayer);
    }

    r32 ScreenWidth = (r32)RenderCommands->ViewportWidth;
    r32 ScreenHeight = (r32)RenderCommands->ViewportHeight;
    s32 RowAmount = (s32)(ScreenHeight/TileSideInPixels)/2 + 2;
    s32 ColumnAmount = (s32)(ScreenWidth/TileSideInPixels)/2 + 2;

    tile_map_position CameraP = GameState->Screen.CameraP;

    gameplay_state_map *GameplayStateMap = GameState->GameplayStateMap;

    v2 ScreenCenter = GameState->Screen.Center;
    r32 MetersToPixels = GameState->Screen.MetersToPixels;
    s32 TileZLayer = 21;
    s32 DefaultEntityZLayer = 20;

// MARK: Render Entities
    for (u32 EntityIndex = 1; 
         EntityIndex < MAX_ENTITIES; 
         EntityIndex++)
    {
        entity SlotTestEntity = GameState->Entities[EntityIndex];

        if (SlotTestEntity.ID == 0)
        {
            continue;
        }

        entity *Entity = TryToGetEntity(GameState->Entities, SlotTestEntity.ID);
 
        if (Entity != nullptr)
        {

        }
    }

    r32 SpriteScaleFactor = (r32)(TileSideInPixels/24.0);

// MARK: Gameplay Selection Grid
    s32 TileBackgroundZLayer = 27;

    screen *Screen = &GameState->Screen;

    s32 MouseCursorZLayer = 0;

#if LEVELEDITOR
    v2 TextRenderSize = V2(TileSideInPixels*0.5f, TileSideInPixels*0.5f);
    v2 BackgroundRenderSize = V2(TextRenderSize.X + 16.0f, TextRenderSize.Y + 16.0f);

#endif

    {
        MouseCursorZLayer = 34;
        DrawMouseCursor(RenderCommands, TextureMap, Input->MousePos, VTileSideInPixels, 
                        MouseCursorZLayer);
    }

    rectangle2 Background = Rectangle2(V2(0.0f, 0.0f), 
                                       V2(RenderCommands->ViewportWidth, RenderCommands->ViewportHeight));
    PushTexturedRectangle(RenderCommands, TextureAtlasTypeTiles, Background, 
                          TextureMap->ColorPalette[39], 
                          1.0f, 0, 0.0f);

    ProcessRenderLayers(RenderCommands);
}

