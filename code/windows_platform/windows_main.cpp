
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shobjidl.h>
#include <dwmapi.h>
#include <mmsystem.h>
#include <shellapi.h>
#include <dsound.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <xinput.h>

#include <stdarg.h>
#include <stdio.h>

#include <string.h>
#include <intrin.h>
#define AssertHR(HR) Assert(SUCCEEDED(HR))

#define STR2(x) #x
#define STR(x) STR2(x)

#include "windows_main.h"

global_variable IFileSaveDialog *FileSaveDialog;
#include "windows_file_io.h"
#include "windows_shader_resource_setup.h"
#include "windows_memory.h"
#include "windows_renderer_setup.h"
#include "windows_gpu_transfer.h"
#include "windows_utility.h"

void FatalError(const char* message)
{
    MessageBoxA(NULL, message, "Error", MB_ICONEXCLAMATION);
    ExitProcess(0);
}

#include "../steam_library/steam_api.h"
#include "../game_library/game_main.cpp"
#include "../shared_platform_layer/shared_input.cpp"
#include "../shared_platform_layer/shared_renderer.cpp"

#if LEVELEDITOR
global_variable s32 LoadMenuID;
global_variable s32 SaveMenuID;
#endif

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);
typedef DIRECT_SOUND_CREATE(direct_sound_create);

bool quitting;
bool resizing;
HANDLE main_fiber;
DWORD update_interval;
DWORD next_update;
global_variable HANDLE GlobalStdHandle;
global_variable b32 GlobalPrintToConsole = true;
global_variable volatile b32 GlobalHasFocus = false;
global_variable volatile button_state GlobalMouseButtons[5] = {};
global_variable s64 GlobalPerformanceCountFrequency;

#define FIXED_DT (1.0f/60.0f)

PLATFORM_QUIT_GAME(PlatformQuitGame)
{
    ExitProcess(0);
}


#if LEVELEDITOR
HMENU FileMenu;
HMENU LevelEditorMenu;

global_variable u32 LevelEditorMenuItemCount;
global_variable s32 LevelEditorMenuStartingIndex;

internal void
UpdateLevelEditorMenuItems(b32 Enabled)
{
    UINT Flags; 

    if (Enabled)
    {
        Flags = MF_ENABLED | MF_BYPOSITION;
    } else
    {
        Flags = MF_GRAYED | MF_BYPOSITION;
    }

    EnableMenuItem(FileMenu, 0, Flags);
    EnableMenuItem(FileMenu, 1, Flags);

    for (u32 ItemIndex = 0;
         ItemIndex < LevelEditorMenuItemCount;
         ItemIndex++)
    {
        EnableMenuItem(LevelEditorMenu, ItemIndex, Flags);
    }
}

#endif

PLATFORM_LOG_MESSAGE(PlatformLogMessage)
{
    if (GlobalPrintToConsole)
    {
	WriteFile(GlobalStdHandle, Message, (DWORD)strlen(Message), 0, 0);
    }
}

void WindowsDebugPrintf(char *Format, ...)
{
#ifndef NDEBUG
    va_list Args;
    va_start(Args, Format);

    char Str[1024] = "";
    vsprintf_s(Str, Format, Args);
    Str[ArrayCount(Str) - 1] = 0; //Safety null-termination

    PlatformLogMessage(Str);

    va_end(Args);
#endif
}

inline 
LARGE_INTEGER WindowsGetTimeCounter()
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return Result;
}

inline 
f32 WindowsGetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
    return (f32)(End.QuadPart - Start.QuadPart) / ((f32)GlobalPerformanceCountFrequency);	
}

inline 
f32 WindowsGetSecondsElapsed(LARGE_INTEGER Start)
{
    return WindowsGetSecondsElapsed(Start, WindowsGetTimeCounter());
}

global_variable b32 WindowSized = false;

game_memory GameMemory = {};

LRESULT CALLBACK
MainWindowCallback(HWND Window, 
                   UINT Message,
                   WPARAM WParam,
                   LPARAM LParam)
{
    LRESULT result = 0;
    switch (Message) {
        case WM_CREATE:
            {
#if LEVELEDITOR
                HMENU MenuBar = GetSystemMenu(Window, false);
                FileMenu = CreateMenu();
                LevelEditorMenu = CreateMenu();

                AppendMenu(MenuBar, MF_POPUP, (UINT_PTR)FileMenu, "File");
                AppendMenu(MenuBar, MF_POPUP, (UINT_PTR)LevelEditorMenu, "Level Editor");

                s32 MenuItemCount = 0; 

                LoadMenuID = MenuItemCount;
                AppendMenu(FileMenu, MF_STRING, LoadMenuID, "Open Level");
                MenuItemCount++;

                SaveMenuID = MenuItemCount;
                AppendMenu(FileMenu, MF_STRING, SaveMenuID, "Save Level");
                MenuItemCount++;

                // TODO: (Ted)  Bring this back to support more entries in the game's level editor.
                /*
                LevelEditorMenuItemCount = 0;
                LevelEditorMenuStartingIndex = MenuItemCount;

                game_state *GameState = (game_state*)GameMemory.PermanentStoragePartition.Start;

                if (GameMemory.IsInitialized)
                {
                    for (s32 Index = 0;
                         Index < GameState->LevelEditModeCount;
                         Index++)
                    {
                        level_edit_mode EditMode = GameState->EditModes[Index];

                        char EditModeTitle[20];
                        GetEditModeTitle(EditModeTitle, EditMode);

                        AppendMenu(LevelEditorMenu, MF_STRING, MenuItemCount, EditModeTitle);
                        LevelEditorMenuItemCount++;
                        MenuItemCount++;
                    }
                }*/

                UpdateLevelEditorMenuItems(true);
                SetMenu(Window, MenuBar);
#endif
                break;
            }

			
        case WM_ACTIVATEAPP: // window gets focus.
            GlobalHasFocus = (WParam == TRUE);
            break;
        
	    case WM_DESTROY:
            quitting = true;
            break;
        case WM_SIZE:
            WindowSized = true;
            resizing = true;
            break;
        case WM_PAINT:
        case WM_TIMER:
            SwitchToFiber(main_fiber);
            break;
        case WM_ENTERMENULOOP:
        case WM_ENTERSIZEMOVE:
            SetTimer(Window, 0, 1, 0);
            break;
        case WM_EXITMENULOOP:
        case WM_EXITSIZEMOVE:
            KillTimer(Window, 0);
            break;
        default:
            result = DefWindowProcA(Window, Message, WParam, LParam);
            break;
    }
    return result;
}

game_input Input = {};

void CALLBACK MessageFiberProc(void *) 
{
    for (;;) 
    {
        DWORD now = GetTickCount();
        if (now >= next_update || MsgWaitForMultipleObjects(0, 0, FALSE, next_update - now, QS_ALLEVENTS) == WAIT_OBJECT_0) 
        {

            MSG Message;
            while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE)) 
            {
                u32 WMessage = Message.message;

                if (WMessage == WM_QUIT)
                {
                    break;
                } else if (WMessage == WM_SYSKEYDOWN)
                {

                } else if (WMessage == WM_SYSKEYUP)
                {

                } else if (WMessage == WM_KEYDOWN || WMessage == WM_KEYUP)
                {
                    u32 VKCode = (u32)Message.wParam;
                    b32 WasDown = ((Message.lParam & (1 << 30)) != 0);
                    b32 IsDown = ((Message.lParam & (1 << 31)) == 0);

                    if (WasDown != IsDown)
                    {
                        if (VKCode == VK_UP)
                        {
                            ProcessButton(&Input.Keyboard.Up, IsDown);
                        } 
                        else if (VKCode == VK_DOWN)
                        {
                            ProcessButton(&Input.Keyboard.Down, IsDown);
                        }
                        else if (VKCode == VK_LEFT)
                        {
                            ProcessButton(&Input.Keyboard.Left, IsDown);
                        }
                        else if (VKCode == VK_RIGHT)
                        {
                            ProcessButton(&Input.Keyboard.Right, IsDown);
                        }
                        else if (VKCode == VK_SPACE)
                        {
                            ProcessButton(&Input.Keyboard.Space, IsDown);
                        }
                        else if (VKCode == VK_RETURN)
                        {
                            ProcessButton(&Input.Keyboard.Enter, IsDown);
                        }
                        else if (VKCode == VK_DELETE)
                        {
                            ProcessButton(&Input.Keyboard.Delete, IsDown);
                        }
                        else if (VKCode == VK_BACK)
                        {
                            ProcessButton(&Input.Keyboard.Backspace, IsDown);
                        }
                        else if (VKCode == VK_TAB)
                        {
                            ProcessButton(&Input.Keyboard.Tab, IsDown);
                        }
                        else if (VKCode == VK_SHIFT)
                        {
                            ProcessButton(&Input.Keyboard.Shift, IsDown);
                        }
                        else if (VKCode == VK_MENU)
                        {
                            ProcessButton(&Input.Keyboard.Alt, IsDown);
                        }
                        else if (VKCode == VK_CONTROL)
                        {
                            ProcessButton(&Input.Keyboard.Control, IsDown);
                        }
                        else if (VKCode == VK_ESCAPE)
                        {
                            ProcessButton(&Input.Keyboard.Escape, IsDown);
                        }
                        else if (VKCode >= 'A' && VKCode <= 'Z' )
                        {
                            ProcessButton(&Input.Keyboard.Letters[VKCode - 'A'], IsDown);
                        }
                        else if (VKCode >= '0' && VKCode <= '9'){
                            ProcessButton(&Input.Keyboard.Numbers[VKCode - '0'], IsDown);
                        }
                        else if (VKCode >= VK_F1 && VKCode <= VK_F15)
                        {
                            ProcessButton(&Input.Keyboard.F[VKCode - VK_F1 + 1], IsDown);
                        }
                    }

                } 
                else if (WMessage == WM_SYSCOMMAND)
                {
#if LEVELEDITOR
                    u16 Param = LOWORD(Message.wParam);
                    game_state *GameState = (game_state*)GameMemory.PermanentStoragePartition.Start;

                    if (Param == LoadMenuID)
                    {
                        Input.OpenLevelThisFrame = true;
                    } else if (Param == SaveMenuID)
                    {
                        Input.SaveLevelThisFrame = true;
                    }
#endif
                }
                else if (WMessage == WM_MOUSEWHEEL)
                {
                    //u32 highWord = (u32) message.wParam >> 16;
                    s32 DWheel = GET_WHEEL_DELTA_WPARAM(Message.wParam) / WHEEL_DELTA;
                    Input.MouseWheel += DWheel;

                }else if (WMessage == WM_LBUTTONDOWN ||
                          WMessage == WM_MBUTTONDOWN ||
                          WMessage == WM_RBUTTONDOWN ||
                          WMessage == WM_XBUTTONDOWN)
                {
                    if (Message.wParam & MK_LBUTTON)  ProcessButton((button_state *)&GlobalMouseButtons[0], true);
                    if (Message.wParam & MK_MBUTTON)  ProcessButton((button_state *)&GlobalMouseButtons[1], true);
                    if (Message.wParam & MK_RBUTTON)  ProcessButton((button_state *)&GlobalMouseButtons[2], true);
                    if (Message.wParam & MK_XBUTTON1) ProcessButton((button_state *)&GlobalMouseButtons[3], true);
                    if (Message.wParam & MK_XBUTTON2) ProcessButton((button_state *)&GlobalMouseButtons[4], true);
                }

                TranslateMessage(&Message);
                DispatchMessage(&Message);
            }

        }

        SwitchToFiber(main_fiber);
    }
}

struct windows_instanced_draw_call_data
{
    ID3D11ShaderResourceView* ShaderView;
    v2 TextureSize;
};

windows_instanced_draw_call_data
GetInstancedDrawCallDataForCurrentTextureAtlasType(texture_atlas_type CurrentTextureAtlasType, 
                                                   ID3D11ShaderResourceView** ShaderViews,
                                                   game_texture_buffer *TextureBuffers) 
{
    windows_instanced_draw_call_data Result = {};
    u32 LookupIndex = 0;

    if (CurrentTextureAtlasType == TextureAtlasTypeTiles)
    {
        LookupIndex = 0;
    } 

    Result.ShaderView = ShaderViews[LookupIndex];
    game_texture_buffer *TextureBuffer = &TextureBuffers[LookupIndex];
    Result.TextureSize = { (r32)TextureBuffer->TextureWidth, 
                           (r32)TextureBuffer->TextureHeight };
    return Result;
}

#if STEAMSTORE
#include "../shared_platform_layer/shared_steam.cpp"
#endif

global_variable s32 CursorDisplayCount;

v2
ToggleFullscreen(HWND WindowHandle, b32 Windowed, 
                 WINDOWPLACEMENT *PreviousWindowPlacement,
                 device_simulator_settings DeviceSimulatorSettings)
{
    DWORD dwStyle = GetWindowLong(WindowHandle, GWL_STYLE);

    v2 WindowWidthHeight = V2(0.0f,0.0f);

    if (Windowed)
    {
        MONITORINFO MonitorInfo = { sizeof(MonitorInfo) };
        GetMonitorInfo(MonitorFromWindow(WindowHandle, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo);

        r32 MonitorWidth = MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left;
        r32 MonitorHeight = MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top;

        if (DeviceSimulatorSettings.DeviceType == SimulatedDeviceType_None)
        {
            if (MonitorHeight <= 1600)
            {
                WindowWidthHeight = V2(1280.0f, 800.0f);
            } else
            {
                WindowWidthHeight = V2(2560.0f, 1600.0f);
            }
        } else
        {
            WindowWidthHeight = V2(DeviceSimulatorSettings.WindowWidth, 
                                   DeviceSimulatorSettings.WindowHeight);
        }

        SetWindowLong(WindowHandle, GWL_STYLE,
                      dwStyle | WS_SYSMENU | WS_CAPTION);
        SetWindowPlacement(WindowHandle, PreviousWindowPlacement);

        RECT WindowRect = { 0, 0, WindowWidthHeight.X, WindowWidthHeight.Y };  // Desired client area size
        AdjustWindowRect(&WindowRect, GWL_STYLE, FALSE);

        SetWindowPos(WindowHandle, NULL, 0, 0, WindowRect.right - WindowRect.left, 
                     WindowRect.bottom - WindowRect.top,
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

        while (CursorDisplayCount <= 0)
        {
            CursorDisplayCount = ShowCursor(true);
        }

    } else
    {
        MONITORINFO MonitorInfo = { sizeof(MonitorInfo) };

        if (GetWindowPlacement(WindowHandle, PreviousWindowPlacement) &&
            GetMonitorInfo(MonitorFromWindow(WindowHandle, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo))
        {
            SetWindowLong(WindowHandle, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
            WindowWidthHeight.X = MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left;
            WindowWidthHeight.Y = MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top;

            SetWindowPos(WindowHandle, HWND_TOP,
                         MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
                         WindowWidthHeight.X, WindowWidthHeight.Y,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

            while (CursorDisplayCount >= 0)
            {
                CursorDisplayCount = ShowCursor(false);
            }
        }
    }

    return WindowWidthHeight;
}

void
AdjustViewportWithUpdatedWindowSize(game_render_commands *RenderCommands, r32 WindowWidth, r32 WindowHeight )
{
    RenderCommands->ViewportWidth = WindowWidth;
    RenderCommands->ViewportHeight = WindowHeight;
}

int CALLBACK
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{
    // NOTE: (Pere) Process command line arguments.
    //              Command line options:
    //              -console: opens up a console to output logs, unless the game was ran from cmd.
    int argcW = 0;
    wchar_t **argvW = CommandLineToArgvW(GetCommandLineW(), &argcW);

    for(int i = 0; i < argcW; i++){
        if (WideStringsAreEqual(argvW[i], L"-console")){
            if(AttachConsole((DWORD)-1) == 0){ // NOTE (Pere): If wasn't launched from console
                AllocConsole(); // alloc your own instead
                GlobalPrintToConsole = true;
            }
        }
    }

    GlobalStdHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	
    LARGE_INTEGER PerformanceFrequency;
    QueryPerformanceFrequency(&PerformanceFrequency);
    GlobalPerformanceCountFrequency = PerformanceFrequency.QuadPart;
	
    main_fiber = ConvertThreadToFiber(0);
    HANDLE message_fiber = CreateFiber(0, MessageFiberProc, 0);

    GameSetupMemory(&GameMemory);
    
    GameMemory.PlatformReadPNGFile = PlatformReadPNGFile;
    GameMemory.PlatformReadEntireFile = PlatformReadEntireFile;
    GameMemory.PlatformFreeFileMemory = PlatformFreeMemory;
    GameMemory.PlatformLogMessage = PlatformLogMessage;
    GameMemory.PlatformOpenFileDialog = PlatformOpenFileDialog;
    GameMemory.PlatformSaveFileDialog = PlatformSaveFileDialog;
    GameMemory.PlatformQuitGame = PlatformQuitGame;

// MARK:    Setup Input Defaults
    Input.PrimaryInputMode = GameInputMode_KeyboardMouse;
    //Input.PrimaryInputMode = GameInputMode_GameController;

#if STEAMSTORE
    GameMemory.PlatformUnlockAchievement = PlatformUnlockAchievement;
#endif

    GameMemory.PlatformWriteEntireFile = PlatformWriteEntireFile;

    VirtualAllocateGameMemoryPartition(&GameMemory.PermanentStoragePartition);
    VirtualAllocateGameMemoryPartition(&GameMemory.TransientStoragePartition);

    if (!GameMemory.PermanentStoragePartition.Start || 
        !GameMemory.TransientStoragePartition.Start)
    {
        return(0);
    }

    GameSetupMemoryPartitions(&GameMemory);

    LoadSounds(&GameMemory);

    game_texture_buffer PixelArtTextureBuffers[4];

    game_texture_map TextureMap = {};
    LoadPixelArtTextures(&GameMemory, PixelArtTextureBuffers, &TextureMap);

    simulated_device_type SimulatedDeviceType = SimulatedDeviceType_None;

    device_simulator_settings DeviceSimulatorSettings = {};
    DeviceSimulatorSettings.DeviceType = SimulatedDeviceType;
    DeviceSimulatorSettings.UsesTouchControls = false;

    if (DeviceSimulatorSettings.DeviceType == SimulatedDeviceType_iPhone11)
    {
        DeviceSimulatorSettings.UsesTouchControls = true;
    }

    if (DeviceSimulatorSettings.DeviceType == SimulatedDeviceType_SteamDeckStandard)
    {
        DeviceSimulatorSettings.WindowWidth = 1280;
        DeviceSimulatorSettings.WindowHeight = 800;
    } else if (DeviceSimulatorSettings.DeviceType == SimulatedDeviceType_iPhone11)
    {
        DeviceSimulatorSettings.WindowWidth = 1792;
        DeviceSimulatorSettings.WindowHeight = 828;
    }

    // NOTE: (Ted)  Memory initialization needs to happen before the window gets created.
    GameInitializeMemory(&GameMemory, DeviceSimulatorSettings); 
        
// MARK: Initialize 3rd Party APIs like Steam.
#if STEAMSTORE
    SteamCallbackThingy SteamThing = {};
    SteamThing.GameState = (game_state*)GameMemory.PermanentStoragePartition.Start;

    steam_action_handles SteamActionHandles = {};
    InitializeSteamAndRequestUserStats(&SteamActionHandles);
#endif

// register window class to have custom WindowProc callback
    WNDCLASSEXW WindowClass = {};
    WindowClass.cbSize = sizeof(WindowClass);
    WindowClass.lpfnWndProc = MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    WindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    WindowClass.lpszClassName = L"d3d11_window_class";

    ATOM atom = RegisterClassExW(&WindowClass);
    Assert(atom && "Failed to register window class");

// window properties - width, height and style
    DWORD ExStyle = (WS_EX_APPWINDOW | WS_EX_NOREDIRECTIONBITMAP);
    DWORD Style = (WS_CAPTION | WS_SYSMENU);

    s32 WindowWidth = 0;
    s32 WindowHeight = 0;

// create window
    HWND WindowHandle = CreateWindowExW(ExStyle, WindowClass.lpszClassName, (LPCWSTR)"GameName", Style, 
                                        0, 0, 0, 0, NULL, NULL, Instance, NULL);
    Assert(WindowHandle && "Failed to create window");

    HRESULT HR;
	
    MONITORINFO MonitorInfo = { sizeof(MonitorInfo) };
    GetMonitorInfo(MonitorFromWindow(WindowHandle, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo);
    WindowWidth = MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left;
    WindowHeight = MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top;

    b32 StartWindowed = false;

    game_render_commands RenderCommands = {};
    RenderCommands.Windowed = StartWindowed;
    AdjustViewportWithUpdatedWindowSize(&RenderCommands, WindowWidth, WindowHeight);

    GameSetupRenderer(&GameMemory, &RenderCommands);
    GamePostContentLoadSetup(&GameMemory, &TextureMap, &RenderCommands);

    // Initialize Direct Sound
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
    LPDIRECTSOUNDBUFFER SecondaryBuffer;

    game_startup_config StartupConfig = GameGetStartupConfig();
    u32 SamplesPerSecond = StartupConfig.SoundSamplesPerSecond;
    u32 BytesPerSample = StartupConfig.SoundBytesPerSample; 
    u32 SecondaryBufferSize = StartupConfig.SoundBufferSize;

    b32 SoundIsEnabled = true;

    if (DSoundLibrary)
    {
        direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

        if (DirectSoundCreate)
        {
            LPDIRECTSOUND DirectSound;
            HR = DirectSoundCreate(0, &DirectSound, 0);
            AssertHR(HR);
			
            HR = DirectSound->SetCooperativeLevel(WindowHandle, DSSCL_PRIORITY);
            AssertHR(HR);

            LPDIRECTSOUNDBUFFER PrimaryBuffer;
            DSBUFFERDESC PrimaryBufferDescription {};
            PrimaryBufferDescription.dwSize = sizeof(PrimaryBufferDescription);
            PrimaryBufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

            HR = DirectSound->CreateSoundBuffer(&PrimaryBufferDescription, &PrimaryBuffer, 0);
            AssertHR(HR);

            WAVEFORMATEX WaveFormat = {};
            WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
            WaveFormat.nChannels = 2;
            WaveFormat.wBitsPerSample = 16;
            WaveFormat.nSamplesPerSec = SamplesPerSecond;
            WaveFormat.nBlockAlign = (WaveFormat.nChannels*WaveFormat.wBitsPerSample)/8;
            WaveFormat.nAvgBytesPerSec = WaveFormat.nBlockAlign*WaveFormat.nSamplesPerSec;
            WaveFormat.cbSize = 0;

            HR = PrimaryBuffer->SetFormat(&WaveFormat);
            AssertHR(HR);

            DSBUFFERDESC SecondaryBufferDescription {};
            SecondaryBufferDescription.dwSize = sizeof(SecondaryBufferDescription);
            SecondaryBufferDescription.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS; // NOTE: (Pere)  GLOBALFOCUS enables sound when window doesn't have focus.
            SecondaryBufferDescription.dwBufferBytes = SecondaryBufferSize;
            SecondaryBufferDescription.lpwfxFormat = &WaveFormat;

            HR = DirectSound->CreateSoundBuffer(&SecondaryBufferDescription, &SecondaryBuffer, 0);
            AssertHR(HR);

            HR = SecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
            AssertHR(HR);

        } else 
        {
            SoundIsEnabled = false;
        }
    } else 
    {
        SoundIsEnabled = false;
    }

    s32 OurCursorSample = 0;
    b32 OurCursorSampleIsValid = false;
    s32 LastExtraSamplesWritten = 0;

    game_sound_output_buffer SoundOutputBuffer = {};
    SoundOutputBuffer.Samples = (s16*)calloc(StartupConfig.SoundOutputBufferArrayCount, BytesPerSample); 
    SoundOutputBuffer.SamplesPerSecond = SamplesPerSecond;
    SoundOutputBuffer.SampleArrayCount = StartupConfig.SoundOutputBufferArrayCount;

    IDXGISwapChain* SwapChain;
    ID3D11Device* D11Device;
    ID3D11DeviceContext* DeviceContext;

 // create swap chain, device and context
    {
        DXGI_SWAP_CHAIN_DESC SwapChainDescription = {};
        SwapChainDescription.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

        SwapChainDescription.SampleDesc = { 1, 0 };
        SwapChainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        SwapChainDescription.BufferCount = 2;
        SwapChainDescription.OutputWindow = WindowHandle;
        SwapChainDescription.Windowed = true;
    
        // use more efficient flip model, available in Windows 10
        // if Windows 8 compatibility required, use DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL
        // if Windows 7/Vista compatibility required, use DXGI_SWAP_EFFECT_DISCARD
        // NOTE: flip models do not allow MSAA framebuffer, so if you want MSAA then
        // you'll need to render offscreen and afterwards resolve to non-MSAA framebuffer
        SwapChainDescription.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        UINT Flags = 0;
#ifndef NDEBUG
        Flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        D3D_FEATURE_LEVEL Levels[] = { D3D_FEATURE_LEVEL_11_0 };
        HR = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, Flags, Levels, 
                                           _countof(Levels), D3D11_SDK_VERSION, &SwapChainDescription, 
                                           &SwapChain, &D11Device, NULL, &DeviceContext);
        AssertHR(HR);
    }

#ifndef NDEBUG
    // for debug builds enable debug break on API errors
    {
        ID3D11Debug *D3DDebug;

        HR = D11Device->QueryInterface(__uuidof(ID3D11Debug), (void**)&D3DDebug);
        AssertHR(HR);

        ID3D11InfoQueue *D3DInfoQueue;
        HR = D11Device->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&D3DInfoQueue);
        AssertHR(HR);

        D3DInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
        D3DInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
        D3DInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, true);

        D3DInfoQueue->Release();
        D3DDebug->Release();
    }
#endif

    // disable stupid Alt+Enter changing monitor resolution to match window size
    {
        IDXGIFactory* Factory;
        HR = SwapChain->GetParent(__uuidof(IDXGIFactory), (void**) &Factory);
        AssertHR(HR);
        Factory->MakeWindowAssociation(WindowHandle, DXGI_MWA_NO_ALT_ENTER);
        Factory->Release();
    }

    {
        u32 VertexCount = 6;
        RenderCommands.VertexBuffer.Max = VertexCount;
        RenderCommands.VertexBuffer.Size = sizeof(game_texture_vertex)*VertexCount;
        RenderCommands.VertexBuffer.Vertices =
            (game_texture_vertex *)VirtualAlloc(0, RenderCommands.VertexBuffer.Size, 
                                                MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    }

    GameSetupVertexBuffer(&RenderCommands);
    game_texture_vertex_buffer *VertexBuffer = &RenderCommands.VertexBuffer;

    ID3D11Buffer* WindowsVertexBuffer;
    {
        D3D11_BUFFER_DESC Desc = {};
        Desc.ByteWidth = VertexBuffer->Size;
        Desc.Usage = D3D11_USAGE_DYNAMIC;
        Desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        D3D11_SUBRESOURCE_DATA Initial = {};
        Initial.pSysMem = VertexBuffer->Vertices;
        HRESULT HR = D11Device->CreateBuffer(&Desc, &Initial, &WindowsVertexBuffer);
        AssertHR(HR);
    }

// MARK: Setup DirectX Shaders
    ID3D11InputLayout* PixelArtShaderInputLayout;
    ID3D11VertexShader* PixelArtVertexShader;
    ID3D11PixelShader* PixelArtFragmentShader;
    {
        D3D11_INPUT_ELEMENT_DESC Desc[] =
        {
            { 
                "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 
                offsetof(struct game_texture_vertex, Position), D3D11_INPUT_PER_VERTEX_DATA, 0 
            },
            { 
                "TEXCOORD",0, DXGI_FORMAT_R32G32_FLOAT, 0, 
                offsetof(struct game_texture_vertex, UV), D3D11_INPUT_PER_VERTEX_DATA, 0 
            },
            {
                "TXROWONE", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 
                offsetof(struct renderer_instance, TransformRow1), D3D11_INPUT_PER_INSTANCE_DATA, 1
            },
            {
                "TXROWTWO", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 
                offsetof(struct renderer_instance, TransformRow2), D3D11_INPUT_PER_INSTANCE_DATA, 1
            },
            {
                "TXROWTHREE", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 
                offsetof(struct renderer_instance, TransformRow3), D3D11_INPUT_PER_INSTANCE_DATA, 1
            },
            {
                "TEXTUREID", 0, DXGI_FORMAT_R32_UINT, 1, 
                offsetof(struct renderer_instance, TextureID), D3D11_INPUT_PER_INSTANCE_DATA, 1 
            },
            {
                "ALPHA", 0, DXGI_FORMAT_R32_FLOAT, 1, 
                offsetof(struct renderer_instance, Alpha), D3D11_INPUT_PER_INSTANCE_DATA, 1 
            }
        };

        #include "pixel_art_vertex_shader.h"
        HR = D11Device->CreateVertexShader(pixel_art_vertex_shader, sizeof(pixel_art_vertex_shader), NULL, 
                                           &PixelArtVertexShader);
        AssertHR(HR);

        #include "pixel_art_fragment_shader.h"
        HR = D11Device->CreatePixelShader(pixel_art_fragment_shader, sizeof(pixel_art_fragment_shader), NULL, 
                                          &PixelArtFragmentShader);
        AssertHR(HR);

        HR = D11Device->CreateInputLayout(Desc, _countof(Desc), pixel_art_vertex_shader, sizeof(pixel_art_vertex_shader), 
                                          &PixelArtShaderInputLayout);
        AssertHR(HR);
    }

    ID3D11Buffer *WindowsInstanceBuffer = SetupInstanceBuffer(D11Device, 5000, &RenderCommands.InstanceBuffer);

    ID3D11Buffer* ViewportSizeConstantsBuffer;
    {
        D3D11_BUFFER_DESC Desc = {};
        Desc.ByteWidth = 2 * 2 * sizeof(float);
        Desc.Usage = D3D11_USAGE_DYNAMIC;
        Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        HR = D11Device->CreateBuffer(&Desc, NULL, &ViewportSizeConstantsBuffer);
        AssertHR(HR);
    }

    ID3D11Buffer* TextureSizeConstantBuffer;
    {
        D3D11_BUFFER_DESC Desc = {};
        Desc.ByteWidth = 4 * 4 * sizeof(float);
        Desc.Usage = D3D11_USAGE_DYNAMIC;
        Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        HR = D11Device->CreateBuffer(&Desc, NULL, &TextureSizeConstantBuffer);
        AssertHR(HR);
    }

    game_texture_buffer *PixelArtTileTextureBuffer = &PixelArtTextureBuffers[TileTextureBufferIndex];
    ID3D11ShaderResourceView* PixelArtTileShaderView = SetupShaderResourceView(D11Device, PixelArtTileTextureBuffer);

    ID3D11ShaderResourceView* ShaderViews[3];
    ShaderViews[0] = PixelArtTileShaderView;

    GameClearTransientMemory(&GameMemory.TransientStoragePartition);

    ID3D11SamplerState* Sampler;
    {
        D3D11_SAMPLER_DESC Desc = {};
        Desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        Desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        Desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        Desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        Desc.MinLOD = -FLT_MAX;
        Desc.MaxLOD = FLT_MAX;
        Desc.MaxAnisotropy = 1;
        Desc.ComparisonFunc = D3D11_COMPARISON_NEVER;

        HR = D11Device->CreateSamplerState(&Desc, &Sampler);
        AssertHR(HR);
    }

    ID3D11BlendState* BlendState;
    {
        // enable alpha blending
        D3D11_BLEND_DESC Desc = {};
        D3D11_RENDER_TARGET_BLEND_DESC RenderTarget = {};
        RenderTarget.BlendEnable = TRUE;
        RenderTarget.SrcBlend = D3D11_BLEND_SRC_ALPHA;
        RenderTarget.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        RenderTarget.BlendOp = D3D11_BLEND_OP_ADD;
        RenderTarget.SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
        RenderTarget.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        RenderTarget.BlendOpAlpha = D3D11_BLEND_OP_ADD;
        RenderTarget.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        Desc.RenderTarget[0] = RenderTarget;

        HR = D11Device->CreateBlendState(&Desc, &BlendState);
        AssertHR(HR);
    }

    ID3D11RasterizerState* RasterizerState;
    {
        // disable culling
        D3D11_RASTERIZER_DESC Desc = {};
        Desc.FillMode = D3D11_FILL_SOLID;
        Desc.CullMode = D3D11_CULL_NONE;
        HR = D11Device->CreateRasterizerState(&Desc, &RasterizerState);
        AssertHR(HR);
    }

    ID3D11RenderTargetView* RTView = NULL;

    // show the window
    ShowWindow(WindowHandle, SW_SHOWDEFAULT);

    local_persist WINDOWPLACEMENT PreviousWindowPlacement = { sizeof(PreviousWindowPlacement) };
    GetWindowPlacement(WindowHandle, &PreviousWindowPlacement);

    v2 WindowWidhtHeight =
        ToggleFullscreen(WindowHandle, RenderCommands.Windowed, 
                         &PreviousWindowPlacement,
                         DeviceSimulatorSettings);
    WindowWidth = WindowWidhtHeight.X;
    WindowHeight = WindowWidhtHeight.Y;
    AdjustViewportWithUpdatedWindowSize(&RenderCommands, WindowWidth, WindowHeight);

    LARGE_INTEGER C1 = WindowsGetTimeCounter();

    r32 dtAccumulator = 0.0f;

    update_interval = 10;
    next_update = GetTickCount();

    local_persist b32 PreviousWindowedState = RenderCommands.Windowed;

    {
        D3D11_MAPPED_SUBRESOURCE Mapped;
        HRESULT HR = DeviceContext->Map((ID3D11Resource*)WindowsVertexBuffer, 0, 
                                         D3D11_MAP_WRITE_DISCARD, 0, &Mapped);
        AssertHR(HR);
        memcpy(Mapped.pData, RenderCommands.VertexBuffer.Vertices, 
               RenderCommands.VertexBuffer.Size);
        DeviceContext->Unmap((ID3D11Resource*)WindowsVertexBuffer, 0);
    }

    s32 FrameCounter;
    s32 FpsMeasured;
    LARGE_INTEGER TimeOfLastFpsUpdate = WindowsGetTimeCounter();

    f32 LastFrameDuration = 1/60.f;
    LARGE_INTEGER TimeOfLastFrame = WindowsGetTimeCounter();

    while (!quitting)
    {
        SwitchToFiber(message_fiber);
        DWORD now = GetTickCount();

        if (now >= next_update) 
        {

#if STEAMSTORE
            SteamAPI_RunCallbacks();
            RunSteamInput(&Input.Controller, &SteamActionHandles);
#endif

            b32 ChangedFullscreenMode = false;

            if (PreviousWindowedState != RenderCommands.Windowed)
            {
                ChangedFullscreenMode = true;
            }

            if (ChangedFullscreenMode)
            {
                WindowWidhtHeight =
                        ToggleFullscreen(WindowHandle, RenderCommands.Windowed, 
                                         &PreviousWindowPlacement,
                                         DeviceSimulatorSettings);
                WindowWidth = WindowWidhtHeight.X;
                WindowHeight = WindowWidhtHeight.Y;
                AdjustViewportWithUpdatedWindowSize(&RenderCommands, WindowWidth, WindowHeight);
            }

            if (RTView == NULL || WindowSized)
            {
                ShowWindow(WindowHandle, SW_SHOWDEFAULT);

                if (RTView)
                {
                    // release old swap chain buffers
                    DeviceContext->ClearState();
                    RTView->Release();
                    RTView = NULL;
                }

                // resize to new size for non-zero sizes
                HR = SwapChain->ResizeBuffers(0, WindowWidth, WindowHeight, DXGI_FORMAT_UNKNOWN, 0);

                if (FAILED(HR))
                {
                    FatalError("Failed to resize swap chain!");
                }

                WindowSized = false;

                D3D11_RENDER_TARGET_VIEW_DESC RTDesc = {};
                RTDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // or use DXGI_FORMAT_R8G8B8A8_UNORM_SRGB for storing sRGB
                RTDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

                // create RenderTarget view for new backbuffer texture
                ID3D11Texture2D* Backbuffer;
                HR = SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&Backbuffer);
                AssertHR(HR);
                HR = D11Device->CreateRenderTargetView((ID3D11Resource*)Backbuffer, &RTDesc, &RTView);
                AssertHR(HR);
                Backbuffer->Release();
            }

// MARK: XInput	
            XINPUT_STATE XInputState;
            DWORD XInputResult;

            XInputResult = XInputGetState(0, &XInputState);

            if (XInputResult == ERROR_SUCCESS)
            {
                game_controller_input *Controller = &Input.Controller;
                b32 APressed = (XInputState.Gamepad.wButtons & XINPUT_GAMEPAD_A);
                ProcessButton(&Controller->A, APressed);
                b32 BPressed = (XInputState.Gamepad.wButtons & XINPUT_GAMEPAD_B);
                ProcessButton(&Controller->B, BPressed);
                b32 XPressed = (XInputState.Gamepad.wButtons & XINPUT_GAMEPAD_X);
                ProcessButton(&Controller->X, XPressed);
                b32 YPressed = (XInputState.Gamepad.wButtons & XINPUT_GAMEPAD_Y);
                ProcessButton(&Controller->Y, YPressed);

                b32 UpPressed = (XInputState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP);
                ProcessButton(&Controller->Up, UpPressed);
                b32 DownPressed = (XInputState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                ProcessButton(&Controller->Down, DownPressed);
                b32 LeftPressed = (XInputState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                ProcessButton(&Controller->Left, LeftPressed);
                b32 RightPressed = (XInputState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                ProcessButton(&Controller->Right, RightPressed);

                b32 RightShoulder1Pressed = (XInputState.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                ProcessButton(&Controller->RightShoulder1, RightShoulder1Pressed);

                r32 RightTriggerValue = XInputState.Gamepad.bRightTrigger;
                b32 RightShoulder2Pressed = (RightTriggerValue > 30);
                ProcessButton(&Controller->RightShoulder2, RightShoulder2Pressed);

                b32 LeftShoulder1Pressed = (XInputState.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                ProcessButton(&Controller->LeftShoulder1, LeftShoulder1Pressed);

                b32 LeftShoulder2Pressed = (XInputState.Gamepad.bLeftTrigger > 30);
                ProcessButton(&Controller->LeftShoulder2, LeftShoulder2Pressed);

                b32 SelectPressed = (XInputState.Gamepad.wButtons & XINPUT_GAMEPAD_START);
                ProcessButton(&Controller->Select, SelectPressed);

                {
                    r32 RightX = XInputState.Gamepad.sThumbRX;
                    r32 RightY = XInputState.Gamepad.sThumbRY;
                    r32 Magnitude = sqrt(RightX*RightX + RightY*RightY);
                    s32 Deadzone = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
                    r32 NormalizedRightX = 0.0f;
                    r32 NormalizedRightY = 0.0f;

                    if (Magnitude < Deadzone)
                    {
                        RightX = 0.0f;
                        RightY = 0.0f;
                    } else
                    {
                        r32 SixteenBitSignedMax = 32767.0f;
                        NormalizedRightX = RightX/SixteenBitSignedMax;
                        NormalizedRightY = RightY/SixteenBitSignedMax;

                        r32 AdjustedMagnitude = (Magnitude - Deadzone)/(SixteenBitSignedMax - Deadzone);
                        NormalizedRightX *= AdjustedMagnitude;
                        NormalizedRightY *= AdjustedMagnitude;
                    }

                    Controller->RightThumb = V2(NormalizedRightX, NormalizedRightY);
                }
                
            }

// MARK: Mouse Input
            v2 WindowDim = WindowsGetWindowDimension(WindowHandle);

            POINT MousePoint;
            GetCursorPos(&MousePoint);
            ScreenToClient(WindowHandle, &MousePoint);
            Input.MousePos.X = (r32)MousePoint.x;
            Input.MousePos.Y = (r32)MousePoint.y;

            Input.WindowDim = WindowDim;
            
            b32 MouseInWindow = (Input.MousePos.X >= 0.0f && Input.MousePos.X < WindowDim.X &&
                                 Input.MousePos.Y >= 0.0f && Input.MousePos.Y < WindowDim.Y);
            b32 LocalHasFocus = GlobalHasFocus; // NOTE: (Pere)  GlobalHasFocus is changed by the other thread.
            if (GlobalHasFocus)
            {
                // NOTE: (Pere)  Here we only unpress buttons. We press them at window message loop
                //               because that allows us to filter out clicks that are not on client area.
                b32 MouseButtonsAreDown[5] = { ((GetKeyState(VK_LBUTTON ) & (1<<15)) != 0),
                                               ((GetKeyState(VK_MBUTTON ) & (1<<15)) != 0),
                                               ((GetKeyState(VK_RBUTTON ) & (1<<15)) != 0),
                                               ((GetKeyState(VK_XBUTTON1) & (1<<15)) != 0),
                                               ((GetKeyState(VK_XBUTTON2) & (1<<15)) != 0) };
                for (s32 i = 0; i < 5; i++)
                {
                    if (GlobalMouseButtons[i].EndedDown && !MouseButtonsAreDown[i])
                        ProcessButton((button_state *)&GlobalMouseButtons[i], false);
                }
            }else
            {
                for(s32 i = 0; i < 5; i++)
                {
                    ProcessButton((button_state *)&GlobalMouseButtons[i], false);
                }
            }

            for (s32 i = 0; i < 5; i++)
            {
                ProcessButton(&Input.MouseButtons[i], GlobalMouseButtons[i].EndedDown);
                GlobalMouseButtons[i].TransitionCount = 0;
            }

            // can render only if window size is non-zero - we must have backbuffer & RenderTarget view created
            if (RTView)
            {
                LARGE_INTEGER C2 = WindowsGetTimeCounter();
		        r32 Delta = WindowsGetSecondsElapsed(C1, C2);
				
                dtAccumulator += Delta;

                /*
                char PerformanceCounterString[100];
                sprintf_s(PerformanceCounterString, "Performance Counter: %f \n", Delta);
                OutputDebugString(PerformanceCounterString);*/

                C1 = C2;

                LARGE_INTEGER WithinFrameCounter1;
                QueryPerformanceCounter(&WithinFrameCounter1);
    
                // output viewport covering all client area of window
                D3D11_VIEWPORT Viewport = {};
                Viewport.TopLeftX = 0;
                Viewport.TopLeftY = 0;
                Viewport.Width = (FLOAT)WindowWidth;
                Viewport.Height = (FLOAT)WindowHeight;
                Viewport.MinDepth = 0;
                Viewport.MaxDepth = 1;

                while (dtAccumulator >= FIXED_DT)
                {
                    PreviousWindowedState = RenderCommands.Windowed;

                    dtAccumulator -= FIXED_DT;
                    Input.dtForFrame = FIXED_DT;
                    Input.FrameRateMultiplier = 1.0f;
                    Input.HasFocusPrev = Input.HasFocus;
                    Input.HasFocus = LocalHasFocus;
                    GameUpdate(&GameMemory, &Input, &RenderCommands);
		            LocalHasFocus = GlobalHasFocus;

                    ResetInput(&Input);

                    b32 DebugMovementSlow = false;
#if DNDEBUG
                    DebugMovementSlow = false;
#endif
                    if (DebugMovementSlow)
                    {
                        Input.Controller.Right.EndedDown = false;
                        Input.Controller.Left.EndedDown = false;
                        Input.Controller.A.EndedDown = false;
                        Input.Controller.Up.EndedDown = false;
                    }
                }

                GameRender(&GameMemory, &TextureMap, 
                           &Input, &RenderCommands);

                // Update FPS counter
                FrameCounter++;
                {
                    LARGE_INTEGER CurrentTime = WindowsGetTimeCounter();
                    f32 Seconds = WindowsGetSecondsElapsed(TimeOfLastFpsUpdate, CurrentTime);
                    if (Seconds >= 1.f)
                    {
                        TimeOfLastFpsUpdate = CurrentTime;
                        FpsMeasured = FrameCounter;
                        FrameCounter = 0;
                        
                        //WindowsDebugPrintf("FPS: %i\n", FpsMeasured);
                    }
                }
                
                // clear screen
                float_color GameClearColor = RenderCommands.ClearColor;
                FLOAT ClearColor[4]; 

                ClearColor[0] = GameClearColor.Red;
                ClearColor[1] = GameClearColor.Green;
                ClearColor[2] = GameClearColor.Blue;
                ClearColor[3] = GameClearColor.Alpha;

                DeviceContext->ClearRenderTargetView(RTView, ClearColor);

// MARK: Direct Sound Output
                if (SoundIsEnabled)
                {
                    DWORD CurrentPlayCursor;
                    DWORD CurrentWriteCursor;

                    static s32 DEBUG_LastNPlayCursorSamples[30] = {};
                    static s32 DEBUG_LastPlayCursorSample = 0;
                    static s32 DEBUG_LastWriteCursorSample = 0;
                    auto PreSoundProcessingTime = WindowsGetTimeCounter();

                    if (SUCCEEDED(SecondaryBuffer->GetCurrentPosition(&CurrentPlayCursor, &CurrentWriteCursor)))
                    {
                        s32 PlayCursorSample = CurrentPlayCursor / BytesPerSample;
                        s32 WriteCursorSample = CurrentWriteCursor / BytesPerSample;
                        s32 SamplesInBuffer = SecondaryBufferSize / BytesPerSample;

                        s32 FrameLengthVariationInSamples = (s32)(SamplesPerSecond*(.023f));// +
                                                                                            //LerpClamp(0.000f, .016666f, (LastFrameDuration - .016666f)/.016666f)));

                        // NOTE: (Pere)  This takes into account the fact that CurrentPlayCursor will have
                        //               moved a bit by the time we get to writing to the Secondary Buffer.
                        s32 EstimatedSamplesPassedWhileProcessing = (s32)(SamplesPerSecond*.001f);
                        s32 EstimatedWriteCursorSampleAfterProcessing = 
                            (WriteCursorSample + EstimatedSamplesPassedWhileProcessing) % SamplesInBuffer;
                        
                        if (!OurCursorSampleIsValid)
                        {
                            OurCursorSample = EstimatedWriteCursorSampleAfterProcessing;
                            OurCursorSampleIsValid = true;
                        }

                        s32 SamplesPerFrame = SamplesPerSecond/60; 
                        s32 TargetCursorSample = 
                            (EstimatedWriteCursorSampleAfterProcessing + 
                             SamplesPerFrame +
                             FrameLengthVariationInSamples) % SamplesInBuffer;

                        // NOTE: (Pere)  We'd like OurCursorSample to now be around EstimatedWriteCursorSampleAfterProcessing,
                        //               and finish at TargetCursorSample.
                        
                        s32 MaxSamplesToAdvance = SamplesPerFrame*8;

                        // NOTE: (Pere)  This simple Lerp just gives more extra samples the lower the frame rate is.
                        SoundOutputBuffer.SamplesToWriteExtra = (s32)LerpClamp(SamplesPerFrame*4, SamplesPerFrame*6, (LastFrameDuration - 1.f/60.f) / (1.f/20.f - 1.f/60.f));
                        SoundOutputBuffer.SamplesToAdvanceBeforeWriting = 0;
                        SoundOutputBuffer.SamplesToWriteAndAdvance = 0;

                        s32 Diff = CircularIndexDifference(OurCursorSample, WriteCursorSample, SamplesInBuffer);
                        if (Diff > MaxSamplesToAdvance + FrameLengthVariationInSamples)
                        { // OurCursor got behind the WriteCursor, so move it to WriteCursor.
                            OurCursorSample = WriteCursorSample;
                            s32 SamplesBehind = SamplesInBuffer - Diff;
                            SoundOutputBuffer.SamplesToAdvanceBeforeWriting = MIN(SamplesBehind, LastExtraSamplesWritten);
                            
#if 0
                        // @DEBUG
                        WindowsDebugPrintf("GOT BEHIND BY %i SAMPLES. Samples to write=%i. Advance=%i. Last Extra=%i. Last Frame=%.2fms. Curr Frame=%.2fms\n",
                                           SamplesBehind,
                                           CircularIndexDifference(TargetCursorSample, OurCursorSample, SamplesInBuffer),
                                           LastExtraSamplesWritten,
                                           SoundOutputBuffer.SamplesToAdvanceBeforeWriting,
                                           1000.f*LastFrameDuration,
                                           1000.f*WindowsGetSecondsElapsed(TimeOfLastFrame));
                        WindowsDebugPrintf("PC=%i, WC=%i, (dif=%i). Last PC=%i, Last WC=%i, (dif=%i)\n",
                                           PlayCursorSample, WriteCursorSample, CircularIndexDifference(WriteCursorSample, PlayCursorSample, SamplesInBuffer),
                                           DEBUG_LastPlayCursorSample, DEBUG_LastWriteCursorSample, CircularIndexDifference(DEBUG_LastWriteCursorSample, DEBUG_LastPlayCursorSample, SamplesInBuffer));
                        WindowsDebugPrintf("Last %i play cursors:\n", ArrayCount(DEBUG_LastNPlayCursorSamples));
                        WindowsDebugPrintf("  Current: %i (+%i)\n", PlayCursorSample,
                                           PlayCursorSample - DEBUG_LastNPlayCursorSamples[0]);

                        for(s32 i = 0; i < ArrayCount(DEBUG_LastNPlayCursorSamples); i++)
                        {
                            WindowsDebugPrintf("  %i: %i", i, DEBUG_LastNPlayCursorSamples[i]);
                            if (i + 1 < ArrayCount(DEBUG_LastNPlayCursorSamples))
                            {
                                WindowsDebugPrintf(" (+%i)", DEBUG_LastNPlayCursorSamples[i] - DEBUG_LastNPlayCursorSamples[i + 1]);
                            }
                            WindowsDebugPrintf("\n");
                        }
#endif
                        }

                        SoundOutputBuffer.SamplesToWriteAndAdvance = CircularIndexDifference(TargetCursorSample, OurCursorSample, SamplesInBuffer);
                        if (SoundOutputBuffer.SamplesToWriteAndAdvance > SamplesInBuffer/2)
                        { // The TargetCursor is behind OurCursor, so don't advance it.
                            SoundOutputBuffer.SamplesToWriteAndAdvance = 0;
                        }

                        GameGetSoundSamples(&GameMemory, &SoundOutputBuffer, &Input);
                        LastExtraSamplesWritten = SoundOutputBuffer.SamplesToWriteExtra;

                        s32 SamplesToWriteTotal = SoundOutputBuffer.SamplesToWriteAndAdvance + SoundOutputBuffer.SamplesToWriteExtra;
                        DWORD ByteToLock = (DWORD)(OurCursorSample*BytesPerSample);
                        DWORD BytesToWrite = (DWORD)(SamplesToWriteTotal*BytesPerSample);
                        Assert(BytesToWrite < SecondaryBufferSize);

                        // Write to buffer.
                        VOID *Region1;
                        VOID *Region2;
                        DWORD Region1Size;
                        DWORD Region2Size;

                        HRESULT HR = SecondaryBuffer->Lock(ByteToLock, BytesToWrite, 
                                       &Region1, &Region1Size,
                                       &Region2, &Region2Size, 0);
                        if (HR == DSERR_BUFFERLOST) 
                        {
                            SecondaryBuffer->Restore(); 
                            HR = SecondaryBuffer->Lock(ByteToLock, BytesToWrite, 
                                                       &Region1, &Region1Size,
                                                       &Region2, &Region2Size, 0);
                        } 

                        if (SUCCEEDED(HR))
                        {
                            Assert(Region1Size + Region2Size == BytesToWrite);
                            
                            memcpy(Region1, SoundOutputBuffer.Samples, Region1Size);
                            if (Region2Size)
                            {
                                memcpy(Region2, (u8 *)SoundOutputBuffer.Samples + Region1Size, Region2Size);
                            }

                            SecondaryBuffer->Unlock(Region1, Region1Size,
                                                    Region2, Region2Size);
                        }

                        OurCursorSample = (OurCursorSample + SoundOutputBuffer.SamplesToWriteAndAdvance) % SamplesInBuffer;

                        // @DEBUG Testing: Put silence behind play cursor. Doesn't sound significantly better,
                        //                 but it can be useful for debugging the sound mixer.
                        if (0){
                            BytesToWrite = SamplesPerFrame*10*BytesPerSample;
                            ByteToLock = (CurrentPlayCursor - 1 - BytesToWrite + SecondaryBufferSize) % SecondaryBufferSize;
                            if (SUCCEEDED(SecondaryBuffer->Lock(ByteToLock, BytesToWrite, 
                                &Region1, &Region1Size,
                                &Region2, &Region2Size, 0)))
                            {
                                Assert(Region1Size + Region2Size == BytesToWrite);
                        
                                memset(Region1, 0, Region1Size);
                                if (Region2Size)
                                {
                                    memset(Region2, 0, Region2Size);
                                }

                                SecondaryBuffer->Unlock(Region1, Region1Size,
                                                        Region2, Region2Size);
                            }
                        }

                        DEBUG_LastPlayCursorSample = PlayCursorSample;
                        DEBUG_LastWriteCursorSample = WriteCursorSample;
                        for(s32 i = ArrayCount(DEBUG_LastNPlayCursorSamples); i > 0; i--){
                                DEBUG_LastNPlayCursorSamples[i] = DEBUG_LastNPlayCursorSamples[i - 1];
                        }
                        DEBUG_LastNPlayCursorSamples[0] = PlayCursorSample;
                    }
                }

                {
                    float ViewportSize[] = {WindowWidth, WindowHeight};

                    D3D11_MAPPED_SUBRESOURCE Mapped;
                    HR = DeviceContext->Map((ID3D11Resource*)ViewportSizeConstantsBuffer, 
                                             0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped);
                    AssertHR(HR);
                    memcpy(Mapped.pData, ViewportSize, sizeof(ViewportSize));
                    DeviceContext->Unmap((ID3D11Resource*)ViewportSizeConstantsBuffer, 0);
                }

                TransferInstanceBufferToGPU(DeviceContext, WindowsInstanceBuffer, &RenderCommands.InstanceBuffer);

                // Input Assembler
                DeviceContext->IASetInputLayout(PixelArtShaderInputLayout);

                DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                UINT Stride = sizeof(struct game_texture_vertex);
                UINT Offset = 0;

                // Rasterizer Stage
                DeviceContext->RSSetViewports(1, &Viewport);
                DeviceContext->RSSetState(RasterizerState);

                // Output Merger
                DeviceContext->OMSetBlendState(BlendState, NULL, ~0U);
                DeviceContext->OMSetRenderTargets(1, &RTView, NULL);

                DeviceContext->VSSetConstantBuffers(0, 1, &ViewportSizeConstantsBuffer);
                DeviceContext->PSSetConstantBuffers(1, 1, &TextureSizeConstantBuffer);
                DeviceContext->VSSetShader(PixelArtVertexShader, NULL, 0);
                DeviceContext->IASetVertexBuffers(0, 1, &WindowsVertexBuffer, &Stride, &Offset);

                UINT InstanceStride = sizeof(struct renderer_instance);
                UINT InstanceOffset = 0;

                // Pixel Shader
                DeviceContext->PSSetSamplers(0, 1, &Sampler);
                DeviceContext->PSSetShader(PixelArtFragmentShader, NULL, 0);

                ResetDrawCommandBuffersBeforePlatformRender(&RenderCommands);

                u32 VerticesInAQuad = 6;
                u32 PreviousInstanceCount = 0;
                u32 InstancesToDraw = 0;

                game_texture_draw_command_buffer *DrawCommandsBuffer = &RenderCommands.DrawCommandsBuffer;
                texture_atlas_type CurrentTextureAtlasType = DrawCommandsBuffer->BottomLayers[0].Commands[0].TextureAtlasType;

                for (s32 LayerIndex = 0;
                     LayerIndex < BOTTOM_LAYER_COUNT;
                     LayerIndex += 1)
                {
                    render_commands_array *CommandsArray;
                    CommandsArray = &DrawCommandsBuffer->BottomLayers[LayerIndex];

                    for (s32 CommandIndex = 0;
                         CommandIndex < CommandsArray->CommandCount;
                         CommandIndex += 1)
                    {
                        game_texture_draw_command Command = CommandsArray->Commands[CommandIndex];

                        if (Command.TextureAtlasType != CurrentTextureAtlasType)
                        {
                            windows_instanced_draw_call_data InstanceDrawCallData =
                                GetInstancedDrawCallDataForCurrentTextureAtlasType(CurrentTextureAtlasType, ShaderViews, 
                                                                                   PixelArtTextureBuffers);

                            DeviceContext->PSSetShaderResources(0, 1, &InstanceDrawCallData.ShaderView);
                            DeviceContext->IASetVertexBuffers(1, 1, &WindowsInstanceBuffer, 
                                                              &InstanceStride, &InstanceOffset);

                            TransferTextureSizeToGPU(DeviceContext, TextureSizeConstantBuffer, 
                                                     InstanceDrawCallData.TextureSize);

                            DeviceContext->DrawInstanced(VerticesInAQuad, InstancesToDraw, 0, PreviousInstanceCount);
                            PreviousInstanceCount += InstancesToDraw;

                            InstancesToDraw = 0;
                        }

                        CurrentTextureAtlasType = Command.TextureAtlasType;
                        InstancesToDraw += 1;
                    }
                }

                if (InstancesToDraw > 0)
                {
                    windows_instanced_draw_call_data InstanceDrawCallData =
                        GetInstancedDrawCallDataForCurrentTextureAtlasType(CurrentTextureAtlasType, ShaderViews,
                                                                           PixelArtTextureBuffers);

                    DeviceContext->PSSetShaderResources(0, 1, &InstanceDrawCallData.ShaderView);
                    DeviceContext->IASetVertexBuffers(1, 1, &WindowsInstanceBuffer, 
                                                      &InstanceStride, &InstanceOffset);

                    TransferTextureSizeToGPU(DeviceContext, TextureSizeConstantBuffer, 
                                             InstanceDrawCallData.TextureSize);

                    DeviceContext->DrawInstanced(VerticesInAQuad, InstancesToDraw, 0, PreviousInstanceCount);
                    PreviousInstanceCount += InstancesToDraw;
                }

                LARGE_INTEGER WithinFrameCounter2 = WindowsGetTimeCounter();
				float WithinFrameDelta = WindowsGetSecondsElapsed(WithinFrameCounter1, WithinFrameCounter2);

                /* 
                char PerformanceCounterString[100];
                sprintf_s(PerformanceCounterString, "Within Frame Performance Counter: %f \n", WithinFrameDelta);
                sprintf_s(PerformanceCounterString, "Within Frame Rate: %f \n", 1/WithinFrameDelta);
                OutputDebugString(PerformanceCounterString);*/
            }

            // change to FALSE to disable vsync
            BOOL Vsync = TRUE;
            HR = SwapChain->Present(Vsync ? 1:0, 0);

            if (HR == DXGI_STATUS_OCCLUDED)
            {
                // window is minimized, cannot vsync - instead sleep a bit
                if (Vsync)
                {
                    Sleep(10);
                }
            }
            else if (FAILED(HR))
            {
                FatalError("Failed to present swap chain! Device lost?");
            }

            {
                LARGE_INTEGER Now = WindowsGetTimeCounter();
                LastFrameDuration = WindowsGetSecondsElapsed(TimeOfLastFrame, Now);
                TimeOfLastFrame = Now;
            }
        }
    }

    SteamAPI_Shutdown();

    return(0);
}
