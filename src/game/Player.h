// Player.h - the '@' character. Movement, dash and auto-aimed shooting.
#pragma once

#include "game/Entity.h"

namespace aa {

struct Config;

class Player : public Entity {
public:
    Player();

    void update(World& world, float dt) override;
    void draw(Canvas& canvas, int originX, int originY) const override;

    void reset(const Config& config);

    // Survival
    float maxHp = 100.0f;
    float hp = 100.0f;
    float invuln = 0.0f;

    // Offence / mobility
    float speed = 16.0f;
    int damage = 10;
    float fireInterval = 0.42f;
    int pierce = 0;
    int extraShots = 0;
    float dashCooldown = 1.3f;

    // Progression
    int level = 1;
    int xp = 0;
    int xpToNext = 8;
    int kills = 0;

    // Runtime timers
    float fireTimer = 0.0f;
    float dashTimer = 0.0f;
    float dashReady = 0.0f;
    Vec2 aim{1.0f, 0.0f};
    bool movingThisFrame = false;

    bool isDashing() const { return dashTimer > 0.0f; }
    bool isDead() const { return hp <= 0.0f; }
    float dashCharge() const {
        if (dashCooldown <= 0.0f) return 1.0f;
        float t = 1.0f - dashReady / dashCooldown;
        return t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    }

    void takeDamage(World& world, int amount);
    void heal(int amount);

private:
    void tryDash(World& world);
    void tryShoot(World& world, float dt);
};

}  // namespace aa
