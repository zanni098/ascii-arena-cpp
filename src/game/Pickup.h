// Pickup.h - heal and score drops left behind by killed enemies.
#pragma once

#include "game/Entity.h"

namespace aa {

enum class PickupKind { Heal, Score };

class Pickup : public Entity {
public:
    Pickup(Vec2 position, PickupKind kind);

    void update(World& world, float dt) override;
    void draw(Canvas& canvas, int originX, int originY) const override;

    PickupKind kind = PickupKind::Heal;
    int amount = 0;
    float life = 9.0f;
};

}  // namespace aa
