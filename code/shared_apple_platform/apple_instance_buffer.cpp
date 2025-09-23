
struct apple_instance_buffer_frame_offset
{
    u32 InstanceUniformBufferOffset;
    void *InstanceUniformBufferAddress;
};

apple_instance_buffer_frame_offset
GetInstanceBufferFrameOffset(texture_draw_command_instance_buffer *GameInstanceBuffer, id<MTLBuffer>MacInstanceBuffer,
                             u32 CurrentFrameIndex)
{
    apple_instance_buffer_frame_offset Result = {};
    Result.InstanceUniformBufferOffset = 
        (u32)(kAlignedPixelArtInstanceUniformsSize*CurrentFrameIndex*GameInstanceBuffer->InstanceMax);
    Result.InstanceUniformBufferAddress = 
        ((u8 *)MacInstanceBuffer.contents) + Result.InstanceUniformBufferOffset;
    return Result;
}

u32 
GetTextureLookupIndex(texture_atlas_type TextureAtlasType)
{
    u32 LookupIndex = 0;

    if (TextureAtlasType == TextureAtlasTypeTiles)
    {
        LookupIndex = 0;
    } 

    return LookupIndex;
}
