// States.h - State pattern: menu, play, pause, level-up and game over screens
// each own their input handling and overlay drawing.
#pragma once

#include <string>
#include <vector>

#include "core/Canvas.h"
#include "game/Upgrades.h"

namespace aa {

class Game;

enum class StateKind { Menu, Playing, Paused, Upgrade, GameOver };

class IGameState {
public:
    virtual ~IGameState() = default;
    virtual StateKind kind() const = 0;
    virtual void enter(Game& game) { (void)game; }
    virtual void update(Game& game, float dt) = 0;
    virtual void draw(Game& game, Canvas& canvas) = 0;
};

class MenuState : public IGameState {
public:
    StateKind kind() const override { return StateKind::Menu; }
    void enter(Game& game) override;
    void update(Game& game, float dt) override;
    void draw(Game& game, Canvas& canvas) override;

private:
    std::vector<std::string> options_{"START RUN", "HOW TO PLAY", "QUIT"};
    int selection_ = 0;
    bool showHelp_ = false;
    float time_ = 0.0f;
};

class PlayState : public IGameState {
public:
    StateKind kind() const override { return StateKind::Playing; }
    void update(Game& game, float dt) override;
    void draw(Game& game, Canvas& canvas) override;
};

class PauseState : public IGameState {
public:
    StateKind kind() const override { return StateKind::Paused; }
    void update(Game& game, float dt) override;
    void draw(Game& game, Canvas& canvas) override;
};

class UpgradeState : public IGameState {
public:
    StateKind kind() const override { return StateKind::Upgrade; }
    void enter(Game& game) override;
    void update(Game& game, float dt) override;
    void draw(Game& game, Canvas& canvas) override;

private:
    std::vector<Upgrade> options_;
    int selection_ = 0;
    float time_ = 0.0f;
};

class GameOverState : public IGameState {
public:
    StateKind kind() const override { return StateKind::GameOver; }
    void enter(Game& game) override;
    void update(Game& game, float dt) override;
    void draw(Game& game, Canvas& canvas) override;

private:
    std::string initials_;
    int rank_ = -1;
    bool submitted_ = false;
    bool qualifies_ = false;
    float time_ = 0.0f;
};

}  // namespace aa
