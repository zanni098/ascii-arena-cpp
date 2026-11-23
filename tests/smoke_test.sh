#!/usr/bin/env bash
# Automated smoke test: builds the game, runs two deterministic headless
# simulations and checks that gameplay actually happened.
set -euo pipefail
cd "$(dirname "$0")/.."

mkdir -p build
echo "== build =="
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Isrc \
    src/main.cpp src/core/*.cpp src/game/*.cpp -o build/ascii_arena_test

echo "== run 1 (seed 12345, 60s) =="
OUT1=$(./build/ascii_arena_test --demo 60 --seed 12345)
echo "$OUT1"

echo "== run 2 (same seed, must match) =="
OUT2=$(./build/ascii_arena_test --demo 60 --seed 12345)
if [ "$OUT1" != "$OUT2" ]; then
    echo "FAIL: same seed produced different results"
    exit 1
fi

SCORE=$(echo "$OUT1" | sed -n 's/.*score=\([0-9]*\).*/\1/p')
KILLS=$(echo "$OUT1" | sed -n 's/.*kills=\([0-9]*\).*/\1/p')
WAVE=$(echo "$OUT1" | sed -n 's/.*wave=\([0-9]*\).*/\1/p')

[ -n "$SCORE" ] || { echo "FAIL: no score in output"; exit 1; }
[ "$SCORE" -gt 0 ] || { echo "FAIL: nothing was killed"; exit 1; }
[ "$KILLS" -gt 5 ] || { echo "FAIL: too few kills ($KILLS)"; exit 1; }
[ "$WAVE" -ge 2 ] || { echo "FAIL: waves did not advance (wave $WAVE)"; exit 1; }

echo "PASS: score=$SCORE kills=$KILLS wave=$WAVE"
