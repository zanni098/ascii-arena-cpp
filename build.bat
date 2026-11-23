@echo off
REM ASCII ARENA - Windows build script.
REM Works from a normal Command Prompt: finds Visual Studio by itself, and
REM falls back to MinGW g++ / clang++ when that is what you have installed.

setlocal enabledelayedexpansion
cd /d "%~dp0"

set "OUTDIR=build"
if not exist "%OUTDIR%" mkdir "%OUTDIR%"

REM ---------- compiler already on PATH? ------------------------------------
where cl >nul 2>nul
if not errorlevel 1 goto :msvc
where g++ >nul 2>nul
if not errorlevel 1 goto :mingw
where clang++ >nul 2>nul
if not errorlevel 1 goto :mingw

REM ---------- locate Visual Studio ----------------------------------------
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" goto :findvs
set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" goto :findvs
goto :nocompiler

:findvs
"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath > "%TEMP%\aa_vspath.txt" 2>nul
set "VSPATH="
set /p VSPATH=<"%TEMP%\aa_vspath.txt"
del "%TEMP%\aa_vspath.txt" >nul 2>nul
if not defined VSPATH goto :nocompiler
if not exist "!VSPATH!\VC\Auxiliary\Build\vcvars64.bat" goto :nocompiler

echo [build] using Visual Studio: !VSPATH!
call "!VSPATH!\VC\Auxiliary\Build\vcvars64.bat" >nul
where cl >nul 2>nul
if errorlevel 1 goto :nocompiler
goto :msvc

REM ---------- no toolchain -------------------------------------------------
:nocompiler
set "VSANY="
if not exist "%VSWHERE%" goto :plainmsg
"%VSWHERE%" -latest -products * -property installationPath > "%TEMP%\aa_vsany.txt" 2>nul
set /p VSANY=<"%TEMP%\aa_vsany.txt"
del "%TEMP%\aa_vsany.txt" >nul 2>nul

if defined VSANY goto :workloadmsg

:plainmsg
echo.
echo [build] No C++ compiler found on this machine.
echo.
echo   Fastest - just play: download the prebuilt exe from
echo       https://github.com/zanni098/ascii-arena-cpp/releases
echo.
echo   To build, install the free MSVC Build Tools WITH the C++ workload:
echo       winget install --id Microsoft.VisualStudio.2022.BuildTools -e --override "--wait --quiet --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
echo   then run build.bat again.
echo.
exit /b 1

:workloadmsg
echo.
echo [build] Visual Studio is installed, but without the C++ workload:
echo         !VSANY!
echo.
echo   Add the workload from a terminal (it will ask for elevation):
echo       winget install --id Microsoft.VisualStudio.2022.BuildTools -e --override "--wait --quiet --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
echo.
echo   Or open "Visual Studio Installer", click Modify on the Build Tools entry,
echo   tick "Desktop development with C++" and apply.
echo.
echo   Fastest meanwhile: prebuilt exe
echo       https://github.com/zanni098/ascii-arena-cpp/releases
echo.
exit /b 1

REM ---------- MSVC ---------------------------------------------------------
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
echo [build] compilation failed - copy the error above when asking for help.
exit /b 1
