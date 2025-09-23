
struct string
{
    char *Characters;
    s32 CharacterMax;
    s32 CharacterCount;
};

void
LoadString(string *String, char *Characters)
{
    s32 CharacterIndex = 0;

    while(Characters[CharacterIndex] != '\0' &&
          CharacterIndex < String->CharacterMax)
    {
        String->Characters[CharacterIndex] = Characters[CharacterIndex];
        CharacterIndex += 1;
    }

    String->Characters[CharacterIndex] = '\0';
    CharacterIndex += 1;
    String->CharacterCount = (CharacterIndex - 1);
}
