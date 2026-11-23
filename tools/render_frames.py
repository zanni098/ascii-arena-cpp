#!/usr/bin/env python3
"""Render ASCII ARENA .ans frame dumps to PNG (used for the documentation shots).

Usage:
    python3 tools/render_frames.py <input.ans> <output.png>

The .ans files are written by `ascii_arena --screens <dir>` and contain plain
ANSI SGR colour codes, so this script only needs to understand 30..37 / 90..97.
"""
import re
import sys

from PIL import Image, ImageDraw, ImageFont

PALETTE = {
    0: "#17171c",
    1: "#d2565f",
    2: "#8cc265",
    3: "#d1b45a",
    4: "#4f9fd6",
    5: "#c162de",
    6: "#3fa8b8",
    7: "#6f6f7a",
    8: "#8b8b96",
    9: "#ff6b6b",
    10: "#a5e075",
    11: "#ffd479",
    12: "#6ab7ff",
    13: "#e18cff",
    14: "#5fd7e0",
    15: "#f4f4f6",
}
BG = "#0b0b0f"
FONT_PATH = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf"

SGR = re.compile(r"\x1b\[(\d+)m")


def parse_line(line):
    """Split one ANSI line into (char, colour_index) pairs."""
    cells = []
    colour = 15
    pos = 0
    for match in SGR.finditer(line):
        chunk = line[pos:match.start()]
        for ch in chunk:
            cells.append((ch, colour))
        code = int(match.group(1))
        if code == 0:
            colour = 15
        elif 30 <= code <= 37:
            colour = code - 30
        elif 90 <= code <= 97:
            colour = code - 90 + 8
        pos = match.end()
    for ch in line[pos:]:
        cells.append((ch, colour))
    return cells


def render(src, dst, font_size=16):
    rows = [parse_line(line.rstrip("\n")) for line in open(src, encoding="utf-8")]
    cols = max((len(r) for r in rows), default=1)

    font = ImageFont.truetype(FONT_PATH, font_size)
    advance = font.getlength("M")
    cell_w = int(round(advance))
    ascent, descent = font.getmetrics()
    cell_h = ascent + descent

    pad = 16
    img = Image.new("RGB", (cols * cell_w + pad * 2, len(rows) * cell_h + pad * 2), BG)
    draw = ImageDraw.Draw(img)

    for y, row in enumerate(rows):
        for x, (ch, colour) in enumerate(row):
            if ch == " ":
                continue
            draw.text((pad + x * cell_w, pad + y * cell_h), ch,
                      font=font, fill=PALETTE.get(colour, PALETTE[15]))

    img.save(dst)
    print(f"{dst}: {img.width}x{img.height}, {len(rows)} rows x {cols} cols")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(__doc__)
        raise SystemExit(2)
    render(sys.argv[1], sys.argv[2])
