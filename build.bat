@echo off
REM ============================================================
REM  PHANTOM STRIKE - Build script (MSYS2 UCRT64 + CMake/Ninja)
REM  Double-click to build. Configures on first run, then builds.
REM ============================================================
setlocal
set MSYSTEM=UCRT64
set CHERE_INVOKING=1
set MSYS2_PATH_TYPE=inherit

REM MSYS2 install locations (junction C:\msys64 -> E:\...\msys64 also works)
if exist "E:\SteamLibrary\IInstalled_Programs\MSYS2_Environment\msys64\usr\bin\bash.exe" (
    set "BASH=E:\SteamLibrary\IInstalled_Programs\MSYS2_Environment\msys64\usr\bin\bash.exe"
) else if exist "C:\msys64\usr\bin\bash.exe" (
    set "BASH=C:\msys64\usr\bin\bash.exe"
) else (
    echo [ERROR] MSYS2 bash.exe not found. Install MSYS2 or edit this script.
    pause
    exit /b 1
)

REM Resolve project root from this script's location (works from any folder).
set "SCRIPT_DIR=%~dp0"
set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

REM Convert Windows path to MSYS2 path (C:\foo\bar -> /c/foo/bar).
set "MSYS_DIR=%SCRIPT_DIR:\=/%"
set "MSYS_DIR=%MSYS_DIR:C:=/c%"
set "MSYS_DIR=%MSYS_DIR:E:=/e%"
set "MSYS_DIR=%MSYS_DIR:D:=/d%"

echo [BUILD] Project: %SCRIPT_DIR%
echo [BUILD] MSYS path: %MSYS_DIR%

"%BASH%" -lc "cd '%MSYS_DIR%' && if [ ! -d build ]; then echo '[BUILD] First-time configure...'; cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release; fi && cmake --build build 2>&1"
echo.
echo [BUILD] Done. (exit code %ERRORLEVEL%)
pause
endlocal
