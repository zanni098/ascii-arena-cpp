cmake_minimum_required(VERSION 3.16)
project(ascii_arena LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_executable(ascii_arena
    src/main.cpp
    src/core/Platform.cpp
    src/core/Canvas.cpp
    src/core/Config.cpp
    src/game/Entity.cpp
    src/game/Projectile.cpp
    src/game/Particle.cpp
    src/game/Pickup.cpp
    src/game/Player.cpp
    src/game/Enemy.cpp
    src/game/World.cpp
    src/game/WaveDirector.cpp
    src/game/Upgrades.cpp
    src/game/Hud.cpp
    src/game/Scoreboard.cpp
    src/game/States.cpp
    src/game/Game.cpp
)

target_include_directories(ascii_arena PRIVATE src)

# Windows: build as a console app (no GUI subsystem), default charset Unicode.
if(WIN32)
    target_compile_definitions(ascii_arena PRIVATE UNICODE _UNICODE NOMINMAX)
endif()
