@echo off

set windows_build_path=..\..\build\windows

mkdir %windows_build_path%
pushd %windows_build_path%
cl -Zi ..\..\code\windows_platform\windows_main.cpp
popd
