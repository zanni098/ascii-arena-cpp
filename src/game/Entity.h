// Entity.h - root of the entity hierarchy. Everything that moves, is drawn or
// can be hit derives from this class and implements update().
#pragma once

#include <cmath>
#include <cstdint>

#include "core/Canvas.h"
#include "core/Math.h"

namespace aa {

class World;

enum class Faction { Player, Enemy };

class Entity {
public:
    explicit Entity(Vec2 position = {}) : pos(position) {}
    virtual ~Entity() = default;

    Entity(const Entity&) = delete;
    Entity& operator=(const Entity&) = delete;

    // Advances one fixed simulation step. Pure virtual: every concrete entity
    // must define its own behaviour.
    virtual void update(World& world, float dt) = 0;

    // Default rendering: one glyph at the entity's rounded grid position.
    virtual void draw(Canvas& canvas, int originX, int originY) const;

    Vec2 pos;
    float radius = 0.5f;
    bool alive = true;
    bool hostile = true;
    unsigned int id = 0;
    wchar_t glyph_ = L'?';
    uint8_t color_ = color::DkGrey;
    float age_ = 0.0f;

protected:
    void moveBy(Vec2 delta) { pos += delta; }
};

}  // namespace aa
