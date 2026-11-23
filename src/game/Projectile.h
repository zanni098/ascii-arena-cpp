// Projectile.h - straight-line shot used by both the player and ranged enemies.
#pragma once

#include "game/Entity.h"

namespace aa {

class Projectile : public Entity {
public:
    Projectile(Vec2 position, Vec2 direction, float speed, int damage, Faction owner,
               wchar_t glyph, uint8_t color, int pierce, float lifetime);

    void update(World& world, float dt) override;
    void draw(Canvas& canvas, int originX, int originY) const override;

    Vec2 vel;
    int damage = 1;
    int pierce = 0;  // extra enemies a shot can pass through
    float life = 3.0f;
    Faction owner = Faction::Player;
};

}  // namespace aa
