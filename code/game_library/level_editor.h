#define LEVEL_EDITOR_MAX_TILE_MENU_TYPES    40
#define LEVEL_EDITOR_MAX_ENTITY_MENU_TYPES  20 
#define MAX_TILE_BRUSH_SIZE                 5 

enum level_edit_mode
{
    LevelEditModePlayer,
    LevelEditModeGrassTiles,
    LevelEditModeWaterTiles,
    LevelEditModeEdgeTiles,
    LevelEditModeTrees,
    LevelEditModeMoose,
    LevelEditModeFlipMoose,
    LevelEditModeFence,
    LevelEditModeEntityTypes,
    LevelEditModeTileEntityTypes,
    LevelEditModeCartRailEntityTypes,
    LevelEditModeDecorPieces
};

enum cursor_step_mode
{
    CursorStepModeFull,
    CursorStepModeHalf
};

struct level_editor_menu_selection
{
    s32 SelectionCount;
    s32 SelectedIndex; 
};

struct level_editor_tile_selection_menu
{
    u32 TileTypes[LEVEL_EDITOR_MAX_TILE_MENU_TYPES];
    level_editor_menu_selection Selection;
};

struct level_editor_entity_selection_menu
{
    entity_type EntityTypes[LEVEL_EDITOR_MAX_ENTITY_MENU_TYPES];
    level_editor_menu_selection Selection;
};
