// WaveDirector.h - decides what spawns, when, and when a wave is over.
#pragma once

#include <vector>

#include "game/Enemy.h"

namespace aa {

class World;
class Rng;

class WaveDirector {
public:
    void configure(Rng* rng, float difficulty);
    void reset();
    void update(World& world, float dt);

    int wave() const { return wave_; }
    bool resting() const { return restTimer_ > 0.0f; }
    float restRemaining() const { return restTimer_; }
    bool bossWave() const { return wave_ > 0 && wave_ % 5 == 0; }
    int queued() const { return static_cast<int>(queue_.size()); }

private:
    void beginWave(World& world);

    Rng* rng_ = nullptr;
    float difficulty_ = 1.0f;
    int wave_ = 0;
    float restTimer_ = 1.2f;
    float spawnTimer_ = 0.0f;
    float spawnInterval_ = 1.0f;
    float waveElapsed_ = 0.0f;
    std::vector<EnemyKind> queue_;
};

}  // namespace aa
