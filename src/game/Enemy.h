// Enemy.h - enemy hierarchy: one abstract base plus four concrete behaviours.
#pragma once

#include <memory>

#include "game/Entity.h"

namespace aa {

class Rng;

// Base class: owns health, contact damage and hit feedback; concrete enemies
// only implement their movement/attack pattern in behave().
class Enemy : public Entity {
public:
    Enemy(Vec2 position, float health, float moveSpeed, int contact, int xp, int score);

    void update(World& world, float dt) override;
    void draw(Canvas& canvas, int originX, int originY) const override;

    virtual void behave(World& world, float dt) = 0;
    virtual bool isBoss() const { return false; }

    void takeDamage(World& world, int amount, Vec2 from);

    float hp = 1.0f;
    float maxHp = 1.0f;
    float speed = 1.0f;
    int contactDamage = 0;
    int xpValue = 1;
    int scoreValue = 10;
    float contactTimer = 0.0f;
    float flash = 0.0f;
    float phase = 0.0f;
    float stuckTimer = 0.0f;
    float escapeTimer = 0.0f;
    Vec2 escapeDir{1.0f, 0.0f};
    Vec2 lastPos;
};

class Rusher : public Enemy {
public:
    Rusher(Vec2 position, int wave, Rng& rng, float difficulty);
    void behave(World& world, float dt) override;

private:
    float weave_ = 0.0f;
    float weavePhase_ = 0.0f;
};

class Spitter : public Enemy {
public:
    Spitter(Vec2 position, int wave, Rng& rng, float difficulty);
    void behave(World& world, float dt) override;

private:
    float fireTimer_ = 0.0f;
    float strafeDir_ = 1.0f;
};

class Brute : public Enemy {
public:
    Brute(Vec2 position, int wave, Rng& rng, float difficulty);
    void behave(World& world, float dt) override;
};

class Boss : public Enemy {
public:
    Boss(Vec2 position, int wave, Rng& rng, float difficulty);
    void behave(World& world, float dt) override;
    bool isBoss() const override { return true; }

private:
    float volleyTimer_ = 2.0f;
    float chargeTimer_ = 4.0f;
    float chargeLeft_ = 0.0f;
    Vec2 chargeDir_{1.0f, 0.0f};
};

enum class EnemyKind { Rusher, Spitter, Brute, Boss };

// Factory: applies wave scaling so the spawn director never touches stats.
std::unique_ptr<Enemy> makeEnemy(EnemyKind kind, Vec2 position, int wave, Rng& rng,
                                 float difficulty);

const char* enemyName(EnemyKind kind);

}  // namespace aa
