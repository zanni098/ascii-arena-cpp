#include "game/Enemy.h"

#include <cmath>

#include "core/Rng.h"
#include "game/Particle.h"
#include "game/Player.h"
#include "game/World.h"

namespace aa {

Enemy::Enemy(Vec2 position, float health, float moveSpeed, int contact, int xp, int score)
    : Entity(position), hp(health), maxHp(health), speed(moveSpeed), contactDamage(contact),
      xpValue(xp), scoreValue(score) {
    radius = 0.45f;
    hostile = true;
}

void Enemy::update(World& world, float dt) {
    age_ += dt;
    phase += dt;
    if (flash > 0.0f) flash -= dt;
    if (contactTimer > 0.0f) contactTimer -= dt;

    if (lastPos.lengthSq() == 0.0f) lastPos = pos;

    if (escapeTimer > 0.0f) {
        // Wall-hugging escape: enemies slide sideways until they clear cover.
        escapeTimer -= dt;
        world.moveEntity(*this, escapeDir * (speed * dt));
    } else {
        behave(world, dt);
    }

    // If an enemy is pinned against cover it stops making progress; after a
    // moment of that, break away sideways instead of shoving the wall forever.
    float moved = distance(pos, lastPos);
    if (moved < speed * dt * 0.35f) stuckTimer += dt;
    else stuckTimer = 0.0f;
    lastPos = pos;

    if (stuckTimer > 0.7f && escapeTimer <= 0.0f) {
        Vec2 toPlayer = (world.player().pos - pos).normalized();
        Vec2 perp{-toPlayer.y, toPlayer.x};
        float sign = world.rng().chance(0.5f) ? 1.0f : -1.0f;
        escapeDir = perp * sign;
        escapeTimer = 0.85f;
        stuckTimer = 0.0f;
    }

    // Contact damage, shared by every enemy type.
    Player& player = world.player();
    if (!player.isDead() && distance(pos, player.pos) < radius + player.radius) {
        if (contactTimer <= 0.0f) {
            player.takeDamage(world, contactDamage);
            contactTimer = 0.9f;
        }
        // Soft separation so enemies do not stack on top of the player.
        world.moveEntity(*this, (pos - player.pos).normalized() * (dt * 5.0f));
    }

    pos.x = clampf(pos.x, radius, world.arenaWidth() - radius);
    pos.y = clampf(pos.y, radius, world.arenaHeight() - radius);
}

void Enemy::takeDamage(World& world, int amount, Vec2 from) {
    if (!alive) return;
    hp -= static_cast<float>(amount);
    flash = 0.08f;
    world.particles().burst(world.rng(), pos, 3, color::White, 6.0f, L"*'`");
    if (hp <= 0.0f) {
        alive = false;
        world.particles().burst(world.rng(), pos, isBoss() ? 40 : 10, color_, 11.0f,
                                isBoss() ? L"W#*@" : L"x*+");
    } else {
        // Light knockback away from the impact.
        Vec2 push = (pos - from);
        if (push.lengthSq() > 0.0001f && !isBoss()) pos += push.normalized() * 0.12f;
    }
}

void Enemy::draw(Canvas& canvas, int originX, int originY) const {
    uint8_t col = flash > 0.0f ? static_cast<uint8_t>(color::White) : color_;
    int x = originX + static_cast<int>(std::lround(pos.x));
    int y = originY + static_cast<int>(std::lround(pos.y));
    if (isBoss() && static_cast<int>(age_ * 6.0f) % 2 == 0) col = color::Yellow;
    canvas.put(x, y, glyph_, col);
}

// --- Rusher -----------------------------------------------------------------
Rusher::Rusher(Vec2 position, int wave, Rng& rng, float difficulty)
    : Enemy(position, 16.0f * (1.0f + 0.16f * (wave - 1)) * difficulty,
            4.9f + 0.22f * static_cast<float>(wave), 8, 2, 10) {
    glyph_ = L'r';
    color_ = color::Grey;
    radius = 0.4f;
    weavePhase_ = rng.range(0.0f, 6.28f);
    weave_ = rng.range(0.15f, 0.45f);
    if (speed > 11.0f) speed = 11.0f;
}

void Rusher::behave(World& world, float dt) {
    Vec2 dir = (world.player().pos - pos).normalized();
    Vec2 perp{-dir.y, dir.x};
    float w = std::sin(phase * 2.4f + weavePhase_) * weave_;
    dir = (dir + perp * w).normalized();
    world.moveEntity(*this, dir * (speed * dt));
}

// --- Spitter ----------------------------------------------------------------
Spitter::Spitter(Vec2 position, int wave, Rng& rng, float difficulty)
    : Enemy(position, 26.0f * (1.0f + 0.17f * (wave - 1)) * difficulty, 3.3f, 7, 3, 16) {
    glyph_ = L's';
    color_ = color::Yellow;
    radius = 0.45f;
    fireTimer_ = rng.range(0.6f, 1.6f);
    strafeDir_ = rng.chance(0.5f) ? 1.0f : -1.0f;
}

void Spitter::behave(World& world, float dt) {
    Vec2 toPlayer = world.player().pos - pos;
    float dist = toPlayer.length();
    Vec2 dir = toPlayer.normalized();
    Vec2 perp{-dir.y, dir.x};

    Vec2 move;
    if (dist > 11.0f) move = dir;                  // close the gap
    else if (dist < 6.5f) move = dir * -1.0f;      // back off
    else move = perp * strafeDir_;                 // hold the ring and strafe

    world.moveEntity(*this, move.normalized() * (speed * dt));

    fireTimer_ -= dt;
    if (fireTimer_ <= 0.0f && dist < 22.0f) {
        world.spawnEnemyShot(pos, dir, 8.5f, 8);
        fireTimer_ = 2.1f;
    }
    if (phase > 2.5f) {
        phase = 0.0f;
        strafeDir_ = -strafeDir_;
    }
}

// --- Brute ------------------------------------------------------------------
Brute::Brute(Vec2 position, int wave, Rng& rng, float difficulty)
    : Enemy(position, 78.0f * (1.0f + 0.18f * (wave - 1)) * difficulty, 2.55f, 16, 6, 34) {
    glyph_ = L'B';
    color_ = color::DkMagenta;
    radius = 1.0f;
    phase = rng.range(0.0f, 6.28f);
}

void Brute::behave(World& world, float dt) {
    Vec2 dir = (world.player().pos - pos).normalized();
    // Heavy, slightly lumbering movement.
    world.moveEntity(*this, dir * (speed * (0.85f + 0.25f * std::sin(phase * 1.7f)) * dt));
}

// --- Boss -------------------------------------------------------------------
Boss::Boss(Vec2 position, int wave, Rng& rng, float difficulty)
    : Enemy(position, 420.0f * (1.0f + 0.12f * (wave - 1)) * difficulty, 2.3f, 22, 25, 320) {
    glyph_ = L'W';
    color_ = color::Red;
    radius = 1.5f;
    volleyTimer_ = 1.8f;
    chargeTimer_ = 4.0f;
    phase = rng.range(0.0f, 6.28f);
}

void Boss::behave(World& world, float dt) {
    Vec2 dir = (world.player().pos - pos).normalized();

    // Charge cycle: drift slowly, then a telegraphed lunge at the player.
    if (chargeLeft_ > 0.0f) {
        chargeLeft_ -= dt;
        world.moveEntity(*this, chargeDir_ * (speed * 4.2f * dt));
        if (world.rng().chance(dt * 14.0f)) {
            world.particles().burst(world.rng(), pos, 1, color_, 5.0f, L".");
        }
    } else {
        world.moveEntity(*this, dir * (speed * dt));
        chargeTimer_ -= dt;
        if (chargeTimer_ <= 0.0f) {
            chargeTimer_ = 5.0f;
            chargeLeft_ = 0.55f;
            chargeDir_ = dir;
        }
    }

    volleyTimer_ -= dt;
    if (volleyTimer_ <= 0.0f) {
        volleyTimer_ = 2.6f;
        const int shots = 14;
        for (int i = 0; i < shots; ++i) {
            float a = static_cast<float>(i) * 6.2831853f / static_cast<float>(shots) + phase;
            world.spawnEnemyShot(pos, Vec2(std::cos(a), std::sin(a)), 7.0f, 9);
        }
        world.particles().burst(world.rng(), pos, 14, color_, 12.0f, L"o*+");
    }
}

// --- Factory ----------------------------------------------------------------
std::unique_ptr<Enemy> makeEnemy(EnemyKind kind, Vec2 position, int wave, Rng& rng,
                                 float difficulty) {
    switch (kind) {
        case EnemyKind::Rusher: return std::make_unique<Rusher>(position, wave, rng, difficulty);
        case EnemyKind::Spitter: return std::make_unique<Spitter>(position, wave, rng, difficulty);
        case EnemyKind::Brute: return std::make_unique<Brute>(position, wave, rng, difficulty);
        case EnemyKind::Boss: return std::make_unique<Boss>(position, wave, rng, difficulty);
    }
    return std::make_unique<Rusher>(position, wave, rng, difficulty);
}

const char* enemyName(EnemyKind kind) {
    switch (kind) {
        case EnemyKind::Rusher: return "RUSHER";
        case EnemyKind::Spitter: return "SPITTER";
        case EnemyKind::Brute: return "BRUTE";
        case EnemyKind::Boss: return "WARDEN";
    }
    return "?";
}

}  // namespace aa
