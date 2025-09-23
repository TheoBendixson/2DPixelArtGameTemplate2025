// 
// By Ted Bendixson
//
// OSX Main

#import <AppKit/AppKit.h>
#import <QuartzCore/QuartzCore.h>
#import <QuartzCore/CoreAnimation.h>
#import <IOKit/hid/IOHIDLib.h>
#import <AudioToolbox/AudioToolbox.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#include <mach/mach_init.h>
#include <mach/mach_time.h>
#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <sys/stat.h>

#import <GameController/GameController.h>

#if MACAPPSTORE
#import <GameKit/GameKit.h>
#endif

#include "osx_main.h"
#include "../../code/shared_apple_platform/TextureShaderTypes.h"

#define TILE_TEXTURE_COUNT 500 

#include "mac_file_path.cpp"

NSString *BuildGameSupportDirectory(NSString *GameName)
{
    NSArray *Paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
    NSString *AppSupportDirectory = [Paths firstObject];
    NSString *GameSupportDirectory = [AppSupportDirectory stringByAppendingPathComponent: GameName];
    return GameSupportDirectory;
}

#include "../../code/shared_apple_platform/apple_file_io.cpp"
#include "mac_file.cpp"

#if INTERNAL
#include "mac_recording.h"
#endif

#include "mac_window.mm"
#include "mac_game_controller.h"
#include "../../code/shared_apple_platform/apple_sound.cpp"
#include "mac_time.h"

static const NSUInteger kMaxInflightBuffers = 3;
static const size_t kAlignedPixelArtInstanceUniformsSize = (sizeof(texture_draw_command_instance_uniforms) & ~0xFF) + 0x100;

#include "../shared_apple_platform/metal_setup.h"
#include "../../code/shared_apple_platform/apple_logging.h"
#include "../shared_platform_layer/shared_input.cpp"
#include "../shared_platform_layer/shared_renderer.cpp"

#include "../../code/shared_apple_platform/apple_instance_buffer.cpp"

#if STEAMSTORE
#include "../steam_library/steam_api.h"
#include "../shared_platform_layer/shared_steam.cpp"
#endif

#include "../game_library/game_main.cpp"

#include "mac_save_panel.mm"

global_variable game_memory GameMemory;

global_variable CVDisplayLinkRef GlobalDisplayLink;
global_variable b32 Quitting = false;

global_variable dispatch_semaphore_t QuitGameSemaphore;
global_variable NSString *GameName;

PLATFORM_QUIT_GAME(PlatformQuitGame)
{
    Quitting = true;

    dispatch_async(dispatch_get_main_queue(), ^{
        dispatch_semaphore_wait(QuitGameSemaphore, DISPATCH_TIME_FOREVER);

#if STEAMSTORE
        SteamAPI_Shutdown();
#endif

        CVDisplayLinkStop(GlobalDisplayLink);
        [NSApp terminate: nil];
    });
}



@interface GameWindow: NSWindow
@end

@implementation GameWindow

- (BOOL)canBecomeKeyWindow
{
    return YES;
}

- (BOOL)canBecomeMainWindow
{
    return YES;
}

- (void)keyDown:(NSEvent *)theEvent 
{ 

}

- (void)keyUp:(NSEvent *)theEvent
{

}

@end


global_variable GameWindow *Window;
global_variable NSOpenPanel *OpenPanel;
global_variable NSSavePanel *SavePanel;
global_variable LevelEditorSavePanelDelegate *SavePanelDelegate;

global_variable NSMenu *FileMenu;
global_variable NSMenuItem *FileMenuItem;

global_variable NSMenu *LevelEditorModeMenu;
global_variable NSMenuItem *LevelEditorModeMenuItem;

@interface CustomMetalView: NSView

@property (nonatomic, nonnull, readonly) CAMetalLayer *metalLayer;
@end

@implementation CustomMetalView

- (instancetype) initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if(self)
    {
        [self initCommon];
    }
    return self;
}

- (instancetype) initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if(self)
    {
        [self initCommon];
    }
    return self;
}

- (void)initCommon
{
    self.wantsLayer = true;
    _metalLayer = [CAMetalLayer layer];
    self.layer = _metalLayer;
}

@end

#include "../../code/shared_apple_platform/metal_view_delegate.m"

@interface AppDelegate: NSObject <NSApplicationDelegate>
@end

@implementation AppDelegate

- (void)applicationDidBecomeActive:(NSNotification *)notification
{
    [Window makeKeyAndOrderFront:self];
}

- (void)applicationDidResignActive:(NSNotification *)notification
{
    [Window orderOut: self];
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification 
{
    @autoreleasepool 
    {
        NSMenuItem *AppMenuItem = [[NSMenuItem alloc] initWithTitle: GameName
                                                             action: NULL
                                                      keyEquivalent: @""];

        NSMenu *AppMenu = [[NSMenu alloc] initWithTitle: GameName];
        NSString *QuitTitle = [NSString stringWithFormat: @"Quit %@", GameName];
        NSMenuItem *QuitMenuItem = [[NSMenuItem alloc] initWithTitle: QuitTitle
                                                              action: @selector(terminate:)
                                                       keyEquivalent: @"q"];
        [QuitMenuItem setTarget: NSApp];

        [AppMenu addItem: QuitMenuItem];
        [AppMenuItem setSubmenu: AppMenu];

        NSMenu *RootMenu = [[NSMenu alloc] initWithTitle: @"RootMenu"];
        RootMenu.autoenablesItems = true;
        [RootMenu addItem: AppMenuItem];
        [NSApp setMainMenu: RootMenu];
    }
}

@end

static CVReturn displayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp *inNow, const CVTimeStamp *inOutputTime, 
                                    CVOptionFlags flagsIn, CVOptionFlags *flagsOut, void *displayLinkContext) 
{
    if (!Quitting)
    {
        MetalViewDelegate *ViewDelegate = (__bridge MetalViewDelegate *)displayLinkContext;
        [ViewDelegate drawInMTKView: ViewDelegate.View];
    } else
    {
        dispatch_semaphore_signal(QuitGameSemaphore);
    }

    return kCVReturnSuccess;
}

#if STEAMSTORE
global_variable SteamCallbackThingy SteamThing = {};
#endif

global_variable game_render_commands RenderCommands = {}; 

int main(int argc, const char * argv[]) 
{
    NSBundle *MainBundle = [NSBundle mainBundle];
    NSString *ExecutablePath = [MainBundle executablePath];
    NSString *ExecutableDirectoryPath = [ExecutablePath stringByDeletingLastPathComponent];

    MainWindowDelegate *WindowDelegate = [[MainWindowDelegate alloc] init];

    NSRect ScreenRect = [[NSScreen mainScreen] frame];

    r32 GlobalRenderWidth = (r32)ScreenRect.size.width;
    r32 GlobalRenderHeight = (r32)ScreenRect.size.height;

    NSRect InitialFrame = NSMakeRect(0, 0, GlobalRenderWidth, GlobalRenderHeight);
    GameName = @"GameName";

#if LEVELEDITOR 
    Window = [[GameWindow alloc] 
                initWithContentRect: InitialFrame
                styleMask: NSWindowStyleMaskTitled |
                           NSWindowStyleMaskClosable |
                           NSWindowStyleMaskResizable
                  backing: NSBackingStoreBuffered
                    defer: NO];    
#else 
    Window = [[GameWindow alloc] 
                initWithContentRect: InitialFrame
                          styleMask: NSWindowStyleMaskBorderless |
                                     NSWindowStyleMaskFullSizeContentView                   
                            backing: NSBackingStoreBuffered
                              defer: YES];    
    [Window setLevel: NSMainMenuWindowLevel+1];
#endif

    [Window setBackgroundColor: NSColor.blackColor];
    [Window setTitle: GameName];
    [Window setDelegate: WindowDelegate];

    CGRect GameViewFrame = CGRectMake(0, 0, GlobalRenderWidth, GlobalRenderHeight); 
    CustomMetalView *GameView = [[CustomMetalView alloc] initWithFrame: GameViewFrame]; 
    GameView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

    RenderCommands.ViewportWidth = (int)GlobalRenderWidth*[Window backingScaleFactor];
    RenderCommands.ViewportHeight = (int)GlobalRenderHeight*[Window backingScaleFactor];
    RenderCommands.FrameIndex = 0;

    id<MTLDevice> MetalDevice = MTLCreateSystemDefaultDevice();
    GameView.metalLayer.device = MetalDevice; 
    GameView.metalLayer.framebufferOnly = false;
    GameView.metalLayer.contentsGravity = kCAGravityResizeAspect;
    GameView.metalLayer.drawableSize = CGSizeMake(RenderCommands.ViewportWidth, 
                                                  RenderCommands.ViewportHeight);
    MTLPixelFormat ColorPixelFormat = MTLPixelFormatBGRA8Unorm;

    [Window setContentView: GameView];
    [Window makeKeyAndOrderFront: nil];

    u32 VertexBufferSize = sizeof(game_texture_vertex)*6;
    RenderCommands.VertexBuffer.Vertices = 
        (game_texture_vertex *)mmap(0, VertexBufferSize, PROT_READ | PROT_WRITE,
                                    MAP_PRIVATE | MAP_ANON, -1, 0);

    GameSetupVertexBuffer(&RenderCommands);

    id<MTLCommandQueue> CommandQueue = [MetalDevice newCommandQueue]; 

    id<MTLBuffer> MacGPUVertexBuffer = SetupAppleVertexBuffer(MetalDevice, CommandQueue, 
                                                              &RenderCommands, VertexBufferSize);

    id<MTLRenderPipelineState> PixelArtPipelineState = 
        BuildPixelArtPipelineState(MetalDevice, ColorPixelFormat);

    id<MTLBuffer> InstanceUniformsBuffer = 
        BuildAppleInstanceUniformsBuffer(MetalDevice, &RenderCommands.InstanceBuffer, 5000);

    u32 AllocationFlags = MAP_PRIVATE | MAP_ANON;

    MetalViewDelegate *ViewDelegate = [[MetalViewDelegate alloc] init];

    GameMemory = {};
    GameSetupMemory(&GameMemory);
    
    GameMemory.PlatformReadPNGFile = PlatformReadPNGFile;
    GameMemory.PlatformReadEntireFile = PlatformReadEntireFile;
    GameMemory.PlatformFreeFileMemory = PlatformFreeFileMemory;
    GameMemory.PlatformWriteEntireFile = PlatformWriteEntireFile;

    // TODO: (Ted)  Bring this back if I ever need to save a level or
    //              world while working from Mac OS.
    //GameMemory.PlatformSaveFileDialog = PlatformSaveFileDialog;

    GameMemory.PlatformLogMessage = PlatformLogMessage;

#if STEAMSTORE || MACAPPSTORE
    GameMemory.PlatformUnlockAchievement = PlatformUnlockAchievement;
#endif

    GameMemory.PlatformQuitGame = PlatformQuitGame;
    GameMemory.PlatformReadFileFromApplicationSupport = PlatformReadFileFromApplicationSupport;

    GameMemory.PermanentStoragePartition.Start = mmap(0, GameMemory.PermanentStoragePartition.Size,
                                                      PROT_READ | PROT_WRITE, AllocationFlags, -1, 0); 

    if (GameMemory.PermanentStoragePartition.Start == MAP_FAILED) 
    {
		printf("mmap error: %d  %s", errno, strerror(errno));
        [NSException raise: @"Game Memory Not Allocated"
                     format: @"Failed to allocate permanent storage"];
    }
   
    u8* TransientStorageAddress = ((u8*)GameMemory.PermanentStoragePartition.Start + 
                                        GameMemory.PermanentStoragePartition.Size);
    GameMemory.TransientStoragePartition.Start = mmap(TransientStorageAddress,
                                                      GameMemory.TransientStoragePartition.Size,
                                                      PROT_READ | PROT_WRITE,
                                                      AllocationFlags, -1, 0); 

    if (GameMemory.TransientStoragePartition.Start == MAP_FAILED) 
    {
		printf("mmap error: %d  %s", errno, strerror(errno));
        [NSException raise: @"Game Memory Not Allocated"
                     format: @"Failed to allocate transient storage"];
    }

    GameSetupMemoryPartitions(&GameMemory);
    GameSetupRenderer(&GameMemory, &RenderCommands);

    mac_game_controller MacKeyboardController = {};
    MacKeyboardController.UsesHatSwitch = false;
    [ViewDelegate setKeyboardControllerPtr: &MacKeyboardController];

    game_input Input = {};

    Input.FrameRateMultiplier = 1.0f;

    [ViewDelegate setInputPtr: &Input];

    [NSCursor hide];

// MARK Device Simulators (Only supported on Windows)
    device_simulator_settings DeviceSimulatorSettings = {};
    DeviceSimulatorSettings.DeviceType = SimulatedDeviceType_None;

// MARK: Initialize Memory
    GameInitializeMemory(&GameMemory, DeviceSimulatorSettings);

    dispatch_queue_t highPriorityQueue = dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0);

    __block id<MTLTexture> TileTextureAtlas;

    __block apple_sound_output AppleSoundOutput = {};
    __block game_sound_output_buffer SoundOutputBuffer = {};

    game_texture_map *TextureMap = (game_texture_map *)malloc(sizeof(game_texture_map));

    // Submit work to the queue
    dispatch_async(highPriorityQueue, ^{

        game_texture_buffer TextureBuffers[4];
        
        LoadPixelArtTextures(&GameMemory, TextureBuffers, TextureMap);
        
        TileTextureAtlas = SetupMetalTexture(MetalDevice, &TextureBuffers[TileTextureBufferIndex]);

        GameClearTransientMemory(&GameMemory.TransientStoragePartition);
        LoadSounds(&GameMemory);

        game_startup_config StartupConfig = GameGetStartupConfig();
        AppleInitSound(&AppleSoundOutput, StartupConfig);
        [ViewDelegate setAppleSoundOutputPtr: &AppleSoundOutput];

        SoundOutputBuffer.Samples = (s16*)calloc(StartupConfig.SoundOutputBufferArrayCount,
                                                 AppleSoundOutput.BytesPerSample); 
        SoundOutputBuffer.SamplesPerSecond = AppleSoundOutput.SamplesPerSecond;
        SoundOutputBuffer.SampleArrayCount = StartupConfig.SoundOutputBufferArrayCount;
        SoundOutputBuffer.SamplesToAdvanceBeforeWriting = 0;
        [ViewDelegate setSoundOutputBufferPtr: &SoundOutputBuffer];

        GamePostContentLoadSetup(&GameMemory, TextureMap, &RenderCommands);
        [ViewDelegate setTextureMapPtr: TextureMap];

        ViewDelegate.TextureAtlases = [[NSMutableArray alloc] init];
        [ViewDelegate.TextureAtlases addObject: TileTextureAtlas];
    });

#if STEAMSTORE
    SteamThing.GameState = (game_state*)GameMemory.PermanentStoragePartition.Start;

    steam_action_handles SteamActionHandles = {};
    InitializeSteamAndRequestUserStats(&SteamActionHandles);

    [ViewDelegate setSteamActionHandlesPtr: &SteamActionHandles];
#endif

    mac_game_controller MacGameController = {}; 
    MacInitGameControllers(&MacGameController); 
    [ViewDelegate setGameControllerPtr: &MacGameController];

    ViewDelegate.GameMemory = GameMemory; 
    ViewDelegate.RenderCommands = RenderCommands; 
    ViewDelegate.PixelArtPipelineState = PixelArtPipelineState;
    ViewDelegate.CommandQueue = CommandQueue;
    ViewDelegate.AppleVertexBuffer = MacGPUVertexBuffer;
    ViewDelegate.InstanceUniformsBuffer = InstanceUniformsBuffer;

    ViewDelegate.View = GameView;

    mach_timebase_info_data_t TimeBase = {};
    mach_timebase_info(&TimeBase);

    [ViewDelegate setTimeBasePtr: &TimeBase];
    [ViewDelegate configureMetal];

    AppDelegate *GameAppDelegate = [[AppDelegate alloc] init];
    [NSApp setDelegate: GameAppDelegate];

    CVDisplayLinkCreateWithActiveCGDisplays(&GlobalDisplayLink);
    CVDisplayLinkSetOutputCallback(GlobalDisplayLink, &displayLinkCallback, 
                                   (__bridge void *)ViewDelegate);
    CVDisplayLinkStart(GlobalDisplayLink);

    QuitGameSemaphore = dispatch_semaphore_create(0);

#if MACAPPSTORE
    GKLocalPlayer.localPlayer.authenticateHandler = 
        ^(NSViewController *viewController, NSError * _Nullable playerAuthError)
    {
        game_state *GameState = (game_state*)GameMemory.PermanentStoragePartition.Start;

        if (viewController != nil)
        {
            [Window.contentViewController presentViewControllerAsModalWindow: viewController];
        } else if (playerAuthError == nil)
        {
            GameState->GameCenterInitialized = true;
        }

        if (GameState->GameCenterInitialized)
        {
            LoadGameCenterAchievementsFromTheCloud(GameState);
        }
    };
#endif

    return NSApplicationMain(argc, argv);
}
