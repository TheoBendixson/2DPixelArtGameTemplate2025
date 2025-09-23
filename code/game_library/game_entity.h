
enum entity_type: u8
{
    EntityType_Null = 0,
};

struct entity
{
    u64 ID;
    u32 BehaviorFlags;
    entity_type Type;
};

