#!/usr/bin/env python3
"""End-to-end test of the real game binary through a pseudo terminal.

Unlike `--demo` (which bypasses the terminal entirely), this drives the actual
menu, run and pause paths with real key presses and checks what the game draws.
Linux/macOS only; the Windows console path needs a real console.

Usage:
    python3 tests/interactive_test.py [path/to/binary]
"""
import os
import pty
import re
import select
import signal
import subprocess
import sys
import time

ANSI = re.compile(r"\x1b\[[0-9;?]*[A-Za-z]")
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BIN = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "build", "ascii_arena_test")

if not os.path.exists(BIN):
    sys.exit(f"binary not found: {BIN}\nbuild it first, e.g. bash tests/smoke_test.sh")

master, slave = pty.openpty()
os.set_blocking(master, False)
proc = subprocess.Popen([BIN], stdin=slave, stdout=slave, stderr=slave,
                        env=dict(os.environ, TERM="xterm"), close_fds=True,
                        preexec_fn=os.setsid)
os.close(slave)

buffer = ""


def pump(seconds):
    global buffer
    end = time.time() + seconds
    while time.time() < end:
        ready, _, _ = select.select([master], [], [], 0.05)
        if master in ready:
            try:
                buffer += os.read(master, 1 << 16).decode("utf-8", "replace")
            except OSError:
                return


def screen_text():
    return ANSI.sub("", buffer)


def send(keys, settle=0.45):
    os.write(master, keys.encode())
    pump(settle)


def check(label, needle):
    ok = needle in screen_text()
    print(f"{'PASS' if ok else 'FAIL'}  {label}: {needle!r}")
    return ok


results = []
try:
    pump(1.2)
    results.append(check("menu renders", "a survival arena drawn in pure ascii"))
    results.append(check("menu lists start option", "START RUN"))

    send("\r")
    pump(1.0)
    results.append(check("run starts, HUD visible", "WAVE"))
    results.append(check("player drawn", "@"))

    send("d", 0.2)
    send("s", 0.2)
    send("d", 0.5)
    results.append(check("still running after movement", "WAVE"))

    send("p", 0.6)
    results.append(check("pause overlay", "PAUSED"))

    send("p", 0.6)
    tail = screen_text().rsplit("PAUSED", 1)[-1]
    resumed = "PAUSED" not in tail
    print(f"{'PASS' if resumed else 'FAIL'}  resume clears the pause overlay")
    results.append(resumed)

    send("q", 0.8)
    results.append(check("q abandons run back to menu", "START RUN"))

    send("q", 1.0)
    exited = proc.poll() is not None
    print(f"{'PASS' if exited else 'FAIL'}  process exits on quit from menu")
    results.append(exited)
finally:
    if proc.poll() is None:
        os.killpg(os.getpgid(proc.pid), signal.SIGKILL)

ok = all(results)
print("\nRESULT:", "ALL PASS" if ok else "SOME CHECKS FAILED")
sys.exit(0 if ok else 1)
