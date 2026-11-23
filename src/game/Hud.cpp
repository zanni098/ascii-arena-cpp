#include "game/Hud.h"

#include <cmath>
#include <cstdio>
#include <cwchar>

#include "game/Player.h"
#include "game/World.h"

namespace aa {
namespace {

std::wstring pad(const std::wstring& s, std::size_t width) {
    std::wstring out = s;
    while (out.size() < width) out.push_back(L' ');
    return out.substr(0, width);
}

}  // namespace

std::wstring Hud::bar(float ratio, int width) {
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;
    int filled = static_cast<int>(std::lround(ratio * static_cast<float>(width)));
    std::wstring out;
    for (int i = 0; i < width; ++i) out.push_back(i < filled ? L'#' : L'-');
    return out;
}

// The status bar is built left to right and drops optional fields on narrow
// consoles instead of overflowing into the next line.
void Hud::drawStatus(Canvas& canvas, const World& world, bool showFps, double fps) const {
    const Player& p = world.player();
    const int width = canvas.width();
    int y = 0;
    int x = 1;
    wchar_t buf[64];

    canvas.text(x, y, "HP", color::DkGrey);
    x += 3;
    float hpRatio = p.maxHp > 0.0f ? p.hp / p.maxHp : 0.0f;
    uint8_t hpColor = hpRatio > 0.6f ? color::Green : (hpRatio > 0.3f ? color::Yellow : color::Red);
    canvas.text(x, y, bar(hpRatio, 14), hpColor);
    x += 15;
    std::swprintf(buf, 64, L"%3d/%3d", static_cast<int>(std::ceil(p.hp)),
                  static_cast<int>(p.maxHp));
    canvas.text(x, y, std::wstring(buf), color::Grey);
    x += 9;

    canvas.text(x, y, "WAVE", color::DkGrey);
    x += 5;
    std::swprintf(buf, 64, L"%-2d", world.wave());
    canvas.text(x, y, std::wstring(buf), color::White);
    x += 4;

    canvas.text(x, y, "SCORE", color::DkGrey);
    x += 6;
    std::swprintf(buf, 64, L"%-7d", world.score());
    canvas.text(x, y, std::wstring(buf), color::White);
    x += 8;

    if (width >= 66) {
        bool ready = p.dashCharge() >= 0.999f;
        canvas.text(x, y, "DASH", color::DkGrey);
        x += 5;
        canvas.text(x, y, bar(p.dashCharge(), 6), ready ? color::Cyan : color::DkCyan);
        x += 7;
    }

    if (width >= 74) {
        canvas.text(x, y, "LVL", color::DkGrey);
        x += 4;
        std::swprintf(buf, 64, L"%-2d", p.level);
        canvas.text(x, y, std::wstring(buf), color::Cyan);
        x += 3;
    }

    if (width >= 83) {
        std::swprintf(buf, 64, L"XP %d/%d", p.xp, p.xpToNext);
        canvas.text(x, y, std::wstring(buf), color::DkCyan);
    }

    if (showFps && width >= 74) {
        std::swprintf(buf, 64, L"%3.0f fps", fps);
        canvas.text(width - 8, y, std::wstring(buf), color::DkGrey);
    }
}

void Hud::drawFooter(Canvas& canvas, const World& world, int totalRows) const {
    const int width = canvas.width();
    std::wstring line;
    if (width >= 90) {
        line = L" WASD/ARROWS move   SPACE dash   P pause   Q quit    |    enemies ";
    } else {
        line = L" WASD move  SPACE dash  P pause  Q quit | foes ";
    }
    line += std::to_wstring(world.enemiesAlive());

    if (world.waveResting()) {
        wchar_t rest[48];
        std::swprintf(rest, 48, L"   next wave %.1fs", world.restRemaining());
        if (line.size() + std::wcslen(rest) < static_cast<std::size_t>(width)) line += rest;
    }

    canvas.text(0, totalRows - 1, pad(line, static_cast<std::size_t>(width)), color::DkGrey);
}

void Hud::drawBanner(Canvas& canvas, const World& world, int arenaWidth) const {
    if (world.bannerTimer() <= 0.0f) return;
    std::wstring text(world.bannerText().begin(), world.bannerText().end());

    int x = (arenaWidth - static_cast<int>(text.size())) / 2 + 1;
    if (x < 1) x = 1;
    int y = 4;
    bool flash = std::fmod(world.bannerTimer(), 0.4f) < 0.28f;
    canvas.text(x, y, text, flash ? color::White : color::Grey);
}

void Hud::drawBossBar(Canvas& canvas, const World& world, int arenaWidth) const {
    if (arenaWidth < 30) return;
    if (!world.bossAlive()) return;

    const Enemy* boss = nullptr;
    for (const auto& e : world.enemies()) {
        if (e && e->alive && e->isBoss()) {
            boss = e.get();
            break;
        }
    }
    if (!boss) return;

    float ratio = boss->maxHp > 0.0f ? boss->hp / boss->maxHp : 0.0f;
    int barWidth = arenaWidth - 24;
    if (barWidth < 8) barWidth = 8;
    if (barWidth > 80) barWidth = 80;
    int x = (arenaWidth - (barWidth + 8)) / 2 + 1;
    canvas.text(x, 2, L" WARDEN ", color::Red);
    canvas.text(x + 8, 2, bar(ratio, barWidth), color::Red);
}

}  // namespace aa
