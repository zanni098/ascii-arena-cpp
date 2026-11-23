// World.h - owns the simulation: entities, collisions, waves and score.
// Rendering is delegated to Canvas, input is pushed in by the Game loop.
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "core/Config.h"
#include "core/Rng.h"
#include "game/Enemy.h"
#include "game/Particle.h"
#include "game/Pickup.h"
#include "game/Player.h"
#include "game/Projectile.h"
#include "game/WaveDirector.h"

namespace aa {

class Canvas;
class Entity;

// Axis aligned cover block, measured in arena cells.
struct Obstacle {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
};

class World {
public:
    World(const Config& config, int arenaWidth, int arenaHeight);

    void reset(unsigned int seed);

    void setInput(const InputState* input) { input_ = input; }
    const InputState& input() const;

    void update(float dt);
    void draw(Canvas& canvas, int originX, int originY);

    Player& player() { return player_; }
    const Player& player() const { return player_; }
    const std::vector<std::unique_ptr<Enemy>>& enemies() const { return enemies_; }
    const std::vector<std::unique_ptr<Projectile>>& projectiles() const { return projectiles_; }

    Rng& rng() { return rng_; }
    ParticleSystem& particles() { return particles_; }
    WaveDirector& waves() { return waves_; }
    const Config& config() const { return config_; }

    int arenaWidth() const { return arenaWidth_; }
    int arenaHeight() const { return arenaHeight_; }
    float elapsed() const { return elapsed_; }

    const std::vector<Obstacle>& obstacles() const { return obstacles_; }
    bool blocked(Vec2 point, float radius) const;
    // Moves an entity, sliding along cover instead of stopping dead.
    void moveEntity(Entity& entity, Vec2 delta);

    int wave() const { return waves_.wave(); }
    bool waveResting() const { return waves_.resting(); }
    float restRemaining() const { return waves_.restRemaining(); }
    bool bossWave() const { return waves_.bossWave(); }

    Enemy* nearestEnemy(Vec2 from, float maxDistance) const;
    int enemiesAlive() const;
    bool bossAlive() const;

    // --- spawning (used by entities, the wave director and the factory) -----
    void spawnPlayerShot(Vec2 pos, Vec2 dir, int damage, int pierce);
    void spawnEnemyShot(Vec2 pos, Vec2 dir, float speed, int damage);
    void spawnEnemy(EnemyKind kind);
    void spawnEnemyAt(EnemyKind kind, Vec2 pos);
    void spawnPickup(Vec2 pos, PickupKind kind);

    // --- scoring and progression -------------------------------------------
    int score() const { return score_; }
    void addScore(int amount) { score_ += amount; }
    void addXp(int amount);
    int pendingUpgrades() const { return pendingUpgrades_; }
    void consumeUpgrade();
    void clearUpgrades() { pendingUpgrades_ = 0; }

    void notifyWaveStarted(int wave, bool boss);
    void notifyWaveCleared(int wave, bool cleared);

    const std::string& bannerText() const { return bannerText_; }
    float bannerTimer() const { return bannerTimer_; }
    float pickupFlashTimer() const { return pickupFlashTimer_; }

    bool playerDead() const { return player_.isDead(); }

private:
    void collide();
    void reap();
    void tryDropLoot(const Enemy& enemy);
    void generateObstacles();
    void generateDebris();

    Config config_;
    int arenaWidth_ = 100;
    int arenaHeight_ = 34;
    Rng rng_;
    WaveDirector waves_;
    Player player_;
    std::vector<std::unique_ptr<Enemy>> enemies_;
    std::vector<std::unique_ptr<Projectile>> projectiles_;
    std::vector<std::unique_ptr<Pickup>> pickups_;
    std::vector<Obstacle> obstacles_;
    std::vector<Vec2> debris_;
    ParticleSystem particles_;
    const InputState* input_ = nullptr;
    float elapsed_ = 0.0f;
    int score_ = 0;
    int pendingUpgrades_ = 0;
    unsigned int nextId_ = 1;
    std::string bannerText_;
    float bannerTimer_ = 0.0f;
    float pickupFlashTimer_ = 0.0f;
};

}  // namespace aa
