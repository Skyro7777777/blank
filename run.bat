@echo off
REM ============================================================
REM  PHANTOM STRIKE - Run script
REM  Adds the MSYS2 UCRT64 bin folder to PATH so the .exe can
REM  find libgcc/libstdc++/libwinpthread, then launches the game.
REM ============================================================
setlocal

REM Locate MSYS2 UCRT64 bin (junction C:\msys64 -> E:\...\msys64 also works).
if exist "E:\SteamLibrary\IInstalled_Programs\MSYS2_Environment\msys64\ucrt64\bin" (
    set "MSYS_BIN=E:\SteamLibrary\IInstalled_Programs\MSYS2_Environment\msys64\ucrt64\bin"
) else if exist "C:\msys64\ucrt64\bin" (
    set "MSYS_BIN=C:\msys64\ucrt64\bin"
) else (
    echo [ERROR] MSYS2 ucrt64\bin not found. Install MSYS2 or edit this script.
    pause
    exit /b 1
)

set "PATH=%MSYS_BIN%;%PATH%"
cd /d "%~dp0build"

if not exist "PhantomStrike.exe" (
    echo [ERROR] PhantomStrike.exe not found in build folder.
    echo         Run build.bat first.
    pause
    exit /b 1
)

PhantomStrike.exe
endlocal
