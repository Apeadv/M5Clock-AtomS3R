import re
from pathlib import Path
from PIL import Image, ImageDraw

h = Path("firmware/src/text_font.h").read_text(encoding="utf-8")
rows = re.findall(r"\{((?:0x[0-9A-F]{2}, ){4}0x[0-9A-F]{2})\}", h)
glyphs = [bytes(int(x, 16) for x in r.split(", ")) for r in rows]

text = "LOVE"
pitch = 6
width = len(text) * pitch - 1
start = (24 - width) // 2
canvas = [[0] * 24 for _ in range(8)]
x = start
for ch in text:
    g = glyphs[ord(ch) - 32]
    for col in range(5):
        bits = g[col]
        for row in range(7):
            if bits & (1 << row) and 0 <= x + col < 24:
                canvas[row][x + col] = 1
    x += pitch

cell = 16
gap = 8
margin = 12
w = margin * 2 + 3 * 8 * cell + 2 * gap
hgt = margin * 2 + 8 * cell
im = Image.new("RGB", (w, hgt), (16, 16, 16))
d = ImageDraw.Draw(im)
for m in range(3):
    ox = margin + m * (8 * cell + gap)
    for y in range(8):
        for xx in range(8):
            cx = ox + xx * cell
            cy = margin + y * cell
            on = canvas[y][m * 8 + xx]
            d.ellipse([cx + 2, cy + 2, cx + cell - 2, cy + cell - 2],
                      fill=(255, 46, 38) if on else (38, 10, 10))
im.save("dist/preview-chain-text.png")
print("ok", width)
