
struct apple_sound_output 
{
    u32 SamplesPerSecond; 
    u32 BytesPerSample;
    u32 BufferSize;
    u32 TotalSamples;
    u32 PlayCursor;
    void *Data;
};
