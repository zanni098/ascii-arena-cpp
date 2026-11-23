#include "game/WaveDirector.h"

#include <algorithm>

#include "core/Rng.h"
#include "game/World.h"

namespace aa {

void WaveDirector::configure(Rng* rng, float difficulty) {
    rng_ = rng;
    difficulty_ = difficulty > 0.1f ? difficulty : 0.1f;
    reset();
}

void WaveDirector::reset() {
    wave_ = 0;
    restTimer_ = 1.2f;  // short breather before the first wave
    spawnTimer_ = 0.0f;
    spawnInterval_ = 1.0f;
    queue_.clear();
}

void WaveDirector::update(World& world, float dt) {
    if (!rng_) return;

    // Between waves: count down, then open the next one.
    if (restTimer_ > 0.0f) {
        restTimer_ -= dt;
        if (restTimer_ > 0.0f) return;
        beginWave(world);
        return;
    }

    waveElapsed_ += dt;

    // Streaming spawns from the queue.
    if (!queue_.empty()) {
        spawnTimer_ -= dt;
        if (spawnTimer_ <= 0.0f) {
            world.spawnEnemy(queue_.back());
            queue_.pop_back();
            spawnTimer_ = spawnInterval_;
        }
        return;
    }

    // The wave ends when the arena is clear, or after a hard time limit so a
    // single straggler hiding behind cover can never stall the run.
    const bool cleared = world.enemiesAlive() == 0;
    if (cleared || waveElapsed_ > 34.0f) {
        world.addScore(cleared ? 50 + wave_ * 12 : 20 + wave_ * 6);
        world.notifyWaveCleared(wave_, cleared);
        restTimer_ = 2.8f;
    }
}

void WaveDirector::beginWave(World& world) {
    ++wave_;
    queue_.clear();

    int count = static_cast<int>((6.0f + static_cast<float>(wave_) * 3.0f) * difficulty_);
    count = std::max(5, std::min(count, 40));

    const bool boss = (wave_ % 5 == 0);
    if (boss) {
        queue_.push_back(EnemyKind::Boss);
        count = std::max(3, count / 3);
    }

    for (int i = 0; i < count; ++i) {
        float roll = rng_->unit();
        if (wave_ >= 6 && roll < 0.18f) queue_.push_back(EnemyKind::Brute);
        else if (wave_ >= 3 && roll < 0.55f) queue_.push_back(EnemyKind::Spitter);
        else queue_.push_back(EnemyKind::Rusher);
    }

    // Randomise the release order so each wave feels different.
    for (std::size_t i = queue_.size(); i > 1; --i) {
        std::size_t j = static_cast<std::size_t>(rng_->rangeInt(0, static_cast<int>(i) - 1));
        std::swap(queue_[i - 1], queue_[j]);
    }

    waveElapsed_ = 0.0f;
    spawnInterval_ = std::max(0.22f, 0.9f - 0.05f * static_cast<float>(wave_));
    spawnTimer_ = 0.0f;
    world.notifyWaveStarted(wave_, boss);
}

}  // namespace aa
