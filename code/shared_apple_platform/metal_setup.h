
void
SetupAlphaBlendForRenderPipelineColorAttachment(MTLRenderPipelineColorAttachmentDescriptor *ColorRenderBufferAttachment)
{
    ColorRenderBufferAttachment.blendingEnabled = YES;
    ColorRenderBufferAttachment.rgbBlendOperation = MTLBlendOperationAdd;
    ColorRenderBufferAttachment.alphaBlendOperation = MTLBlendOperationAdd;
    ColorRenderBufferAttachment.sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
    ColorRenderBufferAttachment.sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
    ColorRenderBufferAttachment.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    ColorRenderBufferAttachment.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
}

id<MTLTexture>
SetupMetalTexture(id<MTLDevice> MetalDevice, game_texture_buffer *TextureBuffer)
{
    MTLTextureDescriptor *TextureDescriptor = [[MTLTextureDescriptor alloc] init];
    TextureDescriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
    TextureDescriptor.width = TextureBuffer->TextureWidth;
    TextureDescriptor.height = TextureBuffer->TextureHeight;
    TextureDescriptor.arrayLength = TextureBuffer->TexturesLoaded;
    TextureDescriptor.textureType = MTLTextureType2DArray;
    TextureDescriptor.usage = MTLTextureUsageShaderRead;

    MTLRegion TextureMetalRegion = {
        { 0, 0, 0 },
        { TextureBuffer->TextureWidth, TextureBuffer->TextureHeight, 1 }
    };

    id<MTLTexture> MetalTexture = [[MetalDevice newTextureWithDescriptor: TextureDescriptor] autorelease];

    for (u32 TextureIndex = 0;
         TextureIndex < TextureBuffer->TexturesLoaded;
         TextureIndex++)
    {
        game_texture GameTexture = TextureBuffer->Textures[TextureIndex];

        [MetalTexture replaceRegion: TextureMetalRegion 
                        mipmapLevel: 0
                              slice: TextureIndex
                          withBytes: (void *)GameTexture.Data
                          bytesPerRow: TextureBuffer->TextureWidth*sizeof(u32) 
                        bytesPerImage: 0];
    }

    return MetalTexture;
}

id<MTLRenderPipelineState>
BuildPixelArtPipelineState(id<MTLDevice> MetalDevice, MTLPixelFormat ColorPixelFormat)
{
    NSString *ShaderLibraryFile = [[NSBundle mainBundle] pathForResource: @"Shaders" ofType: @"metallib"];

#if IOS
    NSURL *ShaderLibraryFileURL = [NSURL URLWithString: ShaderLibraryFile];
    id<MTLLibrary> ShaderLibrary = [MetalDevice newLibraryWithURL: ShaderLibraryFileURL error: nil];

#else
    id<MTLLibrary> ShaderLibrary = [MetalDevice newLibraryWithFile: ShaderLibraryFile error: nil];
#endif

    id<MTLFunction> PixelArtVertexShader = [ShaderLibrary newFunctionWithName:@"pixelArtVertexShader"];
    id<MTLFunction> PixelArtFragmentShader = [ShaderLibrary newFunctionWithName:@"pixelArtFragmentShader"];

    MTLRenderPipelineDescriptor *PixelArtPipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    PixelArtPipelineDescriptor.label = @"Pixel Art Shader";
    PixelArtPipelineDescriptor.vertexFunction = PixelArtVertexShader;
    PixelArtPipelineDescriptor.fragmentFunction = PixelArtFragmentShader; 
    MTLRenderPipelineColorAttachmentDescriptor *PixelArtShaderRenderBufferAttachment = PixelArtPipelineDescriptor.colorAttachments[0];
    PixelArtShaderRenderBufferAttachment.pixelFormat = ColorPixelFormat;
    SetupAlphaBlendForRenderPipelineColorAttachment(PixelArtShaderRenderBufferAttachment);

    NSError *error = NULL;
    id<MTLRenderPipelineState> PixelArtPipelineState = [MetalDevice newRenderPipelineStateWithDescriptor: PixelArtPipelineDescriptor
                                                                                                   error: &error];

    if (error != nil)
    {
        [NSException raise: @"Error creating tile texture pipeline state"
                     format:@"%@", error.localizedDescription];
    }

    return PixelArtPipelineState;
}

id<MTLBuffer>
SetupAppleVertexBuffer(id<MTLDevice> MetalDevice, id<MTLCommandQueue> CommandQueue, 
                       game_render_commands *RenderCommands, u32 VertexBufferSize)
{
    id<MTLBuffer> MacGPUVertexBuffer = [MetalDevice newBufferWithLength: VertexBufferSize
                                                                options: MTLResourceStorageModePrivate];

    id<MTLBuffer> MacStagingVertexBuffer = [MetalDevice newBufferWithBytes: RenderCommands->VertexBuffer.Vertices
                                                                    length: VertexBufferSize 
                                                                   options: MTLResourceStorageModeShared];

    id<MTLCommandBuffer> CommandBuffer = [CommandQueue commandBuffer];
    id<MTLBlitCommandEncoder> BlitEncoder = [CommandBuffer blitCommandEncoder];

    [BlitEncoder copyFromBuffer: MacStagingVertexBuffer
                   sourceOffset: 0
                       toBuffer: MacGPUVertexBuffer
              destinationOffset: 0
                           size: MacStagingVertexBuffer.length];

    [BlitEncoder endEncoding];
    [CommandBuffer commit];
    
    return MacGPUVertexBuffer;
}

id<MTLBuffer> 
BuildAppleInstanceUniformsBuffer(id<MTLDevice> metalDevice, texture_draw_command_instance_buffer *InstanceBuffer, u32 InstanceMax)
{
    InstanceBuffer->InstanceMax = InstanceMax;
    u32 InstanceUniformsSize = kAlignedPixelArtInstanceUniformsSize*kMaxInflightBuffers*InstanceBuffer->InstanceMax;

#if IOS
    InstanceBuffer->InstanceUniforms = 
        (texture_draw_command_instance_uniforms *)malloc(InstanceBuffer->InstanceMax*sizeof(texture_draw_command_instance_uniforms));
#else
    InstanceBuffer->InstanceUniforms = 
        (texture_draw_command_instance_uniforms *)mmap(0, InstanceBuffer->InstanceMax*sizeof(texture_draw_command_instance_uniforms),
                                                 PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
#endif

    id<MTLBuffer> InstanceUniformsBuffer = [metalDevice newBufferWithLength: InstanceUniformsSize
                                                                    options: MTLResourceStorageModeShared];
    InstanceUniformsBuffer.label = @"Instance Uniforms Buffer";
    return InstanceUniformsBuffer;
}
