
struct gameplay_state
{
    u64 EntityID;
};

struct gameplay_state_map
{
    s32 CountX;
    s32 CountY;
    gameplay_state *Tiles;
};
