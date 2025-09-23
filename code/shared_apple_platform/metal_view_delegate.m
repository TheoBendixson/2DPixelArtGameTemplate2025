
#if MACOS
@interface MetalViewDelegate: NSObject
#elif IOS
@interface MetalViewDelegate: NSObject <MTKViewDelegate>
#endif

@property game_memory GameMemory;
@property game_render_commands RenderCommands;
@property (retain) id<MTLRenderPipelineState> PixelArtPipelineState;
@property (retain) id<MTLCommandQueue> CommandQueue;
@property (retain) NSMutableArray *TextureAtlases;
@property (retain) id<MTLBuffer> AppleVertexBuffer;
@property (retain) id<MTLBuffer> InstanceUniformsBuffer;

#if MACOS
@property (retain) CustomMetalView *View;
#elif IOS
@property (retain) id<MTLDevice> MetalDevice;
#endif

- (void)configureMetal;
- (void)setInputPtr: (game_input *)InputPtr;
- (void)setSoundOutputBufferPtr: (game_sound_output_buffer *)GameSoundOutputBufferPtr;
- (void)setAppleSoundOutputPtr:(apple_sound_output *)AppleSoundOutputPtr;

#if MACOS
- (void)setGameControllerPtr: (mac_game_controller *)GameControllerPtr;
- (void)setKeyboardControllerPtr: (mac_game_controller *)KeyboardControllerPtr;
- (void)setTimeBasePtr: (mach_timebase_info_data_t *)TimeBasePtr;
#endif

#if IOS
- (game_texture_map *)getTextureMap;
#endif

@end

bool MacOSIsKeyPressed(CGKeyCode keyCode) 
{
    return CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState, keyCode);
}

@implementation MetalViewDelegate
{
    game_input *_InputPtr;
    game_texture_map *_TextureMapPtr;
    game_sound_output_buffer *_GameSoundOutputBufferPtr;
    apple_sound_output *_AppleSoundOutputPtr;
    dispatch_semaphore_t _frameBoundarySemaphore;
    u32 _currentFrameIndex;

#if STEAMSTORE
    steam_action_handles *_SteamActionHandlesPtr;
#endif

#if MACOS
    mach_timebase_info_data_t *_TimeBasePtr;
    mac_game_controller *_GameControllerPtr;
    mac_game_controller *_KeyboardControllerPtr;
#endif

}

- (void)configureMetal
{
    _frameBoundarySemaphore = dispatch_semaphore_create(kMaxInflightBuffers);
    _currentFrameIndex = 0;
}

#if STEAMSTORE
- (void)setSteamActionHandlesPtr: (steam_action_handles *)SteamActionHandlesPtr
{
    _SteamActionHandlesPtr = SteamActionHandlesPtr;
}
#endif

#if MACOS
- (void)setGameControllerPtr: (mac_game_controller *)GameControllerPtr
{
    _GameControllerPtr = GameControllerPtr;
}

- (void)setKeyboardControllerPtr: (mac_game_controller *)KeyboardControllerPtr
{
    _KeyboardControllerPtr = KeyboardControllerPtr;
}

- (void)setTimeBasePtr: (mach_timebase_info_data_t *)TimeBasePtr
{
    _TimeBasePtr = TimeBasePtr;
}
#endif

- (void)setInputPtr: (game_input *)InputPtr
{
    _InputPtr = InputPtr;
}

- (void)setSoundOutputBufferPtr: (game_sound_output_buffer *)GameSoundOutputBufferPtr
{
    _GameSoundOutputBufferPtr = GameSoundOutputBufferPtr;
}

- (void)setAppleSoundOutputPtr:(apple_sound_output *)AppleSoundOutputPtr
{
    _AppleSoundOutputPtr = AppleSoundOutputPtr;
}

- (void) setTextureMapPtr:(game_texture_map *)TextureMapPtr
{
    _TextureMapPtr = TextureMapPtr;
}

#if IOS
- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size
{
    game_render_commands *RenderCommandsPtr = &_RenderCommands;
    RenderCommandsPtr->ViewportWidth = size.width;
    RenderCommandsPtr->ViewportHeight = size.height;
    game_memory *GameMemoryPtr = &_GameMemory;
    game_texture_map *GameTextureMapPtr = _TextureMapPtr;

    r32 Scale = (r32)[UIScreen mainScreen].scale;
    r32 LeftInset = (r32)view.safeAreaInsets.left*Scale;
    GameSetupTouchControls(RenderCommandsPtr, GameMemoryPtr, 
                           GameTextureMapPtr, LeftInset);
}

- (game_texture_map *)getTextureMap
{
    return _TextureMapPtr;
}

#endif

#if MACOS
- (void)drawInMTKView:(CustomMetalView *)view 
#elif IOS
- (void)drawInMTKView:(nonnull MTKView *)view
#endif
{
    dispatch_semaphore_wait(_frameBoundarySemaphore, DISPATCH_TIME_FOREVER);

#if STEAMSTORE
    SteamAPI_RunCallbacks();

    if (_SteamActionHandlesPtr->SteamInputInitialized)
    {
        RunSteamInput(&_InputPtr->Controller, _SteamActionHandlesPtr);
    }
#endif

    NSArray<GCController *> *GameControllers = [GCController controllers];

    for (GCController *Controller in GameControllers)
    {
        if(Controller.extendedGamepad)
        {
            GCExtendedGamepad *ExtendedGamepad = Controller.extendedGamepad;
            game_controller_input *CrossPlatformController = &_InputPtr->Controller;
            ProcessButton(&CrossPlatformController->Right, ExtendedGamepad.dpad.right.pressed);
            ProcessButton(&CrossPlatformController->Left, ExtendedGamepad.dpad.left.pressed);
            ProcessButton(&CrossPlatformController->Up, ExtendedGamepad.dpad.up.pressed);
            ProcessButton(&CrossPlatformController->Down, ExtendedGamepad.dpad.down.pressed);
            ProcessButton(&CrossPlatformController->B, ExtendedGamepad.buttonB.pressed);
            ProcessButton(&CrossPlatformController->Y, ExtendedGamepad.buttonY.pressed);
            ProcessButton(&CrossPlatformController->X, ExtendedGamepad.buttonX.pressed);
            ProcessButton(&CrossPlatformController->Select, ExtendedGamepad.buttonOptions.pressed);
            ProcessButton(&CrossPlatformController->A, ExtendedGamepad.buttonA.pressed);
            ProcessButton(&CrossPlatformController->RightShoulder1, 
                          ExtendedGamepad.rightShoulder.pressed);
            ProcessButton(&CrossPlatformController->LeftShoulder1, 
                          ExtendedGamepad.leftShoulder.pressed);

            b32 R2Pressed = false;

            if (ExtendedGamepad.rightTrigger.value > 0.3f)
            {
                R2Pressed = true;
            }

            ProcessButton(&CrossPlatformController->RightShoulder2, R2Pressed);

            b32 L2Pressed = false;

            if (ExtendedGamepad.leftTrigger.value > 0.3f)
            {
                L2Pressed = true;
            }

            ProcessButton(&CrossPlatformController->LeftShoulder2, L2Pressed);

            v2 RightThumb = V2(ExtendedGamepad.rightThumbstick.xAxis.value,
                               ExtendedGamepad.rightThumbstick.yAxis.value);
            CrossPlatformController->RightThumb = RightThumb;
        }
    }

    game_render_commands *RenderCommandsPtr = &_RenderCommands;
    _currentFrameIndex = (_currentFrameIndex + 1) % kMaxInflightBuffers;
    RenderCommandsPtr->FrameIndex = (u32)_currentFrameIndex;

    apple_instance_buffer_frame_offset InstanceBufferFrameOffset = 
        GetInstanceBufferFrameOffset(&RenderCommandsPtr->InstanceBuffer, self.InstanceUniformsBuffer,
                                     _currentFrameIndex);

    game_memory *GameMemoryPtr = &_GameMemory;

    NSPoint mouseLocation = [NSEvent mouseLocation];

    _InputPtr->MousePos.X = (r32)(mouseLocation.x*2.0f);

    NSRect ScreenRect = [[NSScreen mainScreen] frame];
    r32 ScreenHeight = (r32)ScreenRect.size.height;

    _InputPtr->MousePos.Y = (r32)(ScreenHeight - mouseLocation.y)*2.0f;

    ProcessButton(&_InputPtr->Keyboard.Left, MacOSIsKeyPressed(LeftArrowKeyCode));
    ProcessButton(&_InputPtr->Keyboard.Right, MacOSIsKeyPressed(RightArrowKeyCode));
    ProcessButton(&_InputPtr->Keyboard.Down, MacOSIsKeyPressed(DownArrowKeyCode));
    ProcessButton(&_InputPtr->Keyboard.Up, MacOSIsKeyPressed(UpArrowKeyCode));
    ProcessButton(&_InputPtr->Keyboard.Space, MacOSIsKeyPressed(SpacebarKeyCode));
    ProcessButton(&_InputPtr->Keyboard.Enter, MacOSIsKeyPressed(ReturnKeyCode));
    ProcessButton(&_InputPtr->Keyboard.Escape, MacOSIsKeyPressed(EscapeKeyCode));

// MARK: Letters
    ProcessButton(&_InputPtr->Keyboard.Letters['W' - 'A'], MacOSIsKeyPressed(WKeyCode));
    ProcessButton(&_InputPtr->Keyboard.Letters['A' - 'A'], MacOSIsKeyPressed(AKeyCode));
    ProcessButton(&_InputPtr->Keyboard.Letters['S' - 'A'], MacOSIsKeyPressed(SKeyCode));
    ProcessButton(&_InputPtr->Keyboard.Letters['D' - 'A'], MacOSIsKeyPressed(DKeyCode));
    ProcessButton(&_InputPtr->Keyboard.Letters['F' - 'A'], MacOSIsKeyPressed(FKeyCode));
    ProcessButton(&_InputPtr->Keyboard.Letters['R' - 'A'], MacOSIsKeyPressed(RKeyCode));
    ProcessButton(&_InputPtr->Keyboard.Letters['L' - 'A'], MacOSIsKeyPressed(LKeyCode));
    ProcessButton(&_InputPtr->Keyboard.Letters['K' - 'A'], MacOSIsKeyPressed(KKeyCode));
    ProcessButton(&_InputPtr->Keyboard.Letters['E' - 'A'], MacOSIsKeyPressed(EKeyCode));
    ProcessButton(&_InputPtr->Keyboard.Letters['T' - 'A'], MacOSIsKeyPressed(TKeyCode));
    ProcessButton(&_InputPtr->Keyboard.Letters['Z' - 'A'], MacOSIsKeyPressed(ZKeyCode));
    ProcessButton(&_InputPtr->Keyboard.Letters['Y' - 'A'], MacOSIsKeyPressed(YKeyCode));
    ProcessButton(&_InputPtr->Keyboard.Letters['M' - 'A'], MacOSIsKeyPressed(MKeyCode));

// MARK: Function Keys
    ProcessButton(&_InputPtr->Keyboard.F[1], MacOSIsKeyPressed(F1KeyCode));
    ProcessButton(&_InputPtr->Keyboard.F[2], MacOSIsKeyPressed(F2KeyCode));
    ProcessButton(&_InputPtr->Keyboard.F[3], MacOSIsKeyPressed(F3KeyCode));
    ProcessButton(&_InputPtr->Keyboard.F[4], MacOSIsKeyPressed(F4KeyCode));
    ProcessButton(&_InputPtr->Keyboard.F[5], MacOSIsKeyPressed(F5KeyCode));
    ProcessButton(&_InputPtr->Keyboard.F[6], MacOSIsKeyPressed(F6KeyCode));
    ProcessButton(&_InputPtr->Keyboard.F[7], MacOSIsKeyPressed(F7KeyCode));
    ProcessButton(&_InputPtr->Keyboard.F[8], MacOSIsKeyPressed(F8KeyCode));

// MARK: Modifier Keys
    s32 LeftShiftKeycode = 56;
    b32 AnyShiftIsPressed = MacOSIsKeyPressed(LeftShiftKeycode);

    if (!AnyShiftIsPressed)
    {
        s32 RightShiftKeycode = 60;
        AnyShiftIsPressed = MacOSIsKeyPressed(RightShiftKeycode);
    }

    ProcessButton(&_InputPtr->Keyboard.Shift, AnyShiftIsPressed);

    s32 LeftControlKeycode = 59;
    b32 AnyControlIsPressed = MacOSIsKeyPressed(LeftControlKeycode);

    if (!AnyControlIsPressed)
    {
        s32 RightControlKeycode = 62;
        AnyControlIsPressed = MacOSIsKeyPressed(RightControlKeycode);
    }

    ProcessButton(&_InputPtr->Keyboard.Control, AnyControlIsPressed);

// MARK: Number Keys
    ProcessButton(&_InputPtr->Keyboard.Numbers[1], MacOSIsKeyPressed(OneKeyCode));
    ProcessButton(&_InputPtr->Keyboard.Numbers[2], MacOSIsKeyPressed(TwoKeyCode));

    BOOL leftDown  = [NSEvent pressedMouseButtons] & (1 << 0);
    BOOL rightDown = [NSEvent pressedMouseButtons] & (1 << 1);
    ProcessButton(&_InputPtr->MouseButtons[0], leftDown);
    ProcessButton(&_InputPtr->MouseButtons[2], rightDown);

    _InputPtr->dtForFrame = 1.0f/60.0f;

    GameUpdate(GameMemoryPtr, _InputPtr, RenderCommandsPtr);

    ResetInput(_InputPtr);

    GameRender(GameMemoryPtr, _TextureMapPtr,
               _InputPtr, RenderCommandsPtr); 

    game_state *GameState = (game_state *)GameMemoryPtr->PermanentStoragePartition.Start;

    if (GameState->SoundAndGraphicsLoaded)
    {
        // LOOKINTO: (Ted)  Make this match Windows.
        apple_sound_output AppleSoundOutput = *_AppleSoundOutputPtr;

        local_persist u32 RunningSampleIndex = 0;
        u32 SamplesPerFrameUpdate = AppleSoundOutput.SamplesPerSecond/60; 
        u32 FramesAhead = 3;
        u32 DesiredFrameBytesToWrite = SamplesPerFrameUpdate*FramesAhead*sizeof(s16)*2;

        u32 TargetCursor = (AppleSoundOutput.PlayCursor + DesiredFrameBytesToWrite)%AppleSoundOutput.BufferSize;

        u32 ByteToLock = (RunningSampleIndex*AppleSoundOutput.BytesPerSample) % AppleSoundOutput.BufferSize; 
        u32 BytesToWrite;

         if (ByteToLock > TargetCursor) {
            // NOTE: (ted)  Play Cursor wrapped.

            // Bytes to the end of the circular buffer.
            BytesToWrite = (AppleSoundOutput.BufferSize - ByteToLock);

            // Bytes up to the target cursor.
            BytesToWrite += TargetCursor;
        } else {
            BytesToWrite = TargetCursor - ByteToLock;
        }

        // NOTE: (Ted)  This is where we can calculate the number of sound samples to write
        //              to the game_sound_output_buffer
        _GameSoundOutputBufferPtr->SamplesToAdvanceBeforeWriting = 0;
        _GameSoundOutputBufferPtr->SamplesToWriteAndAdvance = (BytesToWrite/AppleSoundOutput.BytesPerSample);
        _GameSoundOutputBufferPtr->SamplesToWriteExtra = 4*(BytesToWrite/AppleSoundOutput.BytesPerSample);

        GameGetSoundSamples(&_GameMemory, _GameSoundOutputBufferPtr, 
                            _InputPtr);

        s16* Src = _GameSoundOutputBufferPtr->Samples;

        void *Region1 = (u8*)AppleSoundOutput.Data + ByteToLock;
        u32 Region1Size = BytesToWrite;
        
        if (Region1Size + ByteToLock > AppleSoundOutput.BufferSize) {
            Region1Size = AppleSoundOutput.BufferSize - ByteToLock;
        }

        void *Region2 = AppleSoundOutput.Data;
        u32 Region2Size = BytesToWrite - Region1Size;

        u32 Region1SampleCount = Region1Size/AppleSoundOutput.BytesPerSample;
        s16* SampleOut = (s16*)Region1;

        for (u32 SampleIndex = 0;
             SampleIndex < Region1SampleCount;
             ++SampleIndex) 
        {
            *SampleOut++ = *Src++;
            *SampleOut++ = *Src++;
            RunningSampleIndex++;
        }

        u32 Region2SampleCount = Region2Size/AppleSoundOutput.BytesPerSample;
        SampleOut = (s16*)Region2;
       
        for (u32 SampleIndex = 0;
             SampleIndex < Region2SampleCount;
             ++SampleIndex) 
        {
            *SampleOut++ = *Src++;
            *SampleOut++ = *Src++;
            RunningSampleIndex++;
        }
    }

    NSUInteger Width = (NSUInteger)(RenderCommandsPtr->ViewportWidth);
    NSUInteger Height = (NSUInteger)(RenderCommandsPtr->ViewportHeight);
    MTLViewport Viewport = (MTLViewport){0.0, 0.0, (r64)Width, (r64)Height, -1.0, 1.0 };

    @autoreleasepool 
    {
        id<MTLCommandBuffer> AppleCommandBuffer = [[self CommandQueue] commandBuffer];

#if MACOS
        MTLRenderPassDescriptor *RenderPassDescriptor = 
            [MTLRenderPassDescriptor renderPassDescriptor];
        id<CAMetalDrawable> NextDrawable = [view.metalLayer nextDrawable];
        RenderPassDescriptor.colorAttachments[0].texture = NextDrawable.texture;
#elif IOS
        MTLRenderPassDescriptor *RenderPassDescriptor = view.currentRenderPassDescriptor; 
        id<CAMetalDrawable> NextDrawable = view.currentDrawable;
#endif

        RenderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;

        float_color ClearColor = RenderCommandsPtr->ClearColor;
        MTLClearColor MetalClearColor = MTLClearColorMake(ClearColor.Red, ClearColor.Green, 
                                                          ClearColor.Blue, ClearColor.Alpha);
        RenderPassDescriptor.colorAttachments[0].clearColor = MetalClearColor;

        vector_uint2 ViewportSize = { (u32)RenderCommandsPtr->ViewportWidth, 
                                      (u32)RenderCommandsPtr->ViewportHeight };

        id<MTLRenderCommandEncoder> RenderEncoder = 
            [AppleCommandBuffer renderCommandEncoderWithDescriptor: RenderPassDescriptor];
        RenderEncoder.label = @"RenderEncoder";

        [RenderEncoder setViewport: Viewport];
        [RenderEncoder setRenderPipelineState: [self PixelArtPipelineState]];

        [RenderEncoder setVertexBuffer: [self AppleVertexBuffer]
                                offset: BufferIndexVertices 
                               atIndex: 0];

        [RenderEncoder setVertexBytes: &ViewportSize
                               length: sizeof(ViewportSize)
                              atIndex: BufferIndexViewportSize];

        id<MTLTexture> MetalTexture = nil; 
        MacTextureSize TextureSize = {};

        ResetDrawCommandBuffersBeforePlatformRender(RenderCommandsPtr);

        u32 VerticesInAQuad = 6;
        u32 PreviousInstanceCount = 0;
        u32 InstancesToDraw = 0;

        game_texture_draw_command_buffer *DrawCommandsBuffer = 
            &RenderCommandsPtr->DrawCommandsBuffer;
        texture_atlas_type CurrentTextureAtlasType = 
            DrawCommandsBuffer->BottomLayers[0].Commands[0].TextureAtlasType;

        for (s32 LayerIndex = 0;
             LayerIndex < BOTTOM_LAYER_COUNT;
             LayerIndex += 1)
        {
            render_commands_array *CommandsArray = &DrawCommandsBuffer->BottomLayers[LayerIndex];

            for (s32 CommandIndex = 0;
                 CommandIndex < CommandsArray->CommandCount;
                 CommandIndex += 1)
            {
                game_texture_draw_command Command = CommandsArray->Commands[CommandIndex];

                if (Command.TextureAtlasType != CurrentTextureAtlasType)
                {
                    u32 LookupIndex = GetTextureLookupIndex(CurrentTextureAtlasType);

                    apple_instance_buffer_frame_offset InstanceFrameOffset =
                        GetInstanceBufferFrameOffset(&RenderCommandsPtr->InstanceBuffer, self.InstanceUniformsBuffer,
                                                     _currentFrameIndex);

                    [RenderEncoder setVertexBuffer: self.InstanceUniformsBuffer
                                            offset: InstanceFrameOffset.InstanceUniformBufferOffset
                                           atIndex: BufferIndexPerInstanceUniforms];
                    MetalTexture = [self.TextureAtlases objectAtIndex: LookupIndex];
                    TextureSize.Width = MetalTexture.width;
                    TextureSize.Height = MetalTexture.height;

                    [RenderEncoder setVertexBytes: &TextureSize
                                           length: sizeof(TextureSize)
                                          atIndex: BufferIndexTextureSize];

                    [RenderEncoder setFragmentTexture: MetalTexture atIndex: 0];

                    texture_draw_command_instance_uniforms *Uniforms = 
                        RenderCommandsPtr->InstanceBuffer.InstanceUniforms;
                    texture_draw_command_instance_uniforms * MacInstanceUniforms = 
                        (texture_draw_command_instance_uniforms *)InstanceFrameOffset.InstanceUniformBufferAddress;

                    u32 LastInstanceIndex = PreviousInstanceCount + InstancesToDraw;

                    for (u32 InstanceIndex = PreviousInstanceCount;
                         InstanceIndex < LastInstanceIndex;
                         InstanceIndex++)
                    {
                        texture_draw_command_instance_uniforms InstanceUniforms = Uniforms[InstanceIndex];
                        MacInstanceUniforms[InstanceIndex] = InstanceUniforms;
                    }

                    [RenderEncoder drawPrimitives: MTLPrimitiveTypeTriangle
                                      vertexStart: 0
                                      vertexCount: 6
                                    instanceCount: InstancesToDraw 
                                     baseInstance: PreviousInstanceCount];

                    PreviousInstanceCount += InstancesToDraw;
                    InstancesToDraw = 0;
                }

                CurrentTextureAtlasType = Command.TextureAtlasType;
                InstancesToDraw += 1;
            }
        }

        if (InstancesToDraw > 0)
        {
            u32 LookupIndex = GetTextureLookupIndex(CurrentTextureAtlasType);

            apple_instance_buffer_frame_offset InstanceFrameOffset =
                GetInstanceBufferFrameOffset(&RenderCommandsPtr->InstanceBuffer, self.InstanceUniformsBuffer,
                                             _currentFrameIndex);

            [RenderEncoder setVertexBuffer: self.InstanceUniformsBuffer
                                    offset: InstanceFrameOffset.InstanceUniformBufferOffset
                                   atIndex: BufferIndexPerInstanceUniforms];
            MetalTexture = [self.TextureAtlases objectAtIndex: LookupIndex];
            TextureSize.Width = MetalTexture.width;
            TextureSize.Height = MetalTexture.height;

            [RenderEncoder setVertexBytes: &TextureSize
                                   length: sizeof(TextureSize)
                                  atIndex: BufferIndexTextureSize];

            [RenderEncoder setFragmentTexture: MetalTexture atIndex: 0];

            texture_draw_command_instance_uniforms *Uniforms = 
                RenderCommandsPtr->InstanceBuffer.InstanceUniforms;
            texture_draw_command_instance_uniforms * MacInstanceUniforms = 
                (texture_draw_command_instance_uniforms *)InstanceFrameOffset.InstanceUniformBufferAddress;

            u32 LastInstanceIndex = PreviousInstanceCount + InstancesToDraw;

            for (u32 InstanceIndex = PreviousInstanceCount;
                 InstanceIndex < LastInstanceIndex;
                 InstanceIndex++)
            {
                texture_draw_command_instance_uniforms InstanceUniforms = Uniforms[InstanceIndex];
                MacInstanceUniforms[InstanceIndex] = InstanceUniforms;
            }

            [RenderEncoder drawPrimitives: MTLPrimitiveTypeTriangle
                              vertexStart: 0
                              vertexCount: 6
                            instanceCount: InstancesToDraw 
                             baseInstance: PreviousInstanceCount];

            PreviousInstanceCount += InstancesToDraw;
        }

        [RenderEncoder endEncoding];

        // Schedule a present once the framebuffer is complete using the current drawable
        [AppleCommandBuffer presentDrawable: NextDrawable];

        __block dispatch_semaphore_t semaphore = _frameBoundarySemaphore;
        [AppleCommandBuffer addCompletedHandler:^(id<MTLCommandBuffer> commandBuffer) {
            dispatch_semaphore_signal(semaphore);
        }];

        [AppleCommandBuffer commit];
    }
}

@end
