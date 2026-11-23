#include "game/Entity.h"

#include <cmath>

namespace aa {

void Entity::draw(Canvas& canvas, int originX, int originY) const {
    canvas.put(originX + static_cast<int>(std::lround(pos.x)),
               originY + static_cast<int>(std::lround(pos.y)), glyph_, color_);
}

}  // namespace aa
