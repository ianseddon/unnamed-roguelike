@echo off

@mkdir ..\build
pushd ..\build
cl -Zi ..\source\win32_rouge.cpp user32.lib
popd