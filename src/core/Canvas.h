// Canvas.h - ASCII draw surface. All game drawing goes through this class, so
// rendering logic never touches the platform layer directly.
#pragma once

#include <string>
#include <vector>

#include "core/Platform.h"

namespace aa {

namespace color {
enum : uint8_t {
    Black = 0, DkRed = 1, DkGreen = 2, DkYellow = 3, DkBlue = 4,
    DkMagenta = 5, DkCyan = 6, DkGrey = 7, Grey = 8,
    Red = 9, Green = 10, Yellow = 11, Blue = 12,
    Magenta = 13, Cyan = 14, White = 15
};
}

class Canvas {
public:
    Canvas(int width, int height);

    void resize(int width, int height);
    void clear(wchar_t ch = L' ', uint8_t col = color::DkGrey);

    void put(int x, int y, wchar_t ch, uint8_t col);
    void text(int x, int y, const std::wstring& s, uint8_t col);
    void text(int x, int y, const std::string& s, uint8_t col);
    void hline(int x, int y, int len, wchar_t ch, uint8_t col);
    void vline(int x, int y, int len, wchar_t ch, uint8_t col);
    void box(int x, int y, int w, int h, uint8_t col);
    void fill(int x, int y, int w, int h, wchar_t ch, uint8_t col);

    int width() const { return w_; }
    int height() const { return h_; }
    const std::vector<Cell>& cells() const { return cells_; }

    // Restricts all writes to a rectangle (used so effects cannot punch holes in
    // the arena wall).
    void setClip(int x, int y, int w, int h);
    void clearClip() { clipX_ = 0; clipY_ = 0; clipW_ = w_; clipH_ = h_; }

    void present(Terminal& term) const;
    // Colour-free text dump, used by the headless test harness.
    std::string toPlainText() const;
    // Same frame with ANSI colour escapes (used to render documentation shots).
    std::string toAnsiString() const;

private:
    bool inBounds(int x, int y) const {
        return x >= 0 && y >= 0 && x < w_ && y < h_ && x >= clipX_ && y >= clipY_ &&
               x < clipX_ + clipW_ && y < clipY_ + clipH_;
    }

    int w_ = 0;
    int h_ = 0;
    int clipX_ = 0;
    int clipY_ = 0;
    int clipW_ = 0;
    int clipH_ = 0;
    std::vector<Cell> cells_;
};

}  // namespace aa
