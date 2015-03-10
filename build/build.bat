@echo off
call C:\Applications\vs2012\VC\vcvarsall.bat
pushd ..\build
set INCLUDE_PATHS=/I"..\..\..\sdl\include" /I"../src"
set LIB_PATHS=/LIBPATH:"..\..\..\sdl\lib\x86"
set DEPENDENCIES="SDL2.lib" "SDL2main.lib" user32.lib gdi32.lib winmm.lib
set LINKER_OPTIONS=/DEBUG /SUBSYSTEM:CONSOLE /NODEFAULTLIB:"msvcrt.lib"
REM -MTd
cl %INCLUDE_PATHS% -Zi -nologo -MDd -EHa- -DDEBUG ..\src\test_client.cpp /link %LIB_PATHS% %DEPENDENCIES% %LINKER_OPTIONS% /OUT:client.exe
cl %INCLUDE_PATHS% -Zi -nologo -MDd -EHa- -DDEBUG ..\src\test_server.cpp /link %LIB_PATHS% %DEPENDENCIES% %LINKER_OPTIONS% /OUT:server.exe
popd
