#include "core/Config.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

namespace aa {
namespace {

std::string trim(const std::string& s) {
    std::size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    std::size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

bool parseBool(const std::string& v) {
    return v == "1" || v == "true" || v == "yes" || v == "on" || v == "TRUE";
}

}  // namespace

Config Config::load(const std::string& path) {
    Config cfg;
    std::ifstream in(path);
    if (!in) {
        cfg.save(path);  // first run: materialise defaults so they are editable
        return cfg;
    }

    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        std::size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = trim(line.substr(0, eq));
        std::string val = trim(line.substr(eq + 1));
        if (key.empty() || val.empty()) continue;

        try {
            if (key == "arena_width") cfg.arenaWidth = std::stoi(val);
            else if (key == "arena_height") cfg.arenaHeight = std::stoi(val);
            else if (key == "target_fps") cfg.targetFps = std::stoi(val);
            else if (key == "player_speed") cfg.playerSpeed = std::stof(val);
            else if (key == "start_fire_interval") cfg.startFireInterval = std::stof(val);
            else if (key == "start_damage") cfg.startDamage = std::stoi(val);
            else if (key == "difficulty_scale") cfg.difficultyScale = std::stof(val);
            else if (key == "seed") cfg.seed = static_cast<unsigned int>(std::stoul(val));
            else if (key == "show_fps") cfg.showFps = parseBool(val);
        } catch (const std::exception&) {
            std::cerr << "config: ignoring bad value for " << key << "\n";
        }
    }
    return cfg;
}

void Config::save(const std::string& path) const {
    std::ofstream out(path);
    if (!out) return;
    out << "# ASCII ARENA - runtime configuration\n"
        << "# Changes take effect on the next launch.\n"
        << "arena_width=" << arenaWidth << "\n"
        << "arena_height=" << arenaHeight << "\n"
        << "target_fps=" << targetFps << "\n"
        << "player_speed=" << playerSpeed << "\n"
        << "start_fire_interval=" << startFireInterval << "\n"
        << "start_damage=" << startDamage << "\n"
        << "difficulty_scale=" << difficultyScale << "\n"
        << "seed=" << seed << "\n"
        << "show_fps=" << (showFps ? 1 : 0) << "\n";
}

}  // namespace aa
