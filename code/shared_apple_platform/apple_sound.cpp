
OSStatus 
CircularBufferRenderCallback(void *inRefCon,
                             AudioUnitRenderActionFlags *ioActionFlags,
                             const AudioTimeStamp *inTimeStamp,
                             u32 inBusNumber,
                             u32 inNumberFrames,
                             AudioBufferList *ioData) 
{
    apple_sound_output *SoundOutput = (apple_sound_output *)inRefCon;

    u32 BytesToOutput = inNumberFrames * SoundOutput->BytesPerSample; 

    // NOTE: (ted)  Region 1 is the number of bytes up to the end of the sound buffer
    //              If the frames to be rendered causes us to wrap, the remainder goes
    //              into Region 2.
    u32 Region1Size = BytesToOutput;
    u32 Region2Size = 0;

    // NOTE: (ted)  This handles the case where we wrap.
    if (SoundOutput->PlayCursor + BytesToOutput > SoundOutput->BufferSize) 
    {
        // NOTE: (ted)  Region 1 is the distance from the Play Cursor
        //              to the end of the sound buffer, a.k.a. BufferSize
        Region1Size = SoundOutput->BufferSize - SoundOutput->PlayCursor;

        // NOTE: (ted)  Region 2 is whatever is left over.
        Region2Size = BytesToOutput - Region1Size;
    } 

    u8* Channel = (u8*)ioData->mBuffers[0].mData;

    memcpy(Channel, 
           (u8*)SoundOutput->Data + SoundOutput->PlayCursor, 
           Region1Size);

    memcpy(&Channel[Region1Size],
           SoundOutput->Data,
           Region2Size);

    // Finally, move the play cursor
    SoundOutput->PlayCursor = (SoundOutput->PlayCursor + BytesToOutput) % SoundOutput->BufferSize;

    return noErr;
}

void 
AppleInitSound(apple_sound_output *SoundOutput, game_startup_config StartupConfig)
{
    //Create a two second circular buffer 
    SoundOutput->SamplesPerSecond = StartupConfig.SoundSamplesPerSecond; 
    SoundOutput->BytesPerSample = StartupConfig.SoundBytesPerSample; 
    SoundOutput->BufferSize = StartupConfig.SoundBufferSize; 
    SoundOutput->TotalSamples = SoundOutput->BufferSize/SoundOutput->BytesPerSample;
    SoundOutput->Data = malloc(SoundOutput->BufferSize);

    u8* Byte = (u8 *)SoundOutput->Data;

    for (u32 Index = 0; Index < SoundOutput->BufferSize; Index++)
    {
        *Byte++ = 0;
    }

    SoundOutput->PlayCursor = 0;

    AudioComponentInstance AudioUnit;
    AudioComponentDescription Acd;
    Acd.componentType = kAudioUnitType_Output;

#if MACOS
    Acd.componentSubType = kAudioUnitSubType_DefaultOutput;
#elif IOS
    Acd.componentSubType = kAudioUnitSubType_RemoteIO;
#endif

    Acd.componentManufacturer = kAudioUnitManufacturer_Apple;
    Acd.componentFlags = 0;
    Acd.componentFlagsMask = 0;

    AudioComponent outputComponent = AudioComponentFindNext(NULL, 
                                                            &Acd);
    OSStatus status = AudioComponentInstanceNew(outputComponent, 
                                                &AudioUnit);
   
    if (status != noErr) {
        NSLog(@"There was an error setting up sound");
        return;
    }

    AudioStreamBasicDescription audioDescriptor;
    audioDescriptor.mSampleRate = SoundOutput->SamplesPerSecond;
    audioDescriptor.mFormatID = kAudioFormatLinearPCM;
    audioDescriptor.mFormatFlags = kAudioFormatFlagIsSignedInteger | 
                                   kAudioFormatFlagIsPacked; 

    int framesPerPacket = 1;
    int bytesPerFrame = sizeof(s16) * 2;
    audioDescriptor.mFramesPerPacket = framesPerPacket;
    audioDescriptor.mChannelsPerFrame = 2; // Stereo sound
    audioDescriptor.mBitsPerChannel = sizeof(s16) * 8;
    audioDescriptor.mBytesPerFrame = bytesPerFrame;
    audioDescriptor.mBytesPerPacket = framesPerPacket * bytesPerFrame; 

    status = AudioUnitSetProperty(AudioUnit,
                                  kAudioUnitProperty_StreamFormat,
                                  kAudioUnitScope_Input,
                                  0,
                                  &audioDescriptor,
                                  sizeof(audioDescriptor));

    if (status != noErr) {
        NSLog(@"There was an error setting up the audio unit");
        return;
    }

    AURenderCallbackStruct renderCallback;
    renderCallback.inputProc = CircularBufferRenderCallback;
    renderCallback.inputProcRefCon = SoundOutput;

    status = AudioUnitSetProperty(AudioUnit,
                                  kAudioUnitProperty_SetRenderCallback,
                                  kAudioUnitScope_Global,
                                  0,
                                  &renderCallback,
                                  sizeof(renderCallback));

    if (status != noErr) {
        NSLog(@"There was an error setting up the audio unit");
        return;
    }

    status = AudioUnitInitialize(AudioUnit);

    if (status != noErr) {
        NSLog(@"There was an error setting up the audio unit");
        return;
    }

    status = AudioOutputUnitStart(AudioUnit);

    if (status != noErr) {
        NSLog(@"There was an error setting up the audio unit");
        return;
    }
}
