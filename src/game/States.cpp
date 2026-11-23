#include "game/States.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "core/Config.h"
#include "game/Game.h"
#include "game/Hud.h"
#include "game/Player.h"
#include "game/World.h"

namespace aa {
namespace {

const wchar_t* kTitle[] = {
    L" ###   ####  #### ##### #####      ###  ####  ##### #   #  ###  ",
    L"#   # #     #       #     #       #   # #   # #     ##  # #   # ",
    L"#####  ###  #       #     #       ##### ####  ####  # # # ##### ",
    L"#   #     # #       #     #       #   # #  #  #     #  ## #   # ",
    L"#   # ####   #### ##### #####     #   # #   # ##### #   # #   # ",
};
constexpr int kTitleLines = 5;
constexpr int kTitleWidth = 66;

int centeredX(int canvasWidth, std::size_t textLength) {
    int x = (canvasWidth - static_cast<int>(textLength)) / 2;
    return x < 0 ? 0 : x;
}

void panel(Canvas& canvas, int x, int y, int w, int h, uint8_t borderColor) {
    if (w <= 0 || h <= 0) return;
    canvas.fill(x, y, w, h, L' ', color::Black);
    canvas.box(x, y, w, h, borderColor);
}

std::string formatTime(float seconds) {
    int total = static_cast<int>(seconds);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%02d:%02d", total / 60, total % 60);
    return buf;
}

void drawScoreTable(Canvas& canvas, const Scoreboard& scores, int x, int y, int highlight) {
    canvas.text(x, y, "TOP SCORES", color::DkGrey);
    int row = y + 1;
    if (scores.entries().empty()) {
        canvas.text(x + 2, row, "no runs recorded yet", color::DkGrey);
        return;
    }
    for (std::size_t i = 0; i < scores.entries().size(); ++i) {
        const ScoreEntry& e = scores.entries()[i];
        uint8_t col = (static_cast<int>(i) == highlight) ? color::Yellow : color::Grey;
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%2d. %-3s %7d  wave %d", static_cast<int>(i) + 1,
                      e.initials.c_str(), e.score, e.wave);
        canvas.text(x, row + static_cast<int>(i), buf, col);
    }
}

}  // namespace

// ---------------------------------------------------------------------------
// Menu
// ---------------------------------------------------------------------------
void MenuState::enter(Game& game) {
    selection_ = 0;
    showHelp_ = false;
    game.scores().load("highscores.txt");
}

void MenuState::update(Game& game, float dt) {
    time_ += dt;
    const InputState& in = game.input();

    if (showHelp_) {
        if (in.pressed[K_ESC] || in.pressed[K_ENTER] || in.pressed[K_Q]) showHelp_ = false;
        return;
    }

    if (in.pressed[K_W] || in.pressed[K_UP]) {
        selection_ = (selection_ + static_cast<int>(options_.size()) - 1) %
                     static_cast<int>(options_.size());
    }
    if (in.pressed[K_S] || in.pressed[K_DOWN]) {
        selection_ = (selection_ + 1) % static_cast<int>(options_.size());
    }

    if (in.pressed[K_ENTER]) {
        switch (selection_) {
            case 0: game.startRun(); break;
            case 1: showHelp_ = true; break;
            default: game.requestQuit(); break;
        }
    }
    if (in.pressed[K_Q]) game.requestQuit();
}

void MenuState::draw(Game& game, Canvas& canvas) {
    const int width = game.cols();
    const int height = game.rows();

    if (width >= kTitleWidth + 4) {
        int titleX = centeredX(width, kTitleWidth);
        for (int i = 0; i < kTitleLines; ++i) {
            canvas.text(titleX, 2 + i, kTitle[i], color::White);
        }
    } else {
        // Narrow console: fall back to a plain title instead of clipping the logo.
        const wchar_t* simple = L"A S C I I   A R E N A";
        canvas.text(centeredX(width, std::wcslen(simple)), 3, simple, color::White);
        canvas.hline(centeredX(width, 19), 4, 19, L'=', color::DkGrey);
    }
    const wchar_t* subtitle = L"a survival arena drawn in pure ascii";
    canvas.text(centeredX(width, std::wcslen(subtitle)), 8, subtitle, color::DkGrey);

    if (showHelp_) {
        int w = std::min(64, width - 4);
        int h = 16;
        int x = (width - w) / 2;
        int y = (height - h) / 2;
        panel(canvas, x, y, w, h, color::Cyan);
        canvas.text(x + 3, y + 1, "HOW TO PLAY", color::White);
        const wchar_t* lines[] = {
            L"WASD / ARROWS   move",
            L"SPACE           dash (brief invulnerability)",
            L"SHOOTING        automatic - fires at the nearest enemy",
            L"1 / 2 / 3       choose a level-up upgrade",
            L"P               pause        Q  quit to menu",
            L"",
            L"r rusher   s spitter   B brute   W warden (boss)",
            L"+ heals you   $ bonus score",
            L"",
            L"Waves escalate. Wardens appear every 5th wave.",
        };
        for (int i = 0; i < 10; ++i) {
            canvas.text(x + 3, y + 3 + i, lines[i], color::Grey);
        }
        canvas.text(x + 3, y + h - 2, "enter / esc to go back", color::DkGrey);
        return;
    }

    int boxW = 40;
    int boxX = (width - boxW) / 2;
    int boxY = 11;
    panel(canvas, boxX, boxY, boxW, 9, color::DkGrey);
    for (std::size_t i = 0; i < options_.size(); ++i) {
        bool selected = static_cast<int>(i) == selection_;
        std::wstring wide(options_[i].begin(), options_[i].end());
        int x = boxX + 4;
        int y = boxY + 2 + static_cast<int>(i) * 2;
        canvas.put(x - 2, y, selected ? L'>' : L' ', color::Cyan);
        canvas.text(x, y, wide, selected ? color::White : color::Grey);
    }

    int tableY = boxY + 11;
    if (tableY + 9 < height - 1) {
        drawScoreTable(canvas, game.scores(), boxX + 2, tableY, -1);
    }

    const wchar_t* hint = L"W/S select   ENTER confirm   Q quit";
    canvas.text(centeredX(width, std::wcslen(hint)), height - 2, hint, color::DkGrey);
}

// ---------------------------------------------------------------------------
// Play
// ---------------------------------------------------------------------------
void PlayState::update(Game& game, float dt) {
    World& world = game.world();
    const InputState& in = game.input();

    if (in.pressed[K_P] || in.pressed[K_ESC]) {
        game.changeState(std::make_unique<PauseState>());
        return;
    }
    if (in.pressed[K_Q]) {
        game.recordFinalScore();
        game.changeState(std::make_unique<MenuState>());
        return;
    }

    world.update(dt);

    if (world.playerDead()) {
        game.recordFinalScore();
        game.changeState(std::make_unique<GameOverState>());
        return;
    }
    if (world.pendingUpgrades() > 0) {
        game.changeState(std::make_unique<UpgradeState>());
    }
}

void PlayState::draw(Game& /*game*/, Canvas& /*canvas*/) {
    // The world itself is drawn by Game::drawFrame before overlay states.
}

// ---------------------------------------------------------------------------
// Pause
// ---------------------------------------------------------------------------
void PauseState::update(Game& game, float dt) {
    (void)dt;
    const InputState& in = game.input();
    if (in.pressed[K_P] || in.pressed[K_ESC] || in.pressed[K_ENTER]) {
        game.changeState(std::make_unique<PlayState>());
        return;
    }
    if (in.pressed[K_Q]) {
        game.recordFinalScore();
        game.changeState(std::make_unique<MenuState>());
    }
}

void PauseState::draw(Game& game, Canvas& canvas) {
    int w = 44;
    int h = 7;
    int x = (game.cols() - w) / 2;
    int y = (game.rows() - h) / 2;
    panel(canvas, x, y, w, h, color::White);
    canvas.text(centeredX(game.cols(), 6), y + 2, "PAUSED", color::White);
    canvas.text(x + 4, y + 4, "P / ESC  resume      Q  abandon run", color::DkGrey);
}

// ---------------------------------------------------------------------------
// Level up
// ---------------------------------------------------------------------------
void UpgradeState::enter(Game& game) {
    selection_ = 0;
    time_ = 0.0f;
    options_ = rollUpgrades(game.world().rng(), game.world().player(), 3);
}

void UpgradeState::update(Game& game, float dt) {
    time_ += dt;
    World& world = game.world();

    auto choose = [&](std::size_t index) {
        if (index >= options_.size()) return;
        applyUpgrade(world.player(), options_[index].id);
        world.consumeUpgrade();
        game.changeState(world.pendingUpgrades() > 0 ? std::unique_ptr<IGameState>(
                                                           std::make_unique<UpgradeState>())
                                                     : std::unique_ptr<IGameState>(
                                                           std::make_unique<PlayState>()));
    };

    if (game.autoPickUpgrades()) {
        if (!options_.empty()) choose(0);
        else {
            world.consumeUpgrade();
            game.changeState(std::make_unique<PlayState>());
        }
        return;
    }

    if (options_.empty()) {
        world.consumeUpgrade();
        game.changeState(std::make_unique<PlayState>());
        return;
    }

    const InputState& in = game.input();
    if (in.pressed[K_W] || in.pressed[K_UP]) selection_ = (selection_ + 2) % 3;
    if (in.pressed[K_S] || in.pressed[K_DOWN]) selection_ = (selection_ + 1) % 3;
    if (in.pressed[K_1]) choose(0);
    if (in.pressed[K_2]) choose(1);
    if (in.pressed[K_3]) choose(2);
    if (in.pressed[K_ENTER]) choose(static_cast<std::size_t>(selection_));
}

void UpgradeState::draw(Game& game, Canvas& canvas) {
    int width = std::min(62, game.cols() - 4);
    int height = 5 + static_cast<int>(options_.size()) * 4;
    int x = (game.cols() - width) / 2;
    int y = (game.rows() - height) / 2;
    panel(canvas, x, y, width, height, color::Cyan);

    char header[64];
    std::snprintf(header, sizeof(header), "LEVEL %d - CHOOSE AN UPGRADE",
                  game.world().player().level);
    canvas.text(centeredX(game.cols(), std::strlen(header)), y + 1, header, color::White);

    for (std::size_t i = 0; i < options_.size(); ++i) {
        bool selected = static_cast<int>(i) == selection_;
        int row = y + 3 + static_cast<int>(i) * 4;
        char label[64];
        std::snprintf(label, sizeof(label), "[%d] %s", static_cast<int>(i) + 1,
                      options_[i].name);
        canvas.text(x + 3, row, label, selected ? color::Yellow : color::Grey);
        canvas.text(x + 7, row + 1, options_[i].description,
                    selected ? color::White : color::DkGrey);
    }
    canvas.text(x + 3, y + height - 2, "1/2/3 or enter to confirm", color::DkGrey);
}

// ---------------------------------------------------------------------------
// Game over
// ---------------------------------------------------------------------------
void GameOverState::enter(Game& game) {
    time_ = 0.0f;
    initials_.clear();
    rank_ = -1;
    submitted_ = false;

    game.recordFinalScore();
    const int score = game.finalScore();
    qualifies_ = score > 0 && game.scores().qualifies(score);

    // Automated runs must not pollute the player's score table.
    submitted_ = game.headless() || !qualifies_;
}

void GameOverState::update(Game& game, float dt) {
    time_ += dt;
    if (game.headless()) return;

    const InputState& in = game.input();
    const int score = game.finalScore();

    if (!submitted_) {
        for (wchar_t ch : in.typed) {
            if ((ch >= L'a' && ch <= L'z') || (ch >= L'A' && ch <= L'Z') ||
                (ch >= L'0' && ch <= L'9')) {
                if (initials_.size() < 3) {
                    wchar_t upper = (ch >= L'a' && ch <= L'z') ? static_cast<wchar_t>(ch - 32) : ch;
                    initials_.push_back(static_cast<char>(upper));
                }
            }
        }
        if (in.pressed[K_BACKSPACE] && !initials_.empty()) initials_.pop_back();
        if (in.pressed[K_ENTER] && !initials_.empty() && time_ > 0.25f) {
            rank_ = game.scores().submit(initials_, score, game.finalWave());
            submitted_ = true;
        }
        return;
    }

    if (time_ < 0.3f) return;
    if (in.pressed[K_R] || in.pressed[K_ENTER]) {
        game.startRun();
    } else if (in.pressed[K_Q] || in.pressed[K_ESC]) {
        game.changeState(std::make_unique<MenuState>());
    }
}

void GameOverState::draw(Game& game, Canvas& canvas) {
    int w = std::min(56, game.cols() - 4);
    int h = submitted_ ? 16 : 13;
    int x = (game.cols() - w) / 2;
    int y = (game.rows() - h) / 2;
    panel(canvas, x, y, w, h, color::Red);

    const wchar_t* title = L"GAME OVER";
    canvas.text(centeredX(game.cols(), 9), y + 1, title, color::White);

    const Player& p = game.world().player();
    char line[96];
    std::snprintf(line, sizeof(line), "score %d    wave %d    level %d    kills %d",
                  game.finalScore(), game.finalWave(), p.level, p.kills);
    canvas.text(centeredX(game.cols(), std::strlen(line)), y + 3, line, color::Grey);

    std::string timeStr = formatTime(game.world().elapsed());
    std::snprintf(line, sizeof(line), "survived %s", timeStr.c_str());
    canvas.text(centeredX(game.cols(), std::strlen(line)), y + 4, line, color::DkGrey);

    if (!submitted_) {
        std::string entry = "NEW HIGH SCORE - ENTER INITIALS: " + initials_ + "_";
        canvas.text(x + 3, y + 7, entry, color::Yellow);
        canvas.text(x + 3, y + 9, "letters or digits, backspace to fix, enter to save",
                    color::DkGrey);
        return;
    }

    int tableY = y + 7;
    drawScoreTable(canvas, game.scores(), x + 3, tableY, rank_);
    canvas.text(x + 3, y + h - 2, "R retry      Q menu", color::DkGrey);
}

}  // namespace aa
