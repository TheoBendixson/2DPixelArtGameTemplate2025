
void
ResetInput(game_input *Input)
{

#if LEVELEDITOR
    Input->SaveLevelThisFrame = false;
    Input->OpenLevelThisFrame = false;
#endif

    for (s32 i = 0; i < ArrayCount(Input->Keyboard.Buttons); i++)
        Input->Keyboard.Buttons[i].TransitionCount = 0;
    
    for (s32 i = 0; i < ArrayCount(Input->Controller.Buttons); i++)
        Input->Controller.Buttons[i].TransitionCount = 0;
    
    for (s32 i = 0; i < ArrayCount(Input->MouseButtons); i++)
        Input->MouseButtons[i].TransitionCount = 0;

	Input->MouseWheel = 0;
}
