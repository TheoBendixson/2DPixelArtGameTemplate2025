
struct sound_file
{
    loaded_sound_id SoundID;
    char Path[100];
};

void 
LoadSound(game_memory *Memory, game_state *GameState, sound_file *SoundFile, s32 SoundFileCount)
{
    Assert((s32)SoundFile->SoundID >= 0);
    memory_arena *SoundsArena = &GameState->SoundsArena;

    loaded_sound *Sound = &GameState->AudioState.LoadedSounds[(s32)SoundFile->SoundID];
    Assert(Sound->Samples == 0); // NOTE:   Shouldn't be initialized.

    read_file_result File = Memory->PlatformReadEntireFile(SoundFile->Path);

    if (File.ContentsSize > 0)
    {
        wav_file_header *WaveHeader = (wav_file_header *)File.Contents;

        u16 Channels = 0;
        u32 SampleRate = 0;
        u32 ByteRate = 0;
        u16 BitsPerSample = 0;
        u32 DataSize = 0;

        s16 *SoundSource = NULL;

        b32 FormatChunkFound = false;
        b32 DataChunkFound = false;

        u32 ChunkOffset = sizeof(wav_file_header);

        while (!FormatChunkFound ||
               !DataChunkFound)
        {
            wav_chunk_header *ChunkHeader = (wav_chunk_header *)((u8 *)File.Contents + ChunkOffset);

            char *Header = ChunkHeader->Header;

            if (Header[0] == 'f' &&
                Header[1] == 'm' &&
                Header[2] == 't')
            {
                wav_format_chunk *FormatChunk = (wav_format_chunk *)((u8 *)File.Contents + 
                                                                 sizeof(wav_file_header) + 
                                                                 sizeof(wav_chunk_header));
                LITTLE_ENDIAN_TO_HOST(&Channels, &FormatChunk->Channels);
                LITTLE_ENDIAN_TO_HOST(&SampleRate, &FormatChunk->SampleRate);
                LITTLE_ENDIAN_TO_HOST(&ByteRate, &FormatChunk->ByteRate);
                LITTLE_ENDIAN_TO_HOST(&BitsPerSample, &FormatChunk->BitsPerSample);
                FormatChunkFound = true;
            }

            if (Header[0] == 'd' &&
                Header[1] == 'a' &&
                Header[2] == 't' &&
                Header[3] == 'a')
            {
                LITTLE_ENDIAN_TO_HOST(&DataSize, &ChunkHeader->DataSize);
                SoundSource = (s16*)((u8*)File.Contents + ChunkOffset + sizeof(wav_chunk_header));
                DataChunkFound = true;
            }

            ChunkOffset += (sizeof(wav_chunk_header) + ChunkHeader->DataSize);
        }

        // LOOKINTO: Suport mono sounds.
        size_t BytesAfterHeader = (size_t)File.ContentsSize - ((size_t)(u8 *)SoundSource - (size_t)(u8 *)File.Contents);
        s32 NumSamples = (DataSize/Channels)/sizeof(s16);
        b32 Valid = (Channels == 2 && SampleRate == 48000 && BitsPerSample == 16 &&
                 DataSize <= BytesAfterHeader && NumSamples);

        if (Valid)
        {
            Sound->Samples = PushArray(SoundsArena, NumSamples*Channels, s16);
            memcpy(Sound->Samples, SoundSource, NumSamples*Channels*sizeof(s16));

            Sound->NumSamples = NumSamples;
            Sound->NumChannels = Channels;
            Assert(Channels == 2);

            if (!IsLittleEndian())
            {
                // Convert audio data to host endian.
                for (s32 i = 0; i < NumSamples*Channels; i++)
                {
                    u8 *Bytes = (u8 *)&Sound->Samples[i];
                    SWAP(Bytes[0], Bytes[1]);
                }
            }
        }else
        {
            int breakpoint = 3;
        }
    } 

#if WINDOWS
    PlatformFreeMemory(File.Contents);
#elif MACOS || IOS
    Memory->PlatformFreeFileMemory(File.Contents);
#endif
}

void 
BuildSoundFilePath(sound_file *SoundFile, loaded_sound_id SoundID, char *FilePath)
{
    SoundFile->SoundID = SoundID;

    char *Src = FilePath;
    u32 Index = 0;

    while (*Src != '\0')
    {
	    SoundFile->Path[Index++] = *Src++;
    }

    SoundFile->Path[Index++] = '\0';
}

extern "C"
LOAD_SOUNDS(LoadSounds)
{
    game_state *GameState = (game_state *)Memory->PermanentStoragePartition.Start;
    game_audio_state *AudioState = &GameState->AudioState;
    memory_arena *SoundsArena = &GameState->SoundsArena;

    InitializeArena(SoundsArena, &Memory->SoundsPartition);

    AudioState->SamplesPerSecond = 48000;
    AudioState->MaxPlayingSounds = 100;

    sound_file SoundFiles[40];
    u32 SoundFileIndex = 0; 

// MARK: Music
    BuildSoundFilePath(&SoundFiles[SoundFileIndex++], SoundID_TestBlip, "sounds/aimok_48k.wav");

    s32 SoundFileCount = SoundFileIndex;
    AudioState->MaxLoadedSounds = SoundFileCount;

    AudioState->MasterGain = AudioState->MasterGainTarget = DEFAULT_MASTER_GAIN;

    AudioState->LoadedSounds = PushArray(SoundsArena, AudioState->MaxLoadedSounds, loaded_sound);

    AudioState->PlayingSounds = PushArray(SoundsArena, AudioState->MaxPlayingSounds, playing_sound);

    Assert(AudioState->LastGivenId == 0); // NOTE: (Pere)  The memory we didn't touch should be zero.

    for (u32 Index = 0;
	     Index < SoundFileCount;
	     Index++)
    {
	    LoadSound(Memory, GameState, &SoundFiles[Index], SoundFileCount);
    }
}
