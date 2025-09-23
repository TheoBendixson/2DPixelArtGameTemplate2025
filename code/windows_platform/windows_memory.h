
internal void
VirtualAllocateGameMemoryPartition(game_memory_partition *MemoryPartition)
{
    MemoryPartition->Start = VirtualAlloc(0, MemoryPartition->Size,
                                          MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}
