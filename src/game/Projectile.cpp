#include "game/Projectile.h"

#include "game/World.h"

namespace aa {

Projectile::Projectile(Vec2 position, Vec2 direction, float speed, int dmg, Faction who,
                       wchar_t glyph, uint8_t col, int pierceCount, float lifetime)
    : Entity(position), damage(dmg), pierce(pierceCount), life(lifetime), owner(who) {
    glyph_ = glyph;
    color_ = col;
    radius = 0.35f;
    hostile = (who == Faction::Enemy);
    vel = direction.normalized() * speed;
}

void Projectile::update(World& world, float dt) {
    age_ += dt;
    moveBy(vel * dt);
    life -= dt;

    if (life <= 0.0f) {
        alive = false;
        return;
    }
    if (pos.x < -1.0f || pos.y < -1.0f || pos.x > world.arenaWidth() + 1.0f ||
        pos.y > world.arenaHeight() + 1.0f) {
        alive = false;
    }
}

void Projectile::draw(Canvas& canvas, int originX, int originY) const {
    // Player shots are drawn at full brightness; enemy shots blink slightly so
    // they read as hostile against the white player shots.
    uint8_t col = color_;
    if (owner == Faction::Enemy && static_cast<int>(age_ * 12.0f) % 2 == 0) col = color::DkRed;
    canvas.put(originX + static_cast<int>(std::lround(pos.x)),
               originY + static_cast<int>(std::lround(pos.y)), glyph_, col);
}

}  // namespace aa
