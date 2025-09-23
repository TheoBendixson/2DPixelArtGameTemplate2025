// 
// By Ted Bendixson
//
// iOS Main

#import <UIKit/UIKit.h>
#import <CoreVideo/CoreVideo.h>
#import <QuartzCore/QuartzCore.h>
#import <QuartzCore/CoreAnimation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AVFoundation/AVFoundation.h>
#import <GameKit/GameKit.h>

#include "ios_main.h"
#include "../../code/shared_apple_platform/TextureShaderTypes.h"

NSString * _Nullable
GetIOSUserDataDirectory()
{
    NSFileManager *FileManager = [NSFileManager defaultManager];
    NSArray *DocumentDirectoryPaths = [FileManager URLsForDirectory: NSDocumentDirectory inDomains: NSUserDomainMask];
    NSURL *DocumentsDirectory = [DocumentDirectoryPaths firstObject];

    if (DocumentsDirectory)
    {
        return [DocumentsDirectory path];
    } 

    return nil;
}

#include "../../code/shared_apple_platform/apple_file_io.cpp"

read_file_result
LoadFileFromiOSUserDataDirectory(char *Filename)
{
    read_file_result Result = {};
    Result.Contents = NULL;
    Result.ContentsSize = 0;

    @autoreleasepool
    {
        NSString *UserDataDirectory = GetIOSUserDataDirectory();

        if (UserDataDirectory)
        {
            NSString *AppleFilename = [[NSString alloc] initWithCString: Filename encoding: NSUTF8StringEncoding];
            NSString *FilePath = [UserDataDirectory stringByAppendingPathComponent: AppleFilename];
            NSData *FileData = [[NSFileManager defaultManager] contentsAtPath: FilePath];
            Result.Contents = malloc(FileData.length);
            memcpy(Result.Contents, FileData.bytes, FileData.length);
            Result.ContentsSize = (u64)FileData.length;
        }

    }

    return Result;
}

void
SaveUserDataToGameCenterCloud(char *Filename, u64 FileSize, void *Memory)
{
    NSData *GameData = [NSData dataWithBytes: Memory length: FileSize];
    NSString *AppleFilename = [[NSString alloc] initWithCString: Filename encoding: NSUTF8StringEncoding]; 

    if (GameData) 
    {
        [[GKLocalPlayer localPlayer] saveGameData:GameData 
                                         withName:AppleFilename 
                                completionHandler:^(GKSavedGame * _Nullable savedGame, NSError * _Nullable error) 
        {
            if (error) 
            {
                NSLog(@"Error saving game data: %@", error.localizedDescription);
            } else 
            {
                NSLog(@"Game data saved successfully.");
            }
        }];
    } else 
    {
        NSLog(@"Failed to encode game data.");
    }
}

void
FetchUserSaveDataFromiCloudMergeWithLocalAndPushToiCloud(game_state *GameState)
{
    [[GKLocalPlayer localPlayer] fetchSavedGamesWithCompletionHandler:^(NSArray<GKSavedGame *> * _Nullable savedGames, NSError * _Nullable error) 
    {
        if (error) 
        {
            // Handle the error (e.g., log it or show an alert)
            NSLog(@"Error fetching saved games: %@", error.localizedDescription);
            return;
        }

        // Process the fetched saved games
        if (savedGames.count > 0) 
        {
            GKSavedGame *savedGame = [savedGames objectAtIndex: 0];
            [savedGame loadDataWithCompletionHandler:^(NSData * _Nullable data, NSError * _Nullable loadSavedGameError) 
            {
                if (loadSavedGameError) 
                {
                    // Handle the error (e.g., log it or notify the user)
                    NSLog(@"Error loading game data: %@", 
                          loadSavedGameError.localizedDescription);
                    return;
                }

                memcpy((void *)&GameState->LastSaveStateFromiCloud, data.bytes, data.length);
                GameState->CloudSyncNeedsProcessing = true;
                GameState->IOSCloudSaveState = IOSCloudSaveState_DataExistsOnICloudAndNeedsToBeMergedWithLocal;
            }];

        } else 
        {
            NSLog(@"No saved games found.");
            GameState->CloudSyncNeedsProcessing = true;
            GameState->IOSCloudSaveState = IOSCloudSaveState_NoDataExistsOnICloud;
        }
    }];
}

#include "../../code/shared_apple_platform/apple_logging.h"

static const NSUInteger kMaxInflightBuffers = 3;
static const size_t kAlignedPixelArtInstanceUniformsSize = (sizeof(texture_draw_command_instance_uniforms) & ~0xFF) + 0x100;

#include "../shared_apple_platform/metal_setup.h"

#include "../shared_platform_layer/shared_input.cpp"
#include "../shared_platform_layer/shared_renderer.cpp"
#include "../shared_platform_layer/shared_platform_layer.cpp"

#include "../../code/shared_apple_platform/apple_instance_buffer.cpp"
#include "../../code/shared_apple_platform/apple_sound.cpp"

#include "../game_library/game_main.cpp"

#include "../../code/shared_apple_platform/metal_view_delegate.m"

#include "../../code/shared_apple_platform/achievements.mm"

global_variable MetalViewDelegate *ViewDelegate;
global_variable game_render_commands RenderCommands = {}; 
global_variable game_input Input = {};
global_variable v2 GlobalRenderSize = {};
global_variable game_memory GameMemory;

@interface MetalViewController: UIViewController<UIGestureRecognizerDelegate>
@end

@implementation MetalViewController
{
    MTKView *_view;
}

- (void) loadView
{
    self.view = [[MTKView alloc] initWithFrame: [[UIScreen mainScreen] bounds]];
}

- (void) viewDidLoad
{
    // Set the view to use the default device
    _view = (MTKView *)self.view;
    _view.device = ViewDelegate.MetalDevice;

    _view.framebufferOnly = false;
    _view.layer.contentsGravity = kCAGravityResizeAspect;
    _view.drawableSize = CGSizeMake(RenderCommands.ViewportWidth,
                                    RenderCommands.ViewportHeight);
    self.view.backgroundColor = UIColor.whiteColor;
    _view.delegate = ViewDelegate;

    self.view.userInteractionEnabled = YES;

    UITapGestureRecognizer *tapGestureRecognizer = 
        [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTap:)];
    tapGestureRecognizer.cancelsTouchesInView = false;
    [self.view addGestureRecognizer:tapGestureRecognizer];

    UISwipeGestureRecognizer *SGRLeft = 
        [[UISwipeGestureRecognizer alloc] initWithTarget: self action:@selector(handleSwipe:)];
    SGRLeft.cancelsTouchesInView = false;
    SGRLeft.delegate = self;
    SGRLeft.direction = UISwipeGestureRecognizerDirectionLeft;

    UISwipeGestureRecognizer *SGRRight = 
        [[UISwipeGestureRecognizer alloc] initWithTarget: self action:@selector(handleSwipe:)];
    SGRRight.cancelsTouchesInView = false;
    SGRRight.delegate = self;
    SGRRight.direction = UISwipeGestureRecognizerDirectionRight;

    UISwipeGestureRecognizer *SGRUp = 
        [[UISwipeGestureRecognizer alloc] initWithTarget: self action:@selector(handleSwipe:)];
    SGRUp.cancelsTouchesInView = false;
    SGRUp.delegate = self;
    SGRUp.direction = UISwipeGestureRecognizerDirectionUp;

    UISwipeGestureRecognizer *SGRDown = 
        [[UISwipeGestureRecognizer alloc] initWithTarget: self action:@selector(handleSwipe:)];
    SGRDown.cancelsTouchesInView = false;
    SGRDown.delegate = self;
    SGRDown.direction = UISwipeGestureRecognizerDirectionDown;

    [self.view addGestureRecognizer:SGRLeft];
    [self.view addGestureRecognizer:SGRRight];
    [self.view addGestureRecognizer:SGRUp];
    [self.view addGestureRecognizer:SGRDown];

    UILongPressGestureRecognizer *LongPressGestureRecognizer = 
        [[UILongPressGestureRecognizer alloc] initWithTarget:self 
                                                      action:@selector(handleLongPress:)];
    LongPressGestureRecognizer.cancelsTouchesInView = false;
    LongPressGestureRecognizer.allowableMovement = 2000.0f;
    LongPressGestureRecognizer.delegate = self;
    [self.view addGestureRecognizer: LongPressGestureRecognizer];

    [GKLocalPlayer localPlayer].authenticateHandler = 
    ^(UIViewController * _Nullable viewController, NSError * _Nullable playerAuthError) 
    {
        game_state *GameState = (game_state*)GameMemory.PermanentStoragePartition.Start;

        if (viewController != nil) 
        {
            [self presentViewController:viewController animated:YES completion:nil];
        } else if (playerAuthError == nil) 
        {
            GameState->GameCenterInitialized = true;
        } 

        if (GameState->GameCenterInitialized)
        {
            LoadGameCenterAchievementsFromTheCloud(GameState);
            FetchUserSaveDataFromiCloudMergeWithLocalAndPushToiCloud(GameState);
        }
    };
}

- (BOOL) gestureRecognizer:(UIGestureRecognizer *) gestureRecognizer 
shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *) otherGestureRecognizer
{
    return true;
}

- (void)handleLongPress:(UILongPressGestureRecognizer *)longPressGestureRecognizer
{
    if (longPressGestureRecognizer.state == UIGestureRecognizerStateBegan)
    {
        Input.LongPressGestureState = LongPressGestureState_GestureRecognizerBegan;
    } 
}

- (void)handleTap:(UITapGestureRecognizer *)tapGestureRecognizer 
{
    if (tapGestureRecognizer.state == UIGestureRecognizerStateEnded)
    {
        CGPoint touchLocation = [tapGestureRecognizer locationInView:self.view];
        Input.TappedThisFrame = true;
        r32 Scale = (r32)[UIScreen mainScreen].scale;
        Input.TapPosition = V2((r32)touchLocation.x, (r32)touchLocation.y)*Scale;
    }
}

- (void)handleSwipe:(UISwipeGestureRecognizer *)swipeGestureRecognizer
{
    direction SwipeDirection = Direction_None;

    if (swipeGestureRecognizer.direction == UISwipeGestureRecognizerDirectionLeft)
    {
        SwipeDirection = Direction_Left;
    } else if (swipeGestureRecognizer.direction == UISwipeGestureRecognizerDirectionRight)
    {
        SwipeDirection = Direction_Right;
    } else if (swipeGestureRecognizer.direction == UISwipeGestureRecognizerDirectionUp)
    {
        SwipeDirection = Direction_Up;
    } else if (swipeGestureRecognizer.direction == UISwipeGestureRecognizerDirectionDown)
    {
        SwipeDirection = Direction_Down;
    }

    Input.SwipeDirection = SwipeDirection;

    if (swipeGestureRecognizer.state == UIGestureRecognizerStateBegan)
    {
        Input.SwipeGestureState = SwipeGestureState_Began;
    } else if (swipeGestureRecognizer.state == UIGestureRecognizerStateEnded)
    {
        Input.SwipeGestureState = SwipeGestureState_Ended;
    }
}

- (void) touchesBegan:(NSSet<UITouch *> *) touches 
            withEvent:(UIEvent *) event
{
    [super touchesBegan:touches withEvent:event];

    CGPoint TouchLocation = [[touches.allObjects objectAtIndex: 0] locationInView: self.view];
    r32 Scale = (r32)[UIScreen mainScreen].scale;
    Input.TouchPos = V2((r32)TouchLocation.x, (r32)TouchLocation.y)*Scale;
    Input.LongPressGestureState = LongPressGestureState_TouchBegan;
    Input.FingerDown = true;
}

- (void) touchesMoved:(NSSet<UITouch *> *) touches 
            withEvent:(UIEvent *) event
{
    [super touchesMoved:touches withEvent:event];
    CGPoint TouchLocation = [[touches.allObjects objectAtIndex: 0] locationInView: self.view];
    r32 Scale = (r32)[UIScreen mainScreen].scale;
    Input.TouchPos = V2((r32)TouchLocation.x, (r32)TouchLocation.y)*Scale;
}

- (void) touchesEnded:(NSSet<UITouch *> *) touches 
            withEvent:(UIEvent *) event
{
    [super touchesEnded:touches withEvent:event];
    CGPoint TouchLocation = [[touches.allObjects objectAtIndex: 0] locationInView: self.view];
    r32 Scale = (r32)[UIScreen mainScreen].scale;
    Input.TouchPos = V2((r32)TouchLocation.x, (r32)TouchLocation.y)*Scale;
    Input.LongPressGestureState = LongPressGestureState_Ended;
    Input.FingerDown = false;
}

- (UIStatusBarStyle)preferredStatusBarStyle 
{
    return UIStatusBarStyleLightContent;
}

@end


@interface AppDelegate: UIResponder <UIApplicationDelegate>
@property (strong, nonatomic) UIWindow *window;
@end

@implementation AppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(id)options
{
    CGRect ScreenRect = [[UIScreen mainScreen] bounds];
    self.window = [[UIWindow alloc] initWithFrame: ScreenRect];

    MetalViewController *RVC = [[MetalViewController alloc] init];
    RVC.view.frame = ScreenRect;

    self.window.rootViewController = RVC;
    [self.window makeKeyAndVisible];

    return true;
}

@end


int main(int argc, char * argv[]) 
{
    CGRect ScreenRect = [[UIScreen mainScreen] bounds];

    GlobalRenderSize = V2((r32)ScreenRect.size.width, 
                          (r32)ScreenRect.size.height);
    
    CGFloat ScaleFactor = [UIScreen mainScreen].scale;
    RenderCommands.ViewportWidth = (int)GlobalRenderSize.X*ScaleFactor;
    RenderCommands.ViewportHeight = (int)GlobalRenderSize.Y*ScaleFactor;
    RenderCommands.FrameIndex = 0;

    id<MTLDevice> MetalDevice = MTLCreateSystemDefaultDevice();
    MTLPixelFormat ColorPixelFormat = MTLPixelFormatBGRA8Unorm;

    u32 VertexBufferSize = sizeof(game_texture_vertex)*6;
    RenderCommands.VertexBuffer.Vertices = (game_texture_vertex *)malloc(VertexBufferSize);
    GameSetupVertexBuffer(&RenderCommands);

    id<MTLCommandQueue> CommandQueue = [MetalDevice newCommandQueue]; 

    id<MTLBuffer> AppleGPUVertexBuffer = SetupAppleVertexBuffer(MetalDevice, CommandQueue, 
                                                                &RenderCommands, VertexBufferSize);

    id<MTLBuffer> InstanceUniformsBuffer = 
        BuildAppleInstanceUniformsBuffer(MetalDevice, &RenderCommands.InstanceBuffer, 5000);

    id<MTLRenderPipelineState> PixelArtPipelineState = BuildPixelArtPipelineState(MetalDevice, ColorPixelFormat);

    GameMemory = {};
    GameSetupMemory(&GameMemory);
    GameMemory.PlatformReadEntireFile = PlatformReadEntireFile;
    GameMemory.PlatformFreeFileMemory = PlatformFreeFileMemory;
    GameMemory.PlatformLogMessage = PlatformLogMessage;
    GameMemory.PlatformWriteEntireFile = PlatformWriteEntireFile;
    GameMemory.PlatformUnlockAchievement = PlatformUnlockAchievement;

    GameMemory.PermanentStoragePartition.Start = malloc(GameMemory.PermanentStoragePartition.Size); 

    if (!GameMemory.PermanentStoragePartition.Start)
    {
        [NSException raise: @"Game Memory Not Allocated"
                     format: @"Failed to allocate permanent storage"];
    }

    GameMemory.TransientStoragePartition.Start = malloc(GameMemory.TransientStoragePartition.Size); 

    if (!GameMemory.TransientStoragePartition.Start)
    {
        [NSException raise: @"Game Memory Not Allocated"
                     format: @"Failed to allocate permanent storage"];
    }

    GameSetupMemoryPartitions(&GameMemory);
    GameSetupRenderer(&GameMemory, &RenderCommands);

    ViewDelegate = [[MetalViewDelegate alloc] init];

    Input.FrameRateMultiplier = 1.0f;
    [ViewDelegate setInputPtr: &Input];

// MARK Device Simulators (Only supported on Windows)
    device_simulator_settings DeviceSimulatorSettings = {};
    DeviceSimulatorSettings.DeviceType = SimulatedDeviceType_None;

    {

        ios_device_type IOSDeviceType = IOSDeviceType_iPhone11;
        UIUserInterfaceIdiom InterfaceIdiom = [UIDevice currentDevice].userInterfaceIdiom;

        s32 ViewportWidth = RenderCommands.ViewportWidth;
        s32 ViewportHeight = RenderCommands.ViewportHeight;

        if (InterfaceIdiom == UIUserInterfaceIdiomPhone)
        {
            RenderCommands.ScreenDrawMode = ScreenDrawMode_CameraCenteredOnPlayer;

            if (ViewportWidth <= 750 &&
                ViewportHeight <= 1334)
            {
                IOSDeviceType = IOSDeviceType_iPhoneSE;
            } else if (ViewportWidth <= 828 &&
                       ViewportHeight <= 1792)
            {
                IOSDeviceType = IOSDeviceType_iPhone11;
            } else if (ViewportWidth <= 1179 &&
                       ViewportHeight <= 2556)
            {
                IOSDeviceType = IOSDeviceType_iPhone15;
            } else if (ViewportWidth <= 1290 &&
                       ViewportHeight <= 2796)
           {
                IOSDeviceType = IOSDeviceType_iPhone15Plus;
           }
        } else if (InterfaceIdiom == UIUserInterfaceIdiomPad)
        {
            IOSDeviceType = IOSDeviceType_iPad;
            RenderCommands.ScreenDrawMode = ScreenDrawMode_FitInsideScreen;
        }

        game_state *GameState = (game_state*)GameMemory.PermanentStoragePartition.Start;
        GameState->IOSDeviceType = IOSDeviceType;
        GameState->IOSScreenScale = (r32)[UIScreen mainScreen].scale;
    }

    GameInitializeMemory(&GameMemory, DeviceSimulatorSettings);

    id<MTLTexture> TileTextureAtlas;
    id<MTLTexture> HalfTileTextureAtlas;
    id<MTLTexture> TreeTextureAtlas; 

    apple_sound_output AppleSoundOutput = {};
    game_sound_output_buffer SoundOutputBuffer = {};

    game_texture_map *TextureMap = (game_texture_map *)malloc(sizeof(game_texture_map));
    game_texture_buffer TextureBuffers[4];

    LoadPixelArtTextures(&GameMemory, TextureBuffers, TextureMap);

    TileTextureAtlas = SetupMetalTexture(MetalDevice, &TextureBuffers[TileTextureBufferIndex]);
    HalfTileTextureAtlas = SetupMetalTexture(MetalDevice, &TextureBuffers[HalfTileTextureBufferIndex]);
    TreeTextureAtlas = SetupMetalTexture(MetalDevice, &TextureBuffers[TreeTextureBufferIndex]);

    GameClearTransientMemory(&GameMemory.TransientStoragePartition);
    LoadSounds(&GameMemory);

    game_startup_config StartupConfig = GameGetStartupConfig();

    AVAudioSession *AudioSession = [AVAudioSession sharedInstance];
   
    NSError *Error = NULL;
    [AudioSession setCategory:AVAudioSessionCategoryPlayback
                         mode:AVAudioSessionModeDefault
                      options:AVAudioSessionCategoryOptionMixWithOthers
                        error:&Error];
    
    if (Error) 
    {
        NSLog(@"Error setting category: %@", Error.localizedDescription);
    }

    [AudioSession setActive:YES error:&Error];
    
    if (Error) 
    {
        NSLog(@"Error activating audio session: %@", Error.localizedDescription);
    }

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
    [ViewDelegate.TextureAtlases addObject: HalfTileTextureAtlas];
    [ViewDelegate.TextureAtlases addObject: TreeTextureAtlas];

    ViewDelegate.MetalDevice = MetalDevice;
    ViewDelegate.CommandQueue = CommandQueue;
    ViewDelegate.GameMemory = GameMemory; 
    ViewDelegate.RenderCommands = RenderCommands; 
    ViewDelegate.PixelArtPipelineState = PixelArtPipelineState;
    ViewDelegate.AppleVertexBuffer = AppleGPUVertexBuffer;
    ViewDelegate.InstanceUniformsBuffer = InstanceUniformsBuffer;
    [ViewDelegate configureMetal];

    return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
}
