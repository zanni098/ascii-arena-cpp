// ASCII ARENA - entry point and command line handling.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

#include "core/Config.h"
#include "core/Platform.h"
#include "game/Game.h"
#include "game/States.h"

namespace {

void printUsage() {
    std::printf(
        "ASCII ARENA\n"
        "\n"
        "  ascii_arena [options]\n"
        "\n"
        "Play:\n"
        "  (no arguments)          start the game in the console\n"
        "\n"
        "Options:\n"
        "  --config <path>         config file (default: config.cfg)\n"
        "  --width <cols>          console width in characters  (default 100)\n"
        "  --height <rows>         console height in characters (default 34)\n"
        "  --seed <n>              fixed RNG seed for a reproducible run\n"
        "  --difficulty <f>        enemy health/count multiplier (default 1.0)\n"
        "  --no-auto-upgrade       (headless) pick upgrades interactively\n"
        "\n"
        "Automated run (no console drawing, used by the smoke test):\n"
        "  --demo <seconds>        simulate for N seconds with a scripted player\n"
        "  --dump <dir>            write ASCII frames into <dir>\n"
        "  --dump-ansi             include ANSI colour in the frame dumps\n"
        "  --dump-every <sec>      frame dump interval (default 0.5)\n"
        "  --screens <dir>         write one frame of every screen (for docs)\n");
}

bool startsWith(const std::string& s, const char* prefix) {
    return s.rfind(prefix, 0) == 0;
}

}  // namespace

int main(int argc, char** argv) {
    using namespace aa;

    std::string configPath = "config.cfg";
    double demoSeconds = 0.0;
    double dumpEvery = 0.5;
    std::string dumpDir;
    bool autoUpgrade = true;
    bool ansiDump = false;
    std::string screensDir;
    unsigned int seedOverride = 0;
    float difficulty = 0.0f;
    int width = 0;
    int height = 0;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto value = [&](const char* name) -> std::string {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "%s requires a value\n", name);
                std::exit(2);
            }
            return argv[++i];
        };

        if (arg == "--help" || arg == "-h") {
            printUsage();
            return 0;
        } else if (arg == "--config") {
            configPath = value("--config");
        } else if (arg == "--demo") {
            demoSeconds = std::atof(value("--demo").c_str());
        } else if (arg == "--dump") {
            dumpDir = value("--dump");
        } else if (arg == "--dump-ansi") {
            ansiDump = true;
        } else if (arg == "--dump-every") {
            dumpEvery = std::atof(value("--dump-every").c_str());
        } else if (arg == "--seed") {
            seedOverride = static_cast<unsigned int>(std::strtoul(value("--seed").c_str(), nullptr, 10));
        } else if (arg == "--difficulty") {
            difficulty = static_cast<float>(std::atof(value("--difficulty").c_str()));
        } else if (arg == "--width") {
            width = std::atoi(value("--width").c_str());
        } else if (arg == "--height") {
            height = std::atoi(value("--height").c_str());
        } else if (arg == "--screens") {
            screensDir = value("--screens");
        } else if (arg == "--no-auto-upgrade") {
            autoUpgrade = false;
        } else if (startsWith(arg, "--")) {
            std::fprintf(stderr, "unknown option: %s\n", arg.c_str());
            printUsage();
            return 2;
        }
    }

    Config config = Config::load(configPath);
    if (seedOverride != 0) config.seed = seedOverride;
    if (difficulty > 0.0f) config.difficultyScale = difficulty;
    if (width > 0) config.arenaWidth = width;
    if (height > 0) config.arenaHeight = height;

    if (!screensDir.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(screensDir, ec);
        if (ec) {
            std::fprintf(stderr, "cannot create %s\n", screensDir.c_str());
            return 2;
        }

        Game game(config, config.arenaWidth, config.arenaHeight);
        auto dump = [&](const std::string& name) {
            game.drawFrame();
            std::ofstream(screensDir + "/" + name + ".txt") << game.canvas().toPlainText();
            std::ofstream(screensDir + "/" + name + ".ans") << game.canvas().toAnsiString();
        };

        dump("01_menu");

        game.setAutoPickUpgrades(true);
        game.startRun();
        for (int i = 0; i < 60 * 32; ++i) {
            InputState in;
            int phase = (i / 70) % 4;
            in.down[K_D] = (phase == 0);
            in.down[K_S] = (phase == 1);
            in.down[K_A] = (phase == 2);
            in.down[K_W] = (phase == 3);
            game.setInput(in);
            game.tick(1.0f / 60.0f);
        }
        dump("02_gameplay");

        game.changeState(std::make_unique<PauseState>());
        game.tick(0.0f);
        dump("03_pause");

        game.changeState(std::make_unique<PlayState>());
        game.tick(0.0f);
        Player& player = game.world().player();
        if (!player.isDead()) {
            game.setAutoPickUpgrades(false);  // show the card screen
            game.world().addXp(player.xpToNext - player.xp);
            game.tick(0.0f);
            dump("04_upgrade");
        }

        game.changeState(std::make_unique<GameOverState>());
        game.tick(0.0f);
        dump("05_gameover");
        std::printf("screens written to %s\n", screensDir.c_str());
        return 0;
    }

    if (demoSeconds > 0.0) {
        Game game(config, config.arenaWidth, config.arenaHeight);
        game.setAnsiDump(ansiDump);
        game.runHeadless(demoSeconds, dumpEvery, dumpDir, autoUpgrade);
        return 0;
    }

    Terminal terminal;
    int cols = config.arenaWidth;
    int rows = config.arenaHeight;
    if (!terminal.init(&cols, &rows)) {
        std::fprintf(stderr, "could not attach to a console window\n");
        return 1;
    }

    if (cols < 60 || rows < 22) {
        terminal.shutdown();
        std::fprintf(stderr,
                     "Window too small (%dx%d). Please resize the console to at least "
                     "60x22 characters, ideally 100x34.\n",
                     cols, rows);
        return 1;
    }

    Game game(config, cols, rows);
    game.run(terminal);
    terminal.shutdown();
    return 0;
}
