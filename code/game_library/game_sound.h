
struct wav_file_header
{
    char Riff[4];
    u32 FileSize;
    char Wave[4];
};

struct wav_chunk_header
{
    char Header[4];
    u32 DataSize;
};

struct wav_format_chunk
{
    u16 FormatType;
    u16 Channels;
    u32 SampleRate;
    u32 ByteRate;
    u16 BlockAlign;
    u16 BitsPerSample;
};

enum loaded_sound_id
{
    SoundID_TestBlip = 0
};

struct loaded_sound
{
    s16 *Samples; // 0 for not loaded.
    s32 NumSamples; // A sample includes all channels.
    s32 NumChannels;
};

// NOTE:   We need these limits to avoid moving the speaker diaphragm off-center too much for too
//         long, which I've heard could damage speakers (e.g. having a loud sound at 0.0001 pitch)
#define MIN_PITCH .1f
#define MAX_PITCH 4.f

struct playing_sound
{
    b8 Occupied;
    u64 Id;
    loaded_sound_id SoundId;
    s32 CurrentSample; // Can be negative for a delayed playback (The delay time is not affected by pitch).
    f32 CurrentSampleFrac;

    b8 Loop;
    b8 StartedPlaying; // NOTE:   Whether it has gone through the mixer function at least once
				       //         (Including delayed sounds having their currentSample advanced)
    b8 KillWhenVolumeReachesZero;
    b8 HasStarted;

    f32 Volume[2]; // >=0. 1 is default. 0=Left, 1=Right
    f32 dVolume[2]; // >=0. In change per second.
    f32 VolumeTarget[2];

    f32 Pitch;
    f32 dPitch; // >=0
    f32 PitchTarget;

    u64 EntityID; // NOTE:   Used for entities to track sounds.
};


#define DEFAULT_MASTER_GAIN .7f

struct game_audio_state
{
    f32 MusicVolume;
    f32 SFXVolume;

    f32 MasterGain;
    f32 MasterGainTarget;
    s32 SamplesPerSecond;

    loaded_sound *LoadedSounds;
    s32 LoadedSoundsCount;
    s32 MaxLoadedSounds;

    playing_sound *PlayingSounds;
    s32 MaxPlayingSounds;
    u64 LastGivenId;
    // NOTE:   The term "sound id" can either refer to the loaded_sound_id enum, or
    //         the u64 unique id assigned to a playing_sound every time it plays
    //         (Although that one is usually named "id" or "playing sound id").
    //         If this is confusing in the code we could be more strict in the naming.
};


struct game_sound_output_buffer 
{
    // NOTE: A sample includes 2 channels.
    u32 SamplesPerSecond;
    s16 *Samples;
    u32 SampleArrayCount;
    u32 SamplesToAdvanceBeforeWriting;
    u32 SamplesToWriteAndAdvance;
    u32 SamplesToWriteExtra;
};
