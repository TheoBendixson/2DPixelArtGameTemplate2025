
void
PushTextureIDLookup(text_character_lookup_array *TextCharacterLookup, u32 TextureID)
{
    text_character Character = {};
    Character.Type = TextCharacterTypeTextureLookup;
    Character.TextureID = TextureID;
    TextCharacterLookup->TextCharacters[TextCharacterLookup->Count++] = Character;
}

void
PushSpaceCharacter(text_character_lookup_array *TextCharacterLookup)
{
    text_character Character = {};
    Character.Type = TextCharacterTypeSpace;
    TextCharacterLookup->TextCharacters[TextCharacterLookup->Count++] = Character;
}

void
PushString(string *String, u32 *Alphabet, text_character_lookup_array *CharacterTextures)
{
    for (s32 CharacterIndex = 0;
         CharacterIndex < String->CharacterCount;
         CharacterIndex += 1)
    {
        char c = String->Characters[CharacterIndex];
        
        u32 LookupIndex = 0;

        if (c == ' ')
        {
            PushSpaceCharacter(CharacterTextures);
        } else
        {
            if (c == 'a')
            {
                LookupIndex = ALPHA_A;
            }
            else if (c == 'b')
            {
                LookupIndex = ALPHA_B;
            }
            else if (c == 'c')
            {
                LookupIndex = ALPHA_C;
            }
            else if (c == 'd')
            {
                LookupIndex = ALPHA_D;
            }
            else if (c == 'e')
            {
                LookupIndex = ALPHA_E;
            }
            else if (c == 'f')
            {
                LookupIndex = ALPHA_F;
            }
            else if (c == 'g')
            {
                LookupIndex = ALPHA_G;
            }
            else if (c == 'h')
            {
                LookupIndex = ALPHA_H;
            }
            else if (c == 'i')
            {
                LookupIndex = ALPHA_I;
            }
            else if (c == 'j')
            {
                LookupIndex = ALPHA_J;
            }
            else if (c == 'k')
            {
                LookupIndex = ALPHA_K;
            }
            else if (c == 'l')
            {
                LookupIndex = ALPHA_L;
            }
            else if (c == 'm')
            {
                LookupIndex = ALPHA_M;
            }
            else if (c == 'n')
            {
                LookupIndex = ALPHA_N;
            }
            else if (c == 'o')
            {
                LookupIndex = ALPHA_O;
            }
            else if (c == 'p')
            {
                LookupIndex = ALPHA_P;
            }
            else if (c == 'q')
            {
                LookupIndex = ALPHA_Q;
            }
            else if (c == 'r')
            {
                LookupIndex = ALPHA_R;
            }
            else if (c == 's')
            {
                LookupIndex = ALPHA_S;
            }
            else if (c == 't')
            {
                LookupIndex = ALPHA_T;
            }
            else if (c == 'u')
            {
                LookupIndex = ALPHA_U;
            }
            else if (c == 'v')
            {
                LookupIndex = ALPHA_V;
            }
            else if (c == 'w')
            {
                LookupIndex = ALPHA_W;
            }
            else if (c == 'x')
            {
                LookupIndex = ALPHA_X;
            }
            else if (c == 'y')
            {
                LookupIndex = ALPHA_Y;
            }
            else if (c == 'z')
            {
                LookupIndex = ALPHA_Z;
            }

            PushTextureIDLookup(CharacterTextures, Alphabet[LookupIndex]);
        }
    }
}

