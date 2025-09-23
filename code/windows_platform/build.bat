@echo off

set optimizer_flags= -Zi 
set development_flags= -DLEVELEDITOR=1
set debug_build=true
set steam_store=false
set store_flags=-DSTEAMSTORE=0 -DMACAPPSTORE=0
set pack_assets=true
set compile_shaders=true

for %%A in (%*) do (
    if "%%A" == "-release" set debug_build=false
    if "%%A" == "-steam" set steam_store=true
    if "%%A" == "-no-assets" set pack_assets=true
    if "%%A" == "-no-asset" set pack_assets=true
    if "%%A" == "-no-shaders" set compile_shaders=false
    if "%%A" == "-no-shader"  set compile_shaders=false
    if "%%A" == "-no" (
        set pack_assets=false
        set compile_shaders=false
    )
)

if %steam_store% == true (
    set store_flags=-DSTEAMSTORE=1 -DMACAPPSTORE=0
)

echo %store_flags%

if %debug_build% == false (
	set optimizer_flags= -O2 -DNDEBUG=1
    set development_flags= -DLEVELEDITOR=0
	echo Release Build
)

set ignored_warnings=-wd4805 -wd4244 -wd4018 -wd4838 -wd4700 -wd4996
set windows_build_path=..\..\build\windows
set included_libraries=user32.lib d3d11.lib gdi32.lib dxguid.lib ole32.lib dwmapi.lib shell32.lib XInput.lib

:: Compile the game's shaders
if %compile_shaders% == true (
	echo Compiling shaders...
	fxc.exe /nologo /T vs_5_0 /E vs /O3 /WX /Zpc /Ges /Fh pixel_art_vertex_shader.h /Vn pixel_art_vertex_shader /Qstrip_reflect /Qstrip_debug /Qstrip_priv pixel_art_shader.hlsl
	fxc.exe /nologo /T ps_5_0 /E ps /O3 /WX /Zpc /Ges /Fh pixel_art_fragment_shader.h /Vn pixel_art_fragment_shader /Qstrip_reflect /Qstrip_debug /Qstrip_priv pixel_art_shader.hlsl
)

echo Copying RC File...
copy "GameName.rc" %windows_build_path%\GameName.rc
echo RC File Copied

echo Creating Windows Build Path
if not exist %windows_build_path% mkdir %windows_build_path%
echo Windows Build Path Created

echo Pushing Windows Build Path
pushd %windows_build_path%
echo Pushed Windows Build Path

:: Pack the game's assets
set build_art_path=.\art
set build_levels_path=.\levels
set build_sounds_path=.\sounds

set resources_path=..\..\resources
set steam_binary_path=..\..\code\steam_library\redistributable_bin\win64\

echo Pack Assets Line
if %pack_assets% == true (
	echo Packing assets...
	
	:: Remove the game's assets
	if exist %build_art_path%    rmdir /s /q %build_art_path%
	if exist %build_levels_path%    rmdir /s /q %build_levels_path%
	if exist %build_sounds_path%    rmdir /s /q %build_sounds_path%

	:: Copy the game's assets
	mkdir %build_art_path%
	mkdir %build_levels_path%
    mkdir %build_sounds_path%

	copy %resources_path%\art\ .\art\ >nul
	copy %resources_path%\levels\ .\levels\ >nul
	copy %resources_path%\sounds\ .\sounds\ >nul

)
::echo:

echo Copying Steam Library DLLs
copy %steam_binary_path%\steam_api64.dll steam_api64.dll
echo Copied Steam Library DLLs

if %debug_build% == true (
	copy %resources_path%\steam_appid.txt steam_appid.txt
)

:: Compile the app resources
@echo on
rc.exe GameName.rc
@echo off

:: Compile the game
cl -WX -W3 -nologo %ignored_warnings% %optimizer_flags% %store_flags% -DWINDOWS=1 %development_flags% /Fe"GameName.exe"^
	    ..\..\code\windows_platform\windows_main.cpp GameName.res /link %steam_binary_path%\steam_api64.lib^
           %included_libraries%

if exist windows_main.obj del windows_main.obj >nul

popd

:: Manifest stuff for DPI-awareness
mt.exe -manifest "GameName.exe.manifest" -outputresource:"%windows_build_path%\GameName.exe;1" -nologo

