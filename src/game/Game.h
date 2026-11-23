// Game.h - application shell: fixed timestep loop, state stack, layout.
#pragma once

#include <memory>
#include <string>

#include "core/Canvas.h"
#include "core/Config.h"
#include "core/Platform.h"
#include "game/Hud.h"
#include "game/Scoreboard.h"
#include "game/World.h"

namespace aa {

class IGameState;

class Game {
public:
    Game(const Config& config, int cols, int rows);
    ~Game();

    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    void run(Terminal& term);

    // Deterministic, render-less run used by the automated smoke test.
    void runHeadless(double seconds, double dumpInterval, const std::string& dumpDir,
                     bool autoPickUpgrades);

    void startRun();
    void changeState(std::unique_ptr<IGameState> state);

    World& world() { return world_; }
    const World& world() const { return world_; }
    Canvas& canvas() { return canvas_; }
    const InputState& input() const { return input_; }
    const Config& config() const { return config_; }
    Scoreboard& scores() { return scores_; }
    const Hud& hud() const { return hud_; }

    bool headless() const { return headless_; }
    bool autoPickUpgrades() const { return autoUpgrade_; }
    bool quitRequested() const { return quit_; }
    void requestQuit() { quit_ = true; }

    int cols() const { return cols_; }
    int rows() const { return rows_; }
    int arenaOriginX() const { return 1; }
    int arenaOriginY() const { return 2; }

    double fps() const { return fps_; }
    int finalScore() const { return finalScore_; }
    int finalWave() const { return finalWave_; }
    void recordFinalScore() {
        finalScore_ = world_.score();
        finalWave_ = world_.wave();
    }

    void drawFrame();

    // Hooks used by the headless harness and the screenshot tool.
    void tick(float dt) { step(dt); }
    void setInput(const InputState& in) {
        input_ = in;
        world_.setInput(&input_);
    }
    void setAnsiDump(bool enabled) { ansiDump_ = enabled; }
    void setAutoPickUpgrades(bool enabled) { autoUpgrade_ = enabled; }

private:
    void step(float dt);
    void dumpFrame(const std::string& dir, int index);
    InputState demoInput() const;

    Config config_;
    int cols_ = 100;
    int rows_ = 34;
    Canvas canvas_;
    World world_;
    Hud hud_;
    Scoreboard scores_;
    std::unique_ptr<IGameState> state_;
    std::unique_ptr<IGameState> pendingState_;
    InputState input_;
    bool quit_ = false;
    bool headless_ = false;
    bool autoUpgrade_ = false;
    bool ansiDump_ = false;
    double fps_ = 0.0;
    int finalScore_ = 0;
    int finalWave_ = 0;
};

}  // namespace aa
