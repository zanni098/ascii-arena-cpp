@echo off
REM ASCII ARENA - one-line MSVC build.
REM Run from the "x64 Native Tools Command Prompt for VS 2019/2022".

setlocal
where cl >nul 2>nul
if errorlevel 1 (
    echo [error] cl.exe not found.
    echo         Open "x64 Native Tools Command Prompt for VS" and run build.bat again.
    exit /b 1
)

if not exist build mkdir build

echo Building ascii_arena.exe ...
cl /nologo /std:c++17 /EHsc /O2 /W4 /utf-8 /Isrc ^
    src\main.cpp ^
    src\core\Platform.cpp ^
    src\core\Canvas.cpp ^
    src\core\Config.cpp ^
    src\game\Entity.cpp ^
    src\game\Projectile.cpp ^
    src\game\Particle.cpp ^
    src\game\Pickup.cpp ^
    src\game\Player.cpp ^
    src\game\Enemy.cpp ^
    src\game\World.cpp ^
    src\game\WaveDirector.cpp ^
    src\game\Upgrades.cpp ^
    src\game\Hud.cpp ^
    src\game\Scoreboard.cpp ^
    src\game\States.cpp ^
    src\game\Game.cpp ^
    /Fe:build\ascii_arena.exe /Fo:build\

if errorlevel 1 (
    echo [error] build failed.
    exit /b 1
)

echo.
echo Built build\ascii_arena.exe
echo Run it with:  build\ascii_arena.exe
