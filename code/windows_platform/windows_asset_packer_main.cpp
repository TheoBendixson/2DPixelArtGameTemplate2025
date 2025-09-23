#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// NOTE: (Pere)  @CopyPaste from windows_main.cpp
void FatalError(const char* message)
{
    MessageBoxA(NULL, message, "Error", MB_ICONEXCLAMATION);
	ExitProcess(0);
}

#include "windows_asset_packer_main.h"

PLATFORM_READ_PNG_FILE(PlatformReadPNGFile)
{
    read_file_result Result = {};

    int X,Y,N;
    u32 *ImageData = (u32 *)stbi_load(Filename, &X, &Y, &N, 0);

    if (X > 0 && Y > 0 && ImageData != NULL)
    {
        Result.Contents = ImageData;
        Result.ContentsSize = X*Y*sizeof(u32); 
    }

    return Result;
}

PLATFORM_WRITE_ENTIRE_FILE(PlatformWriteEntireFile)
{
    b32 FileWritten = false;

    HANDLE FileHandle = CreateFileA(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD BytesWritten;

        if (WriteFile(FileHandle, Memory, FileSize, &BytesWritten, 0))
        {
            FileWritten = true;
        } 

        CloseHandle(FileHandle);
    }

    return FileWritten;
}

void PlatformTerminateApp()
{
    ExitProcess(1);
}

#include "../asset_packer/asset_packer_main.cpp"

int main()
{
    PackGameAssetsAndSaveToFile();
    return(0);
}
