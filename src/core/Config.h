// Config.h - tuning values loaded from a plain key=value file so the game can
// be rebalanced without a recompile (useful for demonstrating file input).
#pragma once

#include <string>

namespace aa {

struct Config {
    int arenaWidth = 100;   // 0 = fill the terminal
    int arenaHeight = 34;
    int targetFps = 60;
    float playerSpeed = 16.0f;
    float startFireInterval = 0.42f;
    int startDamage = 10;
    float difficultyScale = 1.0f;  // multiplier on enemy health and count
    unsigned int seed = 0;         // 0 = derived from the clock
    bool showFps = true;

    // Loads "config.cfg" next to the executable. Missing keys keep defaults and
    // a fresh file is written on first run.
    static Config load(const std::string& path);
    void save(const std::string& path) const;
};

}  // namespace aa
