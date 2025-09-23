
#include <windows.h>
#include <stdio.h>

#define MINIMP3_IMPLEMENTATION
#include "../game_library/minimp3.h"

#include "../game_library/base_types.h"
#include "../game_library/game_sound.h"

global_variable mp3dec_t MP3D;

int CALLBACK
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{
    mp3dec_init(&MP3D);

    HANDLE FileHandle = CreateFileA("background.mp3", GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        if (GetFileSizeEx(FileHandle, &FileSize))
        {
            u32 FileSize32 = (u32)FileSize.QuadPart;
            void *FileMemory = VirtualAlloc(0, FileSize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

            DWORD BytesRead = 0;

            if (ReadFile(FileHandle, FileMemory, FileSize32, &BytesRead, 0))
            {
                s16 *SoundScratchMemory = (s16 *)VirtualAlloc(0, Megabytes(50), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

                mp3dec_frame_info_t FrameInfo;
                s16 PCM[MINIMP3_MAX_SAMPLES_PER_FRAME];
                u8 *MP3InputBuffer = (u8 *)FileMemory;

                u32 MP3InputBufferSize = FileSize32;

                s16 FrameSampleCount = 0;

                u32 TotalSamples = 0;

                do {
                    FrameSampleCount = mp3dec_decode_frame(&MP3D, MP3InputBuffer, MP3InputBufferSize, PCM, &FrameInfo);

                    if (FrameInfo.frame_bytes > 0)
                    {
                        MP3InputBuffer += FrameInfo.frame_bytes;
                        MP3InputBufferSize -= FrameInfo.frame_bytes;
                    }

                    u32 FrameSamplesIncludingChannels = FrameSampleCount*2;
                    
                    for (u32 SampleIndex = 0;
                         SampleIndex < FrameSamplesIncludingChannels;
                         SampleIndex++)
                    {
                        *SoundScratchMemory++ = PCM[SampleIndex]; 
                    }

                    TotalSamples += FrameSamplesIncludingChannels;

                } while (FrameSampleCount > 0 && 
                         FrameInfo.frame_bytes > 0);

                if (TotalSamples > 0)
                {
                    printf("There is more than one sample");
                } else
                {
                    printf("No samples loaded");
                }

            } else
            {
                printf("Unable to load file");
            }
        }
    }


    BOOL CloseHandle(
      HANDLE hObject
    );

    return 0;
}
