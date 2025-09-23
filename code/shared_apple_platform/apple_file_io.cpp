
NSString *
FilePathInAppBundleResources(char *Filename)
{
    NSBundle *MainBundle = [NSBundle mainBundle];
    NSString *ExecutablePath = [MainBundle executablePath];
    NSString *ExecutableDirectoryPath = [ExecutablePath stringByDeletingLastPathComponent];

#if MACOS
    NSString *ContentsDirectoryPath = [ExecutableDirectoryPath stringByDeletingLastPathComponent];
    NSString *ResourcesPath = [ContentsDirectoryPath stringByAppendingPathComponent: @"Resources"];
#elif IOS
    NSString *ResourcesPath = [NSString stringWithString: ExecutableDirectoryPath];
#endif

    NSString *AppleFilename = [[NSString alloc] initWithCString: Filename encoding: NSUTF8StringEncoding];
    NSString *Filepath = [ResourcesPath stringByAppendingPathComponent: AppleFilename];
    return Filepath;
}

read_file_result 
AppleReadEntireFile(NSString *Filepath)
{
    read_file_result Result = {};

    @autoreleasepool
    {
        NSData *FileData = [[NSFileManager defaultManager] contentsAtPath: Filepath];
        Result.Contents = malloc(FileData.length);
        Result.ContentsSize = (u64)FileData.length;
        memcpy(Result.Contents, FileData.bytes, FileData.length);
    }

    return (Result);
}

PLATFORM_READ_ENTIRE_FILE(PlatformReadEntireFile) 
{
    read_file_result Result = {};

    @autoreleasepool
    {
        NSString *Filepath = FilePathInAppBundleResources(Filename);
        Result = AppleReadEntireFile(Filepath);
    }

    if (Result.ContentsSize > 0)
    {
        u32 MaxCharacters = 200;
        Result.Filename.CharacterCount = 0;
        Result.Filename.CharacterMax = MaxCharacters;
        Result.Filename.Characters = (char *)malloc(MaxCharacters*sizeof(char));
        LoadString(&Result.Filename, Filename);
    } else
    {
        NSLog(@"No contents loaded");
    }

    return(Result);
}

PLATFORM_FREE_FILE_MEMORY(PlatformFreeFileMemory) 
{
    if (Memory) {
        free(Memory);
    }
}

PLATFORM_WRITE_ENTIRE_FILE(PlatformWriteEntireFile)
{
    b32 Result = false;

    @autoreleasepool 
    {
        NSFileManager *FileManager = [NSFileManager defaultManager];
       
#if MACOS
        NSString *UserDataDirectory = BuildGameSupportDirectory(@"CaveFactory");
#elif IOS
        NSString *UserDataDirectory = GetIOSUserDataDirectory();
#endif

        if (UserDataDirectory)
        {
            BOOL IsDirectory;

            if (![FileManager fileExistsAtPath: UserDataDirectory 
                                   isDirectory: &IsDirectory] || IsDirectory)
            {
                NSError *Error = nil;

                if (![FileManager createDirectoryAtPath: UserDataDirectory
                            withIntermediateDirectories: YES
                                             attributes: nil
                                                  error: &Error])
                {
                    NSLog(@"Error creating directory: %@", Error);
                    return false;
                }
            }

            NSString *AppleFilename = [[NSString alloc] initWithCString: Filename encoding: NSUTF8StringEncoding];
            NSString *FilePath = [UserDataDirectory stringByAppendingPathComponent: AppleFilename];

            NSData *FileData = [NSData dataWithBytes: Memory length: FileSize];

            Result = [FileManager createFileAtPath: FilePath
                                          contents: FileData
                                        attributes: nil];
        }
    }

    return(Result);
}
