#include "game/Player.h"

#include <cmath>

#include "core/Config.h"
#include "game/Enemy.h"
#include "game/Particle.h"
#include "game/World.h"

namespace aa {

Player::Player() : Entity({}) {
    glyph_ = L'@';
    color_ = color::White;
    radius = 0.42f;
    hostile = false;
}

void Player::reset(const Config& config) {
    // Position is set by World::reset so the arena can place the player.
    pos = Vec2(0.0f, 0.0f);
    maxHp = 100.0f;
    hp = maxHp;
    invuln = 0.0f;
    speed = config.playerSpeed;
    damage = config.startDamage;
    fireInterval = config.startFireInterval;
    pierce = 0;
    extraShots = 0;
    dashCooldown = 1.3f;
    level = 1;
    xp = 0;
    xpToNext = 8;
    kills = 0;
    fireTimer = 0.0f;
    dashTimer = 0.0f;
    dashReady = 0.0f;
    aim = Vec2(0.0f, -1.0f);
    age_ = 0.0f;
    alive = true;
}

void Player::update(World& world, float dt) {
    age_ += dt;
    if (invuln > 0.0f) invuln -= dt;
    if (dashReady > 0.0f) dashReady -= dt;
    if (dashTimer > 0.0f) dashTimer -= dt;

    const InputState& in = world.input();

    // --- movement -----------------------------------------------------------
    Vec2 dir;
    if (in.down[K_W] || in.down[K_UP]) dir.y -= 1.0f;
    if (in.down[K_S] || in.down[K_DOWN]) dir.y += 1.0f;
    if (in.down[K_A] || in.down[K_LEFT]) dir.x -= 1.0f;
    if (in.down[K_D] || in.down[K_RIGHT]) dir.x += 1.0f;
    dir = dir.normalized();
    movingThisFrame = dir.lengthSq() > 0.0f;

    if (in.pressed[K_SPACE]) tryDash(world);

    float moveSpeed = speed * (isDashing() ? 2.6f : 1.0f);
    world.moveEntity(*this, dir * (moveSpeed * dt));

    // Keep the '@' inside the arena interior.
    float maxX = world.arenaWidth() - 0.55f;
    float maxY = world.arenaHeight() - 0.55f;
    pos.x = clampf(pos.x, 0.55f, maxX);
    pos.y = clampf(pos.y, 0.55f, maxY);

    if (movingThisFrame) aim = dir;

    tryShoot(world, dt);
}

void Player::tryDash(World& world) {
    if (dashReady > 0.0f) return;
    dashTimer = 0.16f;
    dashReady = dashCooldown;
    invuln = 0.3f;
    world.particles().burst(world.rng(), pos, 8, color::Cyan, 9.0f, L"'-.");
}

void Player::tryShoot(World& world, float dt) {
    if (fireTimer > 0.0f) fireTimer -= dt;
    if (fireTimer > 0.0f) return;

    Enemy* target = world.nearestEnemy(pos, 26.0f);
    if (!target) return;

    Vec2 toTarget = (target->pos - pos).normalized();
    if (toTarget.lengthSq() > 0.0f) aim = toTarget;

    const int shots = 1 + extraShots;
    const float spreadStep = 0.09f;
    for (int i = 0; i < shots; ++i) {
        float offset = (static_cast<float>(i) - (shots - 1) * 0.5f) * spreadStep;
        float c = std::cos(offset);
        float s = std::sin(offset);
        Vec2 dir{aim.x * c - aim.y * s, aim.x * s + aim.y * c};
        world.spawnPlayerShot(pos, dir, damage, pierce);
    }
    world.particles().burst(world.rng(), pos + aim * 0.8f, 2, color::Grey, 4.0f, L"'`");
    fireTimer = fireInterval;
}

void Player::takeDamage(World& world, int amount) {
    if (invuln > 0.0f || isDead()) return;
    hp -= static_cast<float>(amount);
    invuln = 0.55f;
    world.particles().burst(world.rng(), pos, 10, color::Red, 10.0f, L"*x+");
    if (hp <= 0.0f) {
        hp = 0.0f;
        world.particles().burst(world.rng(), pos, 26, color::White, 14.0f, L"@*+#");
    }
}

void Player::heal(int amount) {
    hp += static_cast<float>(amount);
    if (hp > maxHp) hp = maxHp;
}

void Player::draw(Canvas& canvas, int originX, int originY) const {
    if (isDead()) return;
    uint8_t col = color_;
    if (isDashing()) col = color::Cyan;
    else if (invuln > 0.0f && static_cast<int>(age_ * 22.0f) % 2 == 0) col = color::DkGrey;
    canvas.put(originX + static_cast<int>(std::lround(pos.x)),
               originY + static_cast<int>(std::lround(pos.y)), glyph_, col);
}

}  // namespace aa
