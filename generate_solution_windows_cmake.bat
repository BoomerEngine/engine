@ECHO OFF

SET DIR=%cd%

build\bin\lua_build.exe -engineDir=%DIR% -generator=cmake
PAUSE
