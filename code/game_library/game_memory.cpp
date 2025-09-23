
void
ClearMemoryPartition(game_memory_partition *Partition)
{
    u8* Byte = (u8 *)Partition->Start;
    for (u32 Index = 0; Index < Partition->Size; 
         Index++)
    {
        *Byte++ = 0;
    }
}

extern "C"
GAME_CLEAR_TRANSIENT_MEMORY(GameClearTransientMemory)
{
    ClearMemoryPartition(TransientStoragePartition);
}

extern "C"
GAME_SETUP_MEMORY(GameSetupMemory)
{
    // Note: (Ted) Sized correctly. Even a little large.
    Memory->GameStateStorageSize = Megabytes(256);

    // NOTE: (Ted)  Sized Correctly
    Memory->SoundsPartition.Size = Megabytes(64);

    // NOTE: (Ted) Sized correctly. Even a little large.
    Memory->RenderCommandsPartition.Size = Megabytes(16);

    Memory->LongtermArenaPartition.Size = Megabytes(32);

    Memory->PermanentStoragePartition.Size = 
        Memory->GameStateStorageSize + 
        Memory->SoundsPartition.Size + 
        Memory->RenderCommandsPartition.Size +
        Memory->LongtermArenaPartition.Size;

    Memory->FileResultPartition.Size = Megabytes(32);

#if LEVELEDITOR
    Memory->LevelEditorSavePartition.Size = Megabytes(32);
    Memory->TransientStoragePartition.Size = Megabytes(96);
#else
    Memory->TransientStoragePartition.Size = Megabytes(64);
#endif
}

extern "C"
GAME_SETUP_MEMORY_PARTITIONS(GameSetupMemoryPartitions)
{
    Memory->SoundsPartition.Start = 
        (u8*)Memory->PermanentStoragePartition.Start + 
        Memory->GameStateStorageSize;

    Memory->RenderCommandsPartition.Start = 
        (u8*)Memory->SoundsPartition.Start + 
        Memory->SoundsPartition.Size;

    Memory->LongtermArenaPartition.Start =
        (u8*)Memory->RenderCommandsPartition.Start +
        Memory->RenderCommandsPartition.Size;

    Memory->FileResultPartition.Start = 
        ((u8 *)Memory->TransientStoragePartition.Start + Megabytes(32));

#if LEVELEDITOR
    Memory->LevelEditorSavePartition.Start =
        ((u8 *)Memory->TransientStoragePartition.Start + Megabytes(64));
#endif
}
