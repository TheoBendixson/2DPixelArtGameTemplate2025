
void
DrawWrappedText(game_render_commands *RenderCommands, text_character_lookup_array *Text,
                v2 StartLeftTop, r32 TextBoxWidth, r32 CharacterWidth, r32 SpacingWidth,
                s32 ZLayer)
{
    s32 RunningCharacterCount = 0;
    text_character *CurrentCharacter = &Text->TextCharacters[RunningCharacterCount];

    v2 LineLeftTop = StartLeftTop;

    b32 TextExitCondition = false;

    while (!TextExitCondition)
    {
        LineLeftTop.X = StartLeftTop.X;
        s32 LineCharacterStart = RunningCharacterCount;

        r32 RunningTextWidth = 0.0f;

        b32 LineExitCondition = false;

        while(!LineExitCondition)
        {
            s32 WordCharacterCount = 0; 
            s32 StartOfCurrentWord = RunningCharacterCount;
            CurrentCharacter = &Text->TextCharacters[RunningCharacterCount];

            while (CurrentCharacter->Type != TextCharacterTypeSpace &&
                   RunningCharacterCount < Text->Count)
            {
                WordCharacterCount += 1;
                RunningCharacterCount += 1;
                CurrentCharacter += 1;
            }

            r32 WordPlusSpacingWidth = (WordCharacterCount + 1)*CharacterWidth +
                                       (WordCharacterCount - 1)*SpacingWidth;

            r32 ProposedRunningTextWidth = WordPlusSpacingWidth + RunningTextWidth;

            if (ProposedRunningTextWidth > TextBoxWidth)
            {
                RunningCharacterCount = StartOfCurrentWord;
                LineExitCondition = true;
            } else
            {
                RunningCharacterCount += 1;
                RunningTextWidth += WordPlusSpacingWidth;
            }

            if (RunningCharacterCount >= Text->Count)
            {
                RunningCharacterCount = Text->Count;
                TextExitCondition = true;
            }
        }

        v2 LetterMin = LineLeftTop;
        v2 LetterMax = LetterMin;
        LetterMax.X += CharacterWidth;
        LetterMax.Y += CharacterWidth;

        for (u32 CharacterIndex = LineCharacterStart;
             CharacterIndex < RunningCharacterCount;
             CharacterIndex++)
        {
            text_character TextCharacter = Text->TextCharacters[CharacterIndex];

            if (TextCharacter.Type == TextCharacterTypeTextureLookup)
            {
                PushTexturedRectangle(RenderCommands, TextureAtlasTypeTiles, Rectangle2(LetterMin, LetterMax), 
                                      TextCharacter.TextureID, 1.0f, ZLayer, 0.0f);
            }

            LetterMin.X += CharacterWidth + SpacingWidth;
            LetterMax.X = LetterMin.X + CharacterWidth;
        }

        LineLeftTop.Y += CharacterWidth + SpacingWidth;
    }
}
