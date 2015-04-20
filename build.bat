@echo off

@mkdir ..\build
pushd ..\build
cl -FC -Zi ..\source\win32_platform.cpp user32.lib Gdi32.lib
popd