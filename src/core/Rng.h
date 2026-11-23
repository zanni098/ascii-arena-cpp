// Rng.h - deterministic xorshift generator. Deterministic seeding lets the
// headless tests replay a run exactly.
#pragma once

#include <cstdint>

namespace aa {

class Rng {
public:
    explicit Rng(uint32_t seed = 0x9E3779B9u) : state_(seed ? seed : 0x9E3779B9u) {}

    void seed(uint32_t s) { state_ = s ? s : 0x9E3779B9u; }

    uint32_t next() {
        uint32_t x = state_;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        state_ = x;
        return x;
    }

    // Uniform float in [0,1).
    float unit() { return static_cast<float>(next() & 0x00FFFFFFu) / 16777216.0f; }

    // Uniform float in [lo,hi).
    float range(float lo, float hi) { return lo + (hi - lo) * unit(); }

    // Uniform int in [lo,hi].
    int rangeInt(int lo, int hi) {
        if (hi <= lo) return lo;
        return lo + static_cast<int>(next() % static_cast<uint32_t>(hi - lo + 1));
    }

    bool chance(float p) { return unit() < p; }

private:
    uint32_t state_;
};

}  // namespace aa
