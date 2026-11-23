#include "game/Game.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <filesystem>
#include <iostream>

#include "game/States.h"

namespace aa {
namespace {
int clampInt(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
}

Game::Game(const Config& config, int cols, int rows)
    : config_(config),
      cols_(clampInt(cols, 60, 200)),
      rows_(clampInt(rows, 22, 60)),
      canvas_(cols_, rows_),
      world_(config_, cols_ - 2, rows_ - 4),
      state_(nullptr) {
    scores_.load("highscores.txt");
    state_ = std::make_unique<MenuState>();
    state_->enter(*this);
}

Game::~Game() = default;

void Game::changeState(std::unique_ptr<IGameState> state) {
    pendingState_ = std::move(state);
}

void Game::startRun() {
    unsigned int seed = config_.seed;
    if (seed == 0) {
        seed = static_cast<unsigned int>(
            std::chrono::steady_clock::now().time_since_epoch().count() & 0x7FFFFFFF);
    }
    world_.reset(seed);
    changeState(std::make_unique<PlayState>());
}

void Game::step(float dt) {
    if (state_) state_->update(*this, dt);
    if (pendingState_) {
        state_ = std::move(pendingState_);
        pendingState_.reset();
        if (state_) state_->enter(*this);
    }
}

void Game::drawFrame() {
    canvas_.clear(L' ', color::DkGrey);

    if (state_ && state_->kind() == StateKind::Menu) {
        state_->draw(*this, canvas_);
        return;
    }

    hud_.drawStatus(canvas_, world_, config_.showFps, fps_);

    // Arena frame: '#' wall all the way round, drawn beneath every entity.
    canvas_.box(0, 1, cols_, rows_ - 2, color::DkGrey);
    canvas_.put(0, 1, L'#', color::Grey);
    canvas_.put(cols_ - 1, 1, L'#', color::Grey);
    canvas_.put(0, rows_ - 2, L'#', color::Grey);
    canvas_.put(cols_ - 1, rows_ - 2, L'#', color::Grey);

    canvas_.setClip(arenaOriginX(), arenaOriginY(), cols_ - 2, rows_ - 4);
    world_.draw(canvas_, arenaOriginX(), arenaOriginY());
    canvas_.clearClip();
    hud_.drawBossBar(canvas_, world_, cols_ - 2);
    hud_.drawBanner(canvas_, world_, cols_ - 2);
    hud_.drawFooter(canvas_, world_, rows_);

    if (state_) state_->draw(*this, canvas_);
}

InputState Game::demoInput() const {
    InputState in;
    const World& world = world_;
    const Player& player = world.player();
    if (player.isDead()) return in;

    Vec2 desired{0.0f, 0.0f};
    const Enemy* threat = world.nearestEnemy(player.pos, 13.0f);
    if (threat) {
        Vec2 away = (player.pos - threat->pos).normalized();
        Vec2 perp{-away.y, away.x};
        desired = (away + perp * 0.85f).normalized();
    } else {
        Vec2 centre(world.arenaWidth() * 0.5f, world.arenaHeight() * 0.5f);
        desired = (centre - player.pos).normalized();
    }

    // Keep away from the walls: steer back towards the middle when close.
    const float margin = 6.0f;
    if (player.pos.x < margin) desired.x += 0.6f;
    if (player.pos.x > world.arenaWidth() - margin) desired.x -= 0.6f;
    if (player.pos.y < margin) desired.y += 0.6f;
    if (player.pos.y > world.arenaHeight() - margin) desired.y -= 0.6f;
    desired = desired.normalized();

    if (desired.x > 0.25f) in.down[K_D] = true;
    if (desired.x < -0.25f) in.down[K_A] = true;
    if (desired.y > 0.25f) in.down[K_S] = true;
    if (desired.y < -0.25f) in.down[K_W] = true;

    if (threat && distance(threat->pos, player.pos) < 4.5f && player.dashCharge() > 0.99f) {
        in.pressed[K_SPACE] = true;
        in.down[K_SPACE] = true;
    }
    return in;
}

void Game::dumpFrame(const std::string& dir, int index) {
    char name[256];
    std::snprintf(name, sizeof(name), "%s/frame_%04d.txt", dir.c_str(), index);
    {
        std::ofstream out(name);
        if (out) out << canvas_.toPlainText();
    }
    if (ansiDump_) {
        std::snprintf(name, sizeof(name), "%s/frame_%04d.ans", dir.c_str(), index);
        std::ofstream ansi(name);
        if (ansi) ansi << canvas_.toAnsiString();
    }
}

void Game::run(Terminal& term) {
    const double stepDt = 1.0 / 60.0;
    double last = term.nowSeconds();
    double accumulator = 0.0;
    double fpsAccum = 0.0;
    int fpsFrames = 0;

    while (!quit_) {
        const double frameStart = term.nowSeconds();
        double frameTime = frameStart - last;
        last = frameStart;
        if (frameTime > 0.25) frameTime = 0.25;
        accumulator += frameTime;

        input_ = term.poll();
        world_.setInput(&input_);

        int guard = 0;
        while (accumulator >= stepDt && guard++ < 6) {
            step(static_cast<float>(stepDt));
            accumulator -= stepDt;
        }

        fpsAccum += frameTime;
        ++fpsFrames;
        if (fpsAccum >= 0.4) {
            fps_ = static_cast<double>(fpsFrames) / fpsAccum;
            fpsAccum = 0.0;
            fpsFrames = 0;
        }

        drawFrame();
        canvas_.present(term);

        // Frame cap: never spin faster than targetFps.
        const double target = 1.0 / static_cast<double>(std::max(20, config_.targetFps));
        const double spent = term.nowSeconds() - frameStart;
        if (spent < target) term.sleepFor(target - spent);
    }
}

void Game::runHeadless(double seconds, double dumpInterval, const std::string& dumpDir,
                       bool autoPickUpgrades) {
    headless_ = true;
    if (!dumpDir.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(dumpDir, ec);
        if (ec) std::fprintf(stderr, "warning: could not create %s\n", dumpDir.c_str());
    }
    autoUpgrade_ = autoPickUpgrades;
    fps_ = static_cast<double>(config_.targetFps);
    startRun();

    const double stepDt = 1.0 / 60.0;
    double time = 0.0;
    double nextDump = 0.0;
    int frameIndex = 0;

    while (time < seconds && !quit_) {
        input_ = demoInput();
        world_.setInput(&input_);
        step(static_cast<float>(stepDt));
        time += stepDt;

        if (!dumpDir.empty() && time >= nextDump) {
            drawFrame();
            dumpFrame(dumpDir, frameIndex++);
            nextDump += dumpInterval;
        }

        if (state_ && state_->kind() == StateKind::GameOver) break;
    }

    const Player& p = world_.player();
    std::printf(
        "[demo] time=%.2fs score=%d wave=%d level=%d kills=%d hp=%.0f enemies=%d "
        "projectiles=%zu particles=%zu frames=%d\n",
        world_.elapsed(), world_.score(), world_.wave(), p.level, p.kills, p.hp,
        world_.enemiesAlive(), world_.projectiles().size(), world_.particles().count(),
        frameIndex);
}

}  // namespace aa
