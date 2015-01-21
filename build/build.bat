@echo off
call C:\Applications\vs2012\VC\vcvarsall.bat
pushd .\build
cl -Zi -nologo -MTd -EHa- -DDEBUG ..\main.cpp
popd
START .\build\main.exe