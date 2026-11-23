// Hud.h - top status bar, boss bar, wave banner and control footer.
#pragma once

#include <string>

#include "core/Canvas.h"

namespace aa {

class World;

class Hud {
public:
    void drawStatus(Canvas& canvas, const World& world, bool showFps, double fps) const;
    void drawFooter(Canvas& canvas, const World& world, int totalRows) const;
    void drawBanner(Canvas& canvas, const World& world, int arenaWidth) const;
    void drawBossBar(Canvas& canvas, const World& world, int arenaWidth) const;

private:
    static std::wstring bar(float ratio, int width);
};

}  // namespace aa
