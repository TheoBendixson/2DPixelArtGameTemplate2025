@echo off
set optimizer_flags= -O2
if "%~1" == "-debug" set optimizer_flags= -Zi

set ignored_warnings=-wd4805 -wd4244 -wd4018 -wd4838 -wd4700
set windows_build_path=..\..\build\windows

if not exist %windows_build_path% mkdir %windows_build_path%
pushd %windows_build_path%

:: Compile the asset packer
cl -WX -W3 -nologo %ignored_warnings% %optimizer_flags% -DWINDOWS=1 /FeMooselutionsAssetPacker^
           ..\..\code\windows_platform\windows_asset_packer_main.cpp user32.lib

del windows_asset_packer_main.obj >nul
popd

