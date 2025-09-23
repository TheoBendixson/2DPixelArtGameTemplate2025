
struct game_memory_partition
{
    void *Start;
    u64 Size;
};

struct game_memory 
{
    b32 IsInitialized;
    
    game_memory_partition PermanentStoragePartition;

    // NOTE: (Ted)  These three are managed in the permanent
    //              storage partition.

    u64 GameStateStorageSize;
    game_memory_partition SoundsPartition;
    game_memory_partition RenderCommandsPartition;
    game_memory_partition TransientStoragePartition;
    game_memory_partition FileResultPartition;
    game_memory_partition LongtermArenaPartition;

#if LEVELEDITOR
    game_memory_partition LevelEditorSavePartition;
#endif

    platform_read_entire_file *PlatformReadEntireFile;
    platform_free_file_memory *PlatformFreeFileMemory;
    platform_read_png_file *PlatformReadPNGFile;
    platform_write_entire_file *PlatformWriteEntireFile;
    platform_save_file_dialog *PlatformSaveFileDialog;
    platform_open_file_dialog *PlatformOpenFileDialog;
    platform_log_message *PlatformLogMessage;
    platform_unlock_achievement *PlatformUnlockAchievement;
    platform_quit_game *PlatformQuitGame;

#if MACOS || IOS
    platform_read_file_from_application_support *PlatformReadFileFromApplicationSupport;
#endif 
};
