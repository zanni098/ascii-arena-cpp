#include "game/Pickup.h"

#include "game/World.h"

namespace aa {

Pickup::Pickup(Vec2 position, PickupKind k) : Entity(position), kind(k) {
    radius = 0.55f;
    hostile = false;
    if (kind == PickupKind::Heal) {
        glyph_ = L'+';
        color_ = color::Green;
        amount = 22;
    } else {
        glyph_ = L'$';
        color_ = color::Yellow;
        amount = 25;
    }
}

void Pickup::update(World& /*world*/, float dt) {
    age_ += dt;
    life -= dt;
    if (life <= 0.0f) alive = false;
}

void Pickup::draw(Canvas& canvas, int originX, int originY) const {
    // Blink during the last two seconds so the player knows it is about to expire.
    if (life < 2.0f && static_cast<int>(age_ * 8.0f) % 2 == 0) return;
    Entity::draw(canvas, originX, originY);
}

}  // namespace aa
