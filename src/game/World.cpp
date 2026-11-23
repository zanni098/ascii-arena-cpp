#include "game/World.h"

#include <algorithm>
#include <cmath>

#include "core/Canvas.h"

namespace aa {

World::World(const Config& config, int arenaWidth, int arenaHeight)
    : config_(config), arenaWidth_(arenaWidth), arenaHeight_(arenaHeight) {
    waves_.configure(&rng_, config_.difficultyScale);
    reset(config.seed);
}

void World::reset(unsigned int seed) {
    rng_.seed(seed);
    waves_.configure(&rng_, config_.difficultyScale);
    waves_.reset();

    enemies_.clear();
    projectiles_.clear();
    pickups_.clear();
    particles_.clear();

    player_.reset(config_);
    player_.pos = Vec2(static_cast<float>(arenaWidth_) * 0.5f,
                       static_cast<float>(arenaHeight_) * 0.5f);

    generateObstacles();
    generateDebris();

    elapsed_ = 0.0f;
    score_ = 0;
    pendingUpgrades_ = 0;
    nextId_ = 1;
    bannerText_ = "SURVIVE";
    bannerTimer_ = 1.6f;
    pickupFlashTimer_ = 0.0f;
}

bool World::blocked(Vec2 point, float radius) const {
    if (point.x - radius < 0.0f || point.y - radius < 0.0f ||
        point.x + radius > static_cast<float>(arenaWidth_) ||
        point.y + radius > static_cast<float>(arenaHeight_)) {
        return true;
    }
    for (const Obstacle& o : obstacles_) {
        float cx = clampf(point.x, o.x, o.x + o.w);
        float cy = clampf(point.y, o.y, o.y + o.h);
        float dx = point.x - cx;
        float dy = point.y - cy;
        if (dx * dx + dy * dy < radius * radius) return true;
    }
    return false;
}

void World::moveEntity(Entity& entity, Vec2 delta) {
    Vec2 p = entity.pos;
    Vec2 next = p;
    next.x += delta.x;
    if (!blocked(next, entity.radius)) p.x = next.x;
    next = p;
    next.y += delta.y;
    if (!blocked(next, entity.radius)) p.y = next.y;
    entity.pos = p;
}

void World::generateObstacles() {
    obstacles_.clear();
    const float cx = static_cast<float>(arenaWidth_) * 0.5f;
    const float cy = static_cast<float>(arenaHeight_) * 0.5f;

    const int target = 5 + rng_.rangeInt(0, 3);
    int attempts = 0;
    while (static_cast<int>(obstacles_.size()) < target && attempts++ < 250) {
        const int w = rng_.rangeInt(4, 7);
        const int h = rng_.rangeInt(2, 3);
        const int x = rng_.rangeInt(2, std::max(3, arenaWidth_ - w - 3));
        const int y = rng_.rangeInt(2, std::max(3, arenaHeight_ - h - 3));
        if (x + w >= arenaWidth_ - 2 || y + h >= arenaHeight_ - 2) continue;

        // Leave the player's spawn area open.
        if (static_cast<float>(x) < cx + 9.0f && static_cast<float>(x + w) > cx - 9.0f &&
            static_cast<float>(y) < cy + 5.0f && static_cast<float>(y + h) > cy - 5.0f) {
            continue;
        }

        bool clash = false;
        for (const Obstacle& o : obstacles_) {
            if (static_cast<float>(x) < o.x + o.w + 2.0f &&
                static_cast<float>(x + w) > o.x - 2.0f &&
                static_cast<float>(y) < o.y + o.h + 2.0f &&
                static_cast<float>(y + h) > o.y - 2.0f) {
                clash = true;
                break;
            }
        }
        if (clash) continue;
        obstacles_.push_back(Obstacle{static_cast<float>(x), static_cast<float>(y),
                                      static_cast<float>(w), static_cast<float>(h)});
    }
}

void World::generateDebris() {
    // Sparse floor grit: pure decoration, but it makes the arena read as a
    // textured ASCII place rather than an empty rectangle.
    debris_.clear();
    const int cells = arenaWidth_ * arenaHeight_;
    const int count = cells / 42;
    for (int i = 0; i < count; ++i) {
        Vec2 p(rng_.range(0.0f, static_cast<float>(arenaWidth_)),
               rng_.range(0.0f, static_cast<float>(arenaHeight_)));
        if (blocked(p, 0.7f)) continue;
        float dx = p.x - static_cast<float>(arenaWidth_) * 0.5f;
        float dy = p.y - static_cast<float>(arenaHeight_) * 0.5f;
        if (dx * dx * 0.15f + dy * dy < 30.0f) continue;  // keep the spawn clear
        debris_.push_back(p);
    }
}

const InputState& World::input() const {
    static const InputState empty;
    return input_ ? *input_ : empty;
}

void World::update(float dt) {
    elapsed_ += dt;
    if (bannerTimer_ > 0.0f) bannerTimer_ -= dt;
    if (pickupFlashTimer_ > 0.0f) pickupFlashTimer_ -= dt;

    waves_.update(*this, dt);
    player_.update(*this, dt);

    for (auto& enemy : enemies_) {
        if (enemy && enemy->alive) enemy->update(*this, dt);
    }
    for (auto& proj : projectiles_) {
        if (proj && proj->alive) proj->update(*this, dt);
    }
    for (auto& pickup : pickups_) {
        if (pickup && pickup->alive) pickup->update(*this, dt);
    }

    collide();
    reap();
    particles_.update(dt);
}

void World::collide() {
    for (auto& proj : projectiles_) {
        if (!proj || !proj->alive) continue;

        // Cover stops bullets: this is what makes the pillars matter.
        if (blocked(proj->pos, 0.05f)) {
            proj->alive = false;
            particles_.burst(rng_, proj->pos, 2, color::DkGrey, 4.0f, L"*.");
            continue;
        }

        if (proj->owner == Faction::Player) {
            for (auto& enemy : enemies_) {
                if (!enemy || !enemy->alive) continue;
                if (distance(proj->pos, enemy->pos) <= proj->radius + enemy->radius) {
                    enemy->takeDamage(*this, proj->damage, proj->pos);
                    --proj->pierce;
                    if (proj->pierce < 0) proj->alive = false;
                    break;  // one enemy per step, so pierce counts correctly
                }
            }
        } else if (!player_.isDead()) {
            if (distance(proj->pos, player_.pos) <= proj->radius + player_.radius) {
                player_.takeDamage(*this, proj->damage);
                proj->alive = false;
            }
        }
    }

    for (auto& pickup : pickups_) {
        if (!pickup || !pickup->alive) continue;
        if (distance(pickup->pos, player_.pos) <= 0.9f + player_.radius) {
            if (pickup->kind == PickupKind::Heal) {
                player_.heal(pickup->amount);
                particles_.burst(rng_, pickup->pos, 6, color::Green, 7.0f, L"+.");
            } else {
                addScore(pickup->amount);
                particles_.burst(rng_, pickup->pos, 6, color::Yellow, 7.0f, L"$.");
            }
            pickup->alive = false;
            pickupFlashTimer_ = 0.35f;
        }
    }
}

void World::reap() {
    for (auto& enemy : enemies_) {
        if (!enemy || enemy->alive) continue;
        score_ += enemy->scoreValue;
        ++player_.kills;
        addXp(enemy->xpValue);
        tryDropLoot(*enemy);
    }

    enemies_.erase(std::remove_if(enemies_.begin(), enemies_.end(),
                                  [](const std::unique_ptr<Enemy>& e) { return !e || !e->alive; }),
                   enemies_.end());
    projectiles_.erase(
        std::remove_if(projectiles_.begin(), projectiles_.end(),
                       [](const std::unique_ptr<Projectile>& p) { return !p || !p->alive; }),
        projectiles_.end());
    pickups_.erase(
        std::remove_if(pickups_.begin(), pickups_.end(),
                       [](const std::unique_ptr<Pickup>& p) { return !p || !p->alive; }),
        pickups_.end());
}

void World::tryDropLoot(const Enemy& enemy) {
    if (blocked(enemy.pos, 0.5f)) return;  // cover swallowed the drop point
    if (enemy.isBoss()) {
        spawnPickup(enemy.pos, PickupKind::Heal);
        spawnPickup(enemy.pos + Vec2(1.5f, 0.0f), PickupKind::Score);
        spawnPickup(enemy.pos - Vec2(1.5f, 0.0f), PickupKind::Score);
        return;
    }
    if (rng_.chance(0.14f)) spawnPickup(enemy.pos, PickupKind::Heal);
    if (rng_.chance(0.09f)) spawnPickup(enemy.pos, PickupKind::Score);
}

void World::draw(Canvas& canvas, int originX, int originY) {
    for (const Obstacle& o : obstacles_) {
        for (int yy = 0; yy < static_cast<int>(o.h); ++yy) {
            for (int xx = 0; xx < static_cast<int>(o.w); ++xx) {
                canvas.put(originX + static_cast<int>(o.x) + xx,
                           originY + static_cast<int>(o.y) + yy, yy == 0 ? L'=' : L'#',
                           yy == 0 ? color::Grey : color::DkGrey);
            }
        }
    }

    static const wchar_t kGrit[] = {L',', L'.', L'`'};
    for (std::size_t i = 0; i < debris_.size(); ++i) {
        canvas.put(originX + static_cast<int>(debris_[i].x),
                   originY + static_cast<int>(debris_[i].y), kGrit[i % 3], color::DkGrey);
    }

    for (auto& pickup : pickups_) {
        if (pickup && pickup->alive) pickup->draw(canvas, originX, originY);
    }
    for (auto& enemy : enemies_) {
        if (enemy && enemy->alive) enemy->draw(canvas, originX, originY);
    }
    for (auto& proj : projectiles_) {
        if (proj && proj->alive) proj->draw(canvas, originX, originY);
    }
    particles_.draw(canvas, originX, originY);
    player_.draw(canvas, originX, originY);
}

Enemy* World::nearestEnemy(Vec2 from, float maxDistance) const {
    Enemy* best = nullptr;
    float bestDist = maxDistance;
    for (const auto& enemy : enemies_) {
        if (!enemy || !enemy->alive) continue;
        float d = distance(from, enemy->pos);
        if (d < bestDist) {
            bestDist = d;
            best = enemy.get();
        }
    }
    return best;
}

int World::enemiesAlive() const {
    int count = 0;
    for (const auto& enemy : enemies_) {
        if (enemy && enemy->alive) ++count;
    }
    return count;
}

bool World::bossAlive() const {
    for (const auto& enemy : enemies_) {
        if (enemy && enemy->alive && enemy->isBoss()) return true;
    }
    return false;
}

void World::spawnPlayerShot(Vec2 pos, Vec2 dir, int damage, int pierce) {
    projectiles_.push_back(std::make_unique<Projectile>(pos, dir, 27.0f, damage, Faction::Player,
                                                      L'*', color::White, pierce, 2.2f));
    projectiles_.back()->id = nextId_++;
}

void World::spawnEnemyShot(Vec2 pos, Vec2 dir, float speed, int damage) {
    projectiles_.push_back(std::make_unique<Projectile>(pos, dir, speed, damage, Faction::Enemy,
                                                      L'o', color::Red, 0, 5.0f));
    projectiles_.back()->id = nextId_++;
}

void World::spawnEnemy(EnemyKind kind) {
    Vec2 spawn{1.0f, 1.0f};
    for (int attempt = 0; attempt < 16; ++attempt) {
        int side = rng_.rangeInt(0, 3);
        float margin = 1.4f;
        if (side == 0) spawn = Vec2(rng_.range(1.0f, arenaWidth_ - 1.0f), margin);
        else if (side == 1) spawn = Vec2(rng_.range(1.0f, arenaWidth_ - 1.0f), arenaHeight_ - margin);
        else if (side == 2) spawn = Vec2(margin, rng_.range(1.0f, arenaHeight_ - 1.0f));
        else spawn = Vec2(arenaWidth_ - margin, rng_.range(1.0f, arenaHeight_ - 1.0f));
        if (distance(spawn, player_.pos) > 10.0f && !blocked(spawn, 0.8f)) break;
    }
    spawnEnemyAt(kind, spawn);
}

void World::spawnEnemyAt(EnemyKind kind, Vec2 pos) {
    auto enemy = makeEnemy(kind, pos, wave(), rng_, config_.difficultyScale);
    enemy->id = nextId_++;
    enemies_.push_back(std::move(enemy));
}

void World::spawnPickup(Vec2 pos, PickupKind kind) {
    pickups_.push_back(std::make_unique<Pickup>(pos, kind));
    pickups_.back()->id = nextId_++;
}

void World::addXp(int amount) {
    player_.xp += amount;
    while (player_.xp >= player_.xpToNext) {
        player_.xp -= player_.xpToNext;
        ++player_.level;
        player_.xpToNext = 6 + player_.level * 4;
        ++pendingUpgrades_;
    }
}

void World::consumeUpgrade() {
    if (pendingUpgrades_ > 0) --pendingUpgrades_;
}

void World::notifyWaveStarted(int wave, bool boss) {
    if (boss) {
        bannerText_ = "WARDEN INBOUND";
        bannerTimer_ = 2.6f;
    } else {
        bannerText_ = "WAVE " + std::to_string(wave);
        bannerTimer_ = 2.0f;
    }
}

void World::notifyWaveCleared(int wave, bool cleared) {
    bannerText_ = "WAVE " + std::to_string(wave) + (cleared ? " CLEARED" : " COMPLETE");
    bannerTimer_ = 2.0f;
}

}  // namespace aa
