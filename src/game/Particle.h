// Particle.h - short lived ASCII sparks. Stored in an ObjectPool because a busy
// wave spawns hundreds of them per second.
#pragma once

#include "core/Canvas.h"
#include "core/Math.h"
#include "core/ObjectPool.h"
#include "core/Rng.h"

namespace aa {

struct Particle {
    Vec2 pos;
    Vec2 vel;
    float life = 0.0f;
    float maxLife = 1.0f;
    wchar_t ch = L'.';
    uint8_t color = color::DkGrey;
    bool alive = true;
};

class ParticleSystem {
public:
    ParticleSystem();

    void clear() { pool_.clear(); }
    void update(float dt);
    void draw(Canvas& canvas, int originX, int originY);

    void emit(const Particle& p);
    void burst(Rng& rng, Vec2 at, int count, uint8_t color, float speed,
               const wchar_t* glyphSet = L".'`,");

    std::size_t count() const { return pool_.aliveCount(); }

private:
    ObjectPool<Particle> pool_;
};

}  // namespace aa
