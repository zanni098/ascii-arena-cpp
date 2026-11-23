#include "game/Particle.h"

#include <cwchar>

namespace aa {

ParticleSystem::ParticleSystem() : pool_(768) { pool_.clear(); }

void ParticleSystem::update(float dt) {
    pool_.forEachAlive([dt](Particle& p) {
        p.life -= dt;
        if (p.life <= 0.0f) return false;
        p.pos += p.vel * dt;
        p.vel = p.vel * 0.94f;  // drag, so sparks slow down instead of flying off
        return true;
    });
}

void ParticleSystem::draw(Canvas& canvas, int originX, int originY) {
    pool_.forEachAlive([&](Particle& p) {
        if (p.life <= 0.0f) return false;
        // Fade by dropping to a dim palette colour in the second half of life.
        uint8_t col = p.color;
        if (p.life / p.maxLife < 0.45f && col >= 8) col = static_cast<uint8_t>(col - 8);
        canvas.put(originX + static_cast<int>(std::lround(p.pos.x)),
                   originY + static_cast<int>(std::lround(p.pos.y)), p.ch, col);
        return true;
    });
}

void ParticleSystem::emit(const Particle& p) {
    Particle* slot = pool_.acquire(p);
    (void)slot;  // pool recycles the oldest slot when full: silent drop is fine
}

void ParticleSystem::burst(Rng& rng, Vec2 at, int count, uint8_t color, float speed,
                           const wchar_t* glyphSet) {
    std::size_t glyphCount = glyphSet ? std::wcslen(glyphSet) : 0;
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.pos = at;
        float angle = rng.range(0.0f, 6.2831853f);
        float mag = rng.range(speed * 0.25f, speed);
        p.vel = Vec2(std::cos(angle) * mag, std::sin(angle) * mag);
        p.maxLife = rng.range(0.22f, 0.5f);
        p.life = p.maxLife;
        p.color = color;
        p.ch = glyphCount > 0 ? glyphSet[rng.rangeInt(0, static_cast<int>(glyphCount) - 1)] : L'.';
        emit(p);
    }
}

}  // namespace aa
