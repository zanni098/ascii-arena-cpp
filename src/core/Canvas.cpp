#include "core/Canvas.h"

#include <algorithm>
#include <cstdio>

namespace aa {

Canvas::Canvas(int width, int height) { resize(width, height); }

void Canvas::resize(int width, int height) {
    w_ = std::max(1, width);
    h_ = std::max(1, height);
    cells_.assign(static_cast<size_t>(w_) * static_cast<size_t>(h_), Cell{});
    clearClip();
    clear();
}

void Canvas::setClip(int x, int y, int w, int h) {
    clipX_ = x < 0 ? 0 : x;
    clipY_ = y < 0 ? 0 : y;
    clipW_ = w < 0 ? 0 : w;
    clipH_ = h < 0 ? 0 : h;
    if (clipX_ > w_) clipX_ = w_;
    if (clipY_ > h_) clipY_ = h_;
    if (clipX_ + clipW_ > w_) clipW_ = w_ - clipX_;
    if (clipY_ + clipH_ > h_) clipH_ = h_ - clipY_;
}

void Canvas::clear(wchar_t ch, uint8_t col) {
    Cell c;
    c.ch = ch;
    c.color = col;
    std::fill(cells_.begin(), cells_.end(), c);
}

void Canvas::put(int x, int y, wchar_t ch, uint8_t col) {
    if (!inBounds(x, y)) return;
    Cell& c = cells_[static_cast<size_t>(y) * w_ + x];
    c.ch = ch;
    c.color = col;
}

void Canvas::text(int x, int y, const std::wstring& s, uint8_t col) {
    int cx = x;
    for (wchar_t ch : s) {
        if (ch == L'\n') {
            ++y;
            cx = x;
            continue;
        }
        if (ch != L'\r') put(cx, y, ch, col);
        ++cx;
    }
}

void Canvas::text(int x, int y, const std::string& s, uint8_t col) {
    std::wstring w(s.begin(), s.end());
    text(x, y, w, col);
}

void Canvas::hline(int x, int y, int len, wchar_t ch, uint8_t col) {
    for (int i = 0; i < len; ++i) put(x + i, y, ch, col);
}

void Canvas::vline(int x, int y, int len, wchar_t ch, uint8_t col) {
    for (int i = 0; i < len; ++i) put(x, y + i, ch, col);
}

void Canvas::box(int x, int y, int w, int h, uint8_t col) {
    if (w < 2 || h < 2) return;
    hline(x, y, w, L'-', col);
    hline(x, y + h - 1, w, L'-', col);
    vline(x, y, h, L'|', col);
    vline(x + w - 1, y, h, L'|', col);
    put(x, y, L'+', col);
    put(x + w - 1, y, L'+', col);
    put(x, y + h - 1, L'+', col);
    put(x + w - 1, y + h - 1, L'+', col);
}

void Canvas::fill(int x, int y, int w, int h, wchar_t ch, uint8_t col) {
    for (int yy = y; yy < y + h; ++yy)
        for (int xx = x; xx < x + w; ++xx) put(xx, yy, ch, col);
}

void Canvas::present(Terminal& term) const { term.present(cells_, w_, h_); }

std::string Canvas::toAnsiString() const {
    std::string out;
    out.reserve(static_cast<size_t>(w_) * (h_ + 1) * 3);
    for (int y = 0; y < h_; ++y) {
        int lastColor = -1;
        for (int x = 0; x < w_; ++x) {
            const Cell& c = cells_[static_cast<size_t>(y) * w_ + x];
            int col = c.color & 15;
            if (col != lastColor) {
                char esc[16];
                if (col < 8) std::snprintf(esc, sizeof(esc), "\x1b[%dm", 30 + col);
                else std::snprintf(esc, sizeof(esc), "\x1b[%dm", 90 + (col - 8));
                out += esc;
                lastColor = col;
            }
            out.push_back(c.ch < 128 ? static_cast<char>(c.ch) : '?');
        }
        out += "\x1b[0m";
        out.push_back('\n');
    }
    return out;
}

std::string Canvas::toPlainText() const {
    std::string out;
    out.reserve(static_cast<size_t>(w_) * (h_ + 1));
    for (int y = 0; y < h_; ++y) {
        for (int x = 0; x < w_; ++x) {
            wchar_t ch = cells_[static_cast<size_t>(y) * w_ + x].ch;
            out.push_back(ch < 128 ? static_cast<char>(ch) : '?');
        }
        out.push_back('\n');
    }
    return out;
}

}  // namespace aa
