@echo off
call C:\Applications\vs2012\VC\vcvarsall.bat
pushd .\build
set INCLUDE_PATHS=/I"..\..\..\sdl\include"
set LIB_PATHS=/LIBPATH:"..\..\..\sdl\lib\x86"
set DEPENDENCIES="SDL2.lib" "SDL2main.lib"
cl %INCLUDE_PATHS% -Zi -nologo -MTd -EHa- -DDEBUG ..\main.cpp /link %LIB_PATHS% %DEPENDENCIES% /OUT:verlossen.exe user32.lib gdi32.lib winmm.lib
popd
START .\build\verlossen.exe
