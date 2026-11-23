// Platform.h - terminal setup, non-blocking input and frame presentation.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace aa {

struct Cell {
    wchar_t ch = L' ';
    uint8_t color = 7;  // ANSI 0..15 palette index
};

enum KeyId : int {
    K_UP, K_DOWN, K_LEFT, K_RIGHT,
    K_W, K_A, K_S, K_D,
    K_SPACE, K_ESC, K_ENTER, K_P, K_Q, K_R,
    K_1, K_2, K_3, K_BACKSPACE,
    K_COUNT
};

struct InputState {
    bool down[K_COUNT] = {};
    bool pressed[K_COUNT] = {};
    std::wstring typed;  // printable characters typed during this poll
};

// Double buffered terminal. Everything is written in one pass so the screen
// never tears; the cursor is parked at 0,0 and the buffer is drawn on top.
class Terminal {
public:
    Terminal() = default;
    ~Terminal();
    Terminal(const Terminal&) = delete;
    Terminal& operator=(const Terminal&) = delete;

    // Enables raw/non-blocking input and reports the usable character grid.
    bool init(int* cols, int* rows);

    // Restores the console to its original state.
    void shutdown();

    InputState poll();

    void present(const std::vector<Cell>& cells, int cols, int rows);

    double nowSeconds() const;
    void sleepFor(double seconds);

private:
    int cols_ = 80;
    int rows_ = 25;
    bool initialized_ = false;

#ifdef _WIN32
    HANDLE hOut_ = INVALID_HANDLE_VALUE;
    HANDLE hIn_ = INVALID_HANDLE_VALUE;
    DWORD prevOutMode_ = 0;
    DWORD prevInMode_ = 0;
    std::vector<CHAR_INFO> ciBuf_;
#else
    bool rawMode_ = false;
    void* savedTermios_ = nullptr;  // termios*, opaque in the header
    std::vector<Cell> prev_;
    bool hasPrev_ = false;
    double keyHold_[K_COUNT] = {};
#endif
};

}  // namespace aa
