@echo off
call C:\Applications\vs2012\VC\vcvarsall.bat
pushd .\build
set INCLUDE_PATHS=/I"..\..\..\sdl\include"
set LIB_PATHS=/LIBPATH:"..\..\..\sdl\lib\x86"
set DEPENDENCIES="SDL2.lib" "SDL2main.lib" user32.lib gdi32.lib winmm.lib
set LINKER_OPTIONS=/DEBUG /SUBSYSTEM:CONSOLE /NODEFAULTLIB:"msvcrt.lib"
REM -MTd
cl %INCLUDE_PATHS% -Zi -nologo -MDd -EHa- -DDEBUG ..\main.cpp /link %LIB_PATHS% %DEPENDENCIES% %LINKER_OPTIONS% /OUT:verlossen.exe
popd
REM START .\build\verlossen.exe 127.0.0.1:12345
