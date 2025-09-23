
struct read_file_result 
{
    void *Contents;
    u64 ContentsSize;
    string Filename;
};

struct sound_file_result
{
    s16 *Samples;
    u32 SampleCount;
};

#define PLATFORM_READ_ENTIRE_FILE(name) read_file_result name(char *Filename)
typedef PLATFORM_READ_ENTIRE_FILE(platform_read_entire_file);

#if MACOS || IOS
#define PLATFORM_READ_FILE_FROM_APPLICATION_SUPPORT(name) read_file_result name(char *Filename)
typedef PLATFORM_READ_FILE_FROM_APPLICATION_SUPPORT(platform_read_file_from_application_support);
#endif 

#define PLATFORM_READ_PNG_FILE(name) read_file_result name(char *Filename)
typedef PLATFORM_READ_PNG_FILE(platform_read_png_file);

#define PLATFORM_FREE_FILE_MEMORY(name) void name(void *Memory)
typedef PLATFORM_FREE_FILE_MEMORY(platform_free_file_memory);

#define PLATFORM_OPEN_FILE_DIALOG(name) void name(void *TransientStorage)
typedef PLATFORM_OPEN_FILE_DIALOG(platform_open_file_dialog);

#define PLATFORM_WRITE_ENTIRE_FILE(name) b32 name(char *Filename, u64 FileSize, void *Memory)
typedef PLATFORM_WRITE_ENTIRE_FILE(platform_write_entire_file);

#define PLATFORM_SAVE_FILE_DIALOG(name) void name(u64 FileSize, void *Memory)
typedef PLATFORM_SAVE_FILE_DIALOG(platform_save_file_dialog);

