
enum axis_flip_mode
{
    AxisFlipModeNone,
    AxisFlipModeX
};

struct spritesheet
{
    u32 *Data;
    u32 TextureWidth;
    u32 TextureHeight;
    u32 TexturesPerRow;
    u32 TexturesPerColumn;
};

struct spritesheet_section
{
    s32 XOffset;
    s32 YOffset;
    u32 SamplingWidth;
    u32 SamplingHeight;
};

struct spritesheet_position
{
    s32 Row;
    s32 Column;
};
