# 2D Pixel Art Game Template 

## What is it?
This is what I use as a starting point for game jams and game prototypes. It contains my most barebones game development setup for Windows and Mac OS (there's also some iOS-related platform layer stuff in there for those who are curious how I do that as well).

You can use this to make 2D pixel art games with a single texture atlas that supports 24px by 24px sprites, and sound mixing which supports .wav files at 48khz 16bit PCM audio.

If you have never made a game "from scratch" before and were curious how one person who is a mega fan of Casey Muratori / Jonathan Blow does it, this is for you. I won't explain who it isn't for, but if you think Clean Code is gospel and Bob is your uncle, there are plenty of other options for you :-).

I arrived at this template by spending years prototyping a bunch of different game concepts. I actually published one of those games (Mooselutions) to Steam on Windows and Mac OS. I've also used this to publish games to iOS. It is, at present, the most distilled form of my cross-platform game tool set.

## What's in it?

### Super Basic Pixel Art Rendering Pipeline (Windows, Mac OS, iOS)
- You give it a rectangle that you define in screen coordinates, it draws textured rectangles with support for z-layers (35 of them), alpha compositing, and rotation. 
- This uses a pixel art sampling shader that helps scale the pixel art while preserving that blocky look you know and love. You can scale up to any rendering target size, and it looks pretty good.
- This template starts with a base sprite size of 24x24, but it's not that much extra effort to put in different sized sprites and other atlases. The path has been paved for you.
- This uses STB libraries to load PNG files so you won't have to build that part either.

### Super Basic Input
- Keyboard and Mouse Buttons are supported by default
- There's a hint at how you might support game pads and game controllers using Steam Input and Official Apple libraries.

### Basic Audio Mixing (Windows, Mac OS, iOS)
- You give the sample a name, tell it to load, and it will play some audio for you. The default project is setup to play a sound when you left click, which you can use as an audio test.

### Basic Entity Setup, Structure, Update and Rendering Loops
- It's a simple generational entity system setup that starts with fat structs and behavior flags for easy prototyping. 
- Entity handles are usually a good idea especially when you're dealing with games where an entity you're tracking can get deleted from frame to frame.

### Basic Text Types and Text Rendering
- This is just enough to give some helpful instructions to the player (in English), but not so much that we've fallen into the rabbit hole of supporting things like Arabic or Chinese and therefore need to write a PHD dissertation.
- Text wrapping for longer expository blurbs (although maybe your game design isn't conveying your ideas well enough and you should consider not using this?).

### Simple Animations
- Sprite animation frame cycling
- Lerp animations

### Arena-Based Manual Memory Management
- This starts with a few memory arenas and dedicated partitions already setup for you.

### Window Management, Full Screen Support
- All of the Windows API annoyances are mostly taken care of. You get a borderless fullscreen window by default. If you press F11, it toggles fullscreen.
- The game runs windowed on Mac OS in debug build mode, but it runs full screen in release build mode. Command Q will quit the game, as expected.

### Dead Simple Procedural Build System with Debug and Release Build Configurations
- No Cmake, Xcode project reliance, or anything "magical." Just a build.bat/build.sh script file that you run once.
- Unity builds (not the engine) that build the whole game in less than a few seconds.

## Gotchas

### Mac OS
- You need to include a file called apple_environment.txt in your code/mac_platform directory. It should specify the name of the Xcode install you want to use for your SDK tools. Take a look at the build.sh script to fully understand how that file is used. It's spelled out pretty clearly. I use this on two different Mac computers with different Xcode installs, and I have it setup this way to seamlessly transition between the two computers without the need to configure anything. That's why the apple_environment.txt file isn't ordinarily checked in. But you don't have to do this! You can just refer to one Xcode SDK install and not use the fancy environment file.

- This includes an empty Xcode project for the game, but be aware that it's just a shell project so I can use Xcode as a debugger. You'll probably need to change the debug executable location if you want the debugger to work.

### iOS
- Use the iOS platform layer as a reference, not as an actual starting point. It's pretty good, but I need to clean it up first. It probably won't build without errors. 
- I don't usually ship most prototypes to iOS, so it's probably not relevant to your project anyway.  

## How to support this work
- You can [buy the game I built using this toolset, Mooselutions, on Steam](https://store.steampowered.com/app/2287140/Mooselutions/)
- You can also [buy Mooselutions on the App Store](https://apps.apple.com/us/app/mooselutions/id6477404960)

If you buy the game, and you play through it, please leave an honest review.

