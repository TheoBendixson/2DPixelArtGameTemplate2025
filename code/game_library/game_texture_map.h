
struct game_texture_map
{
    u32 MissingAsset;
    u32 Alphabet[26];
    u32 AlphabetBlack[26];
    u32 Numbers[11];
    u32 ColonCharacter;
    u32 MouseCursor;
    u32 ColorPalette[48];
};

enum text_character_type
{
    TextCharacterTypeTextureLookup,
    TextCharacterTypeSpace
};

struct text_character
{
    text_character_type Type;
    u32 TextureID;
};

struct text_character_lookup_array
{
    text_character TextCharacters[100];
    u32 Count;
};

#define ALPHA_A 0
#define ALPHA_B 1
#define ALPHA_C 2
#define ALPHA_D 3
#define ALPHA_E 4
#define ALPHA_F 5
#define ALPHA_G 6
#define ALPHA_H 7
#define ALPHA_I 8
#define ALPHA_J 9
#define ALPHA_K 10 
#define ALPHA_L 11 
#define ALPHA_M 12 
#define ALPHA_N 13 
#define ALPHA_O 14 
#define ALPHA_P 15 
#define ALPHA_Q 16 
#define ALPHA_R 17 
#define ALPHA_S 18 
#define ALPHA_T 19 
#define ALPHA_U 20 
#define ALPHA_V 21 
#define ALPHA_W 22 
#define ALPHA_X 23 
#define ALPHA_Y 24 
#define ALPHA_Z 25 

#define NUMERIC_PERIOD  10
