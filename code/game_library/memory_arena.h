struct memory_arena
{
    memory_index Size;
    u8* Base;
    memory_index Used; 
};

internal void
InitializeArena(memory_arena *Arena, memory_index Size, u8* Base)
{
    Arena->Size = Size;
    Arena->Base = Base;
    Arena->Used = 0; 
}

internal void
InitializeArena(memory_arena *Arena, game_memory_partition *MemoryPartition)
{
    InitializeArena(Arena, MemoryPartition->Size, (u8*)MemoryPartition->Start);
}


#define PushStruct(Arena, type) ((type *)PushSize_(Arena, sizeof(type))) 
#define PushArray(Arena, Count, type) ((type *)PushSize_(Arena, (Count)*sizeof(type))) 

void *
PushSize_(memory_arena *Arena, memory_index Size)
{
    Assert((Arena->Used + Size) <= Arena->Size);
    void *Result = Arena->Base + Arena->Used;
    Arena->Used += Size;
    return (Result);
}
