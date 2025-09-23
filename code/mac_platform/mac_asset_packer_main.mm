// Mooselutions Asset Packer
// By Ted Bendixson

#import <AppKit/AppKit.h>

#import "mac_asset_packer_main.h"

NSString *
FilePathInAppBundleResources(char *Filename)
{
    NSBundle *MainBundle = [NSBundle mainBundle];
    NSString *ExecutablePath = [MainBundle executablePath];
    NSString *ExecutableDirectoryPath = [ExecutablePath stringByDeletingLastPathComponent];
    NSString *ContentsDirectoryPath = [ExecutableDirectoryPath stringByDeletingLastPathComponent];
    NSString *ResourcesPath = [ContentsDirectoryPath stringByAppendingPathComponent: @"Resources"];
    NSString *MacFilename = [[NSString alloc] initWithCString: Filename encoding: NSUTF8StringEncoding];
    NSString *Filepath = [ResourcesPath stringByAppendingPathComponent: MacFilename];
    return Filepath;
}

PLATFORM_WRITE_ENTIRE_FILE(PlatformWriteEntireFile)
{
    b32 Result = false;

    NSString *Filepath = FilePathInAppBundleResources(Filename);
    NSData *FileData = [NSData dataWithBytes: Memory length: FileSize];

    NSFileManager *FileManager = [NSFileManager defaultManager];
    Result = [FileManager createFileAtPath: Filepath
                                  contents: FileData
                                attributes: nil];
    
    return Result;
}

PLATFORM_READ_PNG_FILE(PlatformReadPNGFile)
{
    NSString *Filepath = FilePathInAppBundleResources(Filename);
    char *SandboxFilename = (char *)[Filepath cStringUsingEncoding: NSUTF8StringEncoding];
    read_file_result Result = {};

    int X,Y,N;
    u32 *ImageData = (u32 *)stbi_load(SandboxFilename, &X, &Y, &N, 0);

    if (X > 0 && Y > 0 && ImageData != NULL)
    {
        Result.Contents = ImageData;
        Result.ContentsSize = X*Y*sizeof(u32); 
    }

    return Result;
}

// LOOKINTO: (Ted)  Consider having this terminate with an error as the reason
void PlatformTerminateApp()
{
    [NSApp terminate: nil];
}

#import "../asset_packer/asset_packer_main.cpp"

int main(int argc, const char * argv[]) 
{
    NSLog(@"Running Mooselutions Asset Packer");
    PackGameAssetsAndSaveToFile();
    NSLog(@"Finished Running Mooselutions Asset Packer");
    return (0);
}
