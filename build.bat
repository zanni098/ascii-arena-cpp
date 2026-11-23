@echo off
REM ASCII ARENA - Windows build script.
REM Works from a normal Command Prompt: it locates Visual Studio by itself,
REM and falls back to MinGW g++ if that is what you have.

setlocal enabledelayedexpansion
cd /d "%~dp0"

set "OUTDIR=build"
if not exist "%OUTDIR%" mkdir "%OUTDIR%"

REM ---------- 1. is a compiler already on PATH? -----------------------------
where cl >nul 2>nul
if not errorlevel 1 goto :msvc
where g++ >nul 2>nul
if not errorlevel 1 goto :mingw
where clang++ >nul 2>nul
if not errorlevel 1 goto :mingw

REM ---------- 2. find Visual Studio and load its environment ----------------
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"

if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"
    if defined VSPATH (
        if exist "!VSPATH!\VC\Auxiliary\Build\vcvars64.bat" (
            echo [build] using Visual Studio: !VSPATH!
            call "!VSPATH!\VC\Auxiliary\Build\vcvars64.bat" >nul
        )
    )
)

where cl >nul 2>nul
if not errorlevel 1 goto :msvc

:noCompiler
echo.
echo [build] No C++ compiler found.
echo.
echo   Option A - install the free MSVC Build Tools (one command, no admin needed):
echo       winget install --id Microsoft.VisualStudio.2022.BuildTools -e
echo       then re-run build.bat
echo.
echo   Option B - just play: download the prebuilt ascii_arena.exe from
echo       https://github.com/zanni098/ascii-arena-cpp/releases
echo.
exit /b 1

REM ---------- MSVC ----------------------------------------------------------
:msvc
echo [build] compiling with MSVC ...
cl /nologo /std:c++17 /EHsc /O2 /W4 /utf-8 /Isrc ^
   src\main.cpp ^
   src\core\Platform.cpp src\core\Canvas.cpp src\core\Config.cpp ^
   src\game\Entity.cpp src\game\Projectile.cpp src\game\Particle.cpp ^
   src\game\Pickup.cpp src\game\Player.cpp src\game\Enemy.cpp ^
   src\game\World.cpp src\game\WaveDirector.cpp src\game\Upgrades.cpp ^
   src\game\Hud.cpp src\game\Scoreboard.cpp src\game\States.cpp src\game\Game.cpp ^
   /Fe:%OUTDIR%\ascii_arena.exe /Fo:%OUTDIR%\
if errorlevel 1 goto :failed
goto :built

REM ---------- MinGW / clang ------------------------------------------------
:mingw
echo [build] compiling with g++ ...
g++ -std=c++17 -O2 -Wall -Wextra -Isrc src\main.cpp src\core\*.cpp src\game\*.cpp -o %OUTDIR%\ascii_arena.exe
if errorlevel 1 goto :failed
goto :built

:built
echo.
echo [build] OK: %OUTDIR%\ascii_arena.exe
echo [build] run it with:  %OUTDIR%\ascii_arena.exe
exit /b 0

:failed
echo.
echo [build] compilation failed. Copy the error above when asking for help.
exit /b 1
