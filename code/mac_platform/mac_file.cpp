

// NOTE: (Ted)  For some reason, this is the only way to load a level from a file
//              dialog without any data corruption. I think this is because it bypasses
//              some sort of finder entitlements nonsense.
read_file_result 
MacReadEntireFileFromDialog(char *Filename)
{
    read_file_result Result = {};

    FILE *FileHandle = fopen(Filename, "r+");

    if(FileHandle != NULL)
    {
		fseek(FileHandle, 0, SEEK_END);
		u64 FileSize = ftell(FileHandle);
        if(FileSize)
        {
        	rewind(FileHandle);
            Result.Contents = malloc(FileSize);

            if(Result.Contents)
            {
                u64 BytesRead = fread(Result.Contents, 1, FileSize, FileHandle);
                if(FileSize == BytesRead)
                {
                    Result.ContentsSize = FileSize;

                    u32 MaxCharacters = 200;
                    Result.Filename.CharacterCount = 0;
                    Result.Filename.CharacterMax = MaxCharacters;
                    Result.Filename.Characters = (char *)malloc(MaxCharacters*sizeof(char));
                    LoadString(&Result.Filename, Filename);
                }
                else
                {                    
                    NSLog(@"File loaded size mismatch. FileSize: %llu, BytesRead: %llu",
                          FileSize, BytesRead);

                    if (Result.Contents)
                    {
                        free(Result.Contents);
                    }

                    Result.Contents = 0;
                }
            }
            else
            {
                NSLog(@"Missing Result Contents Pointer from file load.");
            }
        }
        else
        {
            NSLog(@"Missing File Size from file load");
        }

        fclose(FileHandle);
    }
    else
    {
        NSLog(@"Unable to acquire File handle");
    }

    return (Result);
}

b32 MacWriteEntireFile(char *Filename, u64 FileSize, void *Memory)
{
    NSString *Filepath = [[NSString alloc] initWithCString: Filename encoding: NSUTF8StringEncoding];
    NSData *FileData = [NSData dataWithBytes: Memory length: FileSize];
    b32 Written = [[NSFileManager defaultManager] createFileAtPath: Filepath
                                                          contents: FileData
                                                        attributes: nil];

    return (Written);
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

PLATFORM_READ_FILE_FROM_APPLICATION_SUPPORT(PlatformReadFileFromApplicationSupport)
{
    read_file_result Result = {};
    Result.Contents = NULL;
    Result.ContentsSize = 0;

    @autoreleasepool
    {
        NSString *GameSupportDirectory = BuildGameSupportDirectory(@"CaveFactory");
        NSFileManager *FileManager = [NSFileManager defaultManager];
        NSString *MacFilename = [[NSString alloc] initWithCString: Filename encoding: NSUTF8StringEncoding];
        NSString *FilePath = [GameSupportDirectory stringByAppendingPathComponent: MacFilename];
        NSData *FileData = [[NSFileManager defaultManager] contentsAtPath: FilePath];
        Result.Contents = malloc(FileData.length);
        memcpy(Result.Contents, FileData.bytes, FileData.length);
        Result.ContentsSize = (u64)FileData.length;
    }

    return Result;
}
