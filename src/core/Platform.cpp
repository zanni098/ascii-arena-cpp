// Platform.cpp - Windows Console API implementation with a POSIX fallback so the
// same game can be built and tested on Linux/macOS terminals.
#include "core/Platform.h"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>

#ifdef _WIN32
// windows.h pulled in by Platform.h
#else
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace aa {
namespace {

int clampInt(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

#ifdef _WIN32
int mapVirtualKey(WORD vk) {
    switch (vk) {
        case VK_UP: return K_UP;
        case VK_DOWN: return K_DOWN;
        case VK_LEFT: return K_LEFT;
        case VK_RIGHT: return K_RIGHT;
        case VK_SPACE: return K_SPACE;
        case VK_ESCAPE: return K_ESC;
        case VK_RETURN: return K_ENTER;
        case VK_BACK: return K_BACKSPACE;
        case 'W': return K_W;
        case 'A': return K_A;
        case 'S': return K_S;
        case 'D': return K_D;
        case 'P': return K_P;
        case 'Q': return K_Q;
        case 'R': return K_R;
        case '1': return K_1;
        case '2': return K_2;
        case '3': return K_3;
        default: return -1;
    }
}

// ANSI palette index -> Win32 character attribute.
WORD ansiToAttr(uint8_t c) {
    WORD a = 0;
    if (c & 1) a |= FOREGROUND_RED;
    if (c & 2) a |= FOREGROUND_GREEN;
    if (c & 4) a |= FOREGROUND_BLUE;
    if (c & 8) a |= FOREGROUND_INTENSITY;
    return a;
}
#endif

}  // namespace

Terminal::~Terminal() { shutdown(); }

bool Terminal::init(int* cols, int* rows) {
    if (initialized_) {
        if (cols) *cols = cols_;
        if (rows) *rows = rows_;
        return true;
    }

#ifdef _WIN32
    hOut_ = GetStdHandle(STD_OUTPUT_HANDLE);
    hIn_ = GetStdHandle(STD_INPUT_HANDLE);
    if (hOut_ == INVALID_HANDLE_VALUE || hIn_ == INVALID_HANDLE_VALUE) return false;

    CONSOLE_SCREEN_BUFFER_INFO csbi{};
    GetConsoleScreenBufferInfo(hOut_, &csbi);
    cols_ = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    rows_ = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    // Normalise to a sensible play area. Any failure here is harmless.
    int wantCols = clampInt(cols_ < 50 ? 100 : cols_, 60, 140);
    int wantRows = clampInt(rows_ < 22 ? 34 : rows_, 22, 50);
    SMALL_RECT win{0, 0, static_cast<SHORT>(wantCols - 1), static_cast<SHORT>(wantRows - 1)};
    COORD buf{static_cast<SHORT>(wantCols), static_cast<SHORT>(wantRows)};
    SetConsoleWindowInfo(hOut_, TRUE, &win);
    SetConsoleScreenBufferSize(hOut_, buf);
    SetConsoleWindowInfo(hOut_, TRUE, &win);
    GetConsoleScreenBufferInfo(hOut_, &csbi);
    cols_ = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    rows_ = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    GetConsoleMode(hOut_, &prevOutMode_);
    SetConsoleMode(hOut_, prevOutMode_);
    GetConsoleMode(hIn_, &prevInMode_);
    SetConsoleMode(hIn_, ENABLE_WINDOW_INPUT | ENABLE_EXTENDED_FLAGS);

    SetConsoleTitleW(L"ASCII ARENA");
    CONSOLE_CURSOR_INFO cci{1, FALSE};
    SetConsoleCursorInfo(hOut_, &cci);
    ciBuf_.assign(static_cast<size_t>(cols_) * static_cast<size_t>(rows_), CHAR_INFO{});
    initialized_ = true;
    if (cols) *cols = cols_;
    if (rows) *rows = rows_;
    return true;
#else
    cols_ = 100;
    rows_ = 34;
    if (isatty(STDOUT_FILENO)) {
        struct winsize ws {};
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
            cols_ = ws.ws_col;
            rows_ = ws.ws_row;
        }
    }
    cols_ = clampInt(cols_, 50, 200);
    rows_ = clampInt(rows_, 18, 60);

    if (isatty(STDIN_FILENO)) {
        termios* saved = new termios();
        if (tcgetattr(STDIN_FILENO, saved) == 0) {
            termios raw = *saved;
            raw.c_lflag &= ~(ICANON | ECHO);
            raw.c_cc[VMIN] = 0;
            raw.c_cc[VTIME] = 0;
            raw.c_iflag &= ~(IXON | ICRNL);
            tcsetattr(STDIN_FILENO, TCSANOW, &raw);
            savedTermios_ = saved;
            rawMode_ = true;
        } else {
            delete saved;
        }
    }
    std::fputs("\x1b[?25l\x1b[2J", stdout);
    std::fflush(stdout);
    initialized_ = true;
    if (cols) *cols = cols_;
    if (rows) *rows = rows_;
    return true;
#endif
}

void Terminal::shutdown() {
    if (!initialized_) return;
#ifdef _WIN32
    if (hOut_ != INVALID_HANDLE_VALUE && prevOutMode_ != 0) SetConsoleMode(hOut_, prevOutMode_);
    if (hIn_ != INVALID_HANDLE_VALUE && prevInMode_ != 0) SetConsoleMode(hIn_, prevInMode_);
#else
    if (rawMode_ && savedTermios_) {
        tcsetattr(STDIN_FILENO, TCSANOW, static_cast<termios*>(savedTermios_));
    }
    delete static_cast<termios*>(savedTermios_);
    savedTermios_ = nullptr;
    rawMode_ = false;
    std::fputs("\x1b[0m\x1b[?25h\n", stdout);
    std::fflush(stdout);
#endif
    initialized_ = false;
}

InputState Terminal::poll() {
    InputState in;
#ifdef _WIN32
    DWORD pending = 0;
    GetNumberOfConsoleInputEvents(hIn_, &pending);
    if (pending > 0) {
        std::vector<INPUT_RECORD> recs(pending);
        DWORD got = 0;
        if (!ReadConsoleInputW(hIn_, recs.data(), pending, &got)) return in;
        for (DWORD i = 0; i < got; ++i) {
            if (recs[i].EventType != KEY_EVENT) continue;
            const KEY_EVENT_RECORD& k = recs[i].Event.KeyEvent;
            int id = mapVirtualKey(k.wVirtualKeyCode);
            wchar_t ch = k.uChar.UnicodeChar;
            if (ch >= L'a' && ch <= L'z') ch = static_cast<wchar_t>(ch - 32);  // normalise
            if (id < 0) {
                switch (ch) {
                    case L'W': id = K_W; break;
                    case L'A': id = K_A; break;
                    case L'S': id = K_S; break;
                    case L'D': id = K_D; break;
                    case L'P': id = K_P; break;
                    case L'Q': id = K_Q; break;
                    case L'R': id = K_R; break;
                    case L'1': id = K_1; break;
                    case L'2': id = K_2; break;
                    case L'3': id = K_3; break;
                    case L' ': id = K_SPACE; break;
                    case L'\r': id = K_ENTER; break;
                    case 27: id = K_ESC; break;
                    default: break;
                }
            }
            if (id < 0) continue;
            if (k.bKeyDown) {
                if (!in.down[id]) in.pressed[id] = true;
                in.down[id] = true;
                if (ch >= 32 && ch != 127) in.typed.push_back(ch);
            } else {
                in.down[id] = false;
            }
        }
    }
#else
    unsigned char buf[256];
    ssize_t n = 0;
    if (rawMode_) {
        n = ::read(STDIN_FILENO, buf, sizeof(buf) - 1);
    }
    const double now = nowSeconds();
    for (ssize_t i = 0; i < n; ++i) {
        unsigned char c = buf[i];
        if (c == 27 && i + 2 < n && buf[i + 1] == '[') {
            unsigned char code = buf[i + 2];
            i += 2;
            int id = -1;
            if (code == 'A') id = K_UP;
            else if (code == 'B') id = K_DOWN;
            else if (code == 'C') id = K_RIGHT;
            else if (code == 'D') id = K_LEFT;
            if (id >= 0) {
                in.down[id] = true;
                in.pressed[id] = true;
                keyHold_[id] = now + 0.16;
            }
            continue;
        }
        wchar_t ch = static_cast<wchar_t>(c);
        int id = -1;
        switch (ch) {
            case 'w': case 'W': id = K_W; break;
            case 'a': case 'A': id = K_A; break;
            case 's': case 'S': id = K_S; break;
            case 'd': case 'D': id = K_D; break;
            case 'p': case 'P': id = K_P; break;
            case 'q': case 'Q': id = K_Q; break;
            case 'r': case 'R': id = K_R; break;
            case '1': id = K_1; break;
            case '2': id = K_2; break;
            case '3': id = K_3; break;
            case ' ': id = K_SPACE; break;
            case '\r': case '\n': id = K_ENTER; break;
            case 127: case 8: id = K_BACKSPACE; break;
            case 27: id = K_ESC; break;
            default: break;
        }
        if (ch > 32 && ch < 127) in.typed.push_back(ch);
        if (id >= 0) {
            in.down[id] = true;
            in.pressed[id] = true;
            keyHold_[id] = now + 0.16;
        }
    }
    // Terminal key repeat keeps a movement key "held" between reads.
    for (int k = 0; k < K_COUNT; ++k) {
        if (keyHold_[k] > now) in.down[k] = true;
    }
#endif
    return in;
}

void Terminal::present(const std::vector<Cell>& cells, int cols, int rows) {
    if (cols <= 0 || rows <= 0) return;
    cols_ = cols;
    rows_ = rows;

#ifdef _WIN32
    ciBuf_.resize(static_cast<size_t>(cols) * static_cast<size_t>(rows));
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            const Cell& c = cells[static_cast<size_t>(y) * cols + x];
            CHAR_INFO& ci = ciBuf_[static_cast<size_t>(y) * cols + x];
            ci.Char.UnicodeChar = c.ch;
            ci.Attributes = ansiToAttr(c.color);
        }
    }
    COORD size{static_cast<SHORT>(cols), static_cast<SHORT>(rows)};
    COORD origin{0, 0};
    SMALL_RECT region{0, 0, static_cast<SHORT>(cols - 1), static_cast<SHORT>(rows - 1)};
    WriteConsoleOutputW(hOut_, ciBuf_.data(), size, origin, &region);
#else
    std::string out;
    out.reserve(static_cast<size_t>(cols) * rows * 2 + 1024);
    out += "\x1b[H";
    int lastColor = -1;
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            const Cell& c = cells[static_cast<size_t>(y) * cols + x];
            int col = c.color & 15;
            if (col != lastColor) {
                char esc[16];
                if (col < 8) std::snprintf(esc, sizeof(esc), "\x1b[%dm", 30 + col);
                else std::snprintf(esc, sizeof(esc), "\x1b[%dm", 90 + (col - 8));
                out += esc;
                lastColor = col;
            }
            wchar_t wc = c.ch;
            if (wc < 128) out.push_back(static_cast<char>(wc));
            else out.push_back('?');  // ASCII-safe: the game only draws ASCII glyphs
        }
        if (y + 1 < rows) out += "\n";
    }
    out += "\x1b[0m";
    std::fwrite(out.data(), 1, out.size(), stdout);
    std::fflush(stdout);
#endif
}

double Terminal::nowSeconds() const {
    using clock = std::chrono::steady_clock;
    static const clock::time_point start = clock::now();
    return std::chrono::duration<double>(clock::now() - start).count();
}

void Terminal::sleepFor(double seconds) {
    if (seconds <= 0.0) return;
    std::this_thread::sleep_for(std::chrono::duration<double>(seconds));
}

}  // namespace aa
