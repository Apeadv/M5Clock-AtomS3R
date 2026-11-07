import os
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

CHARS = "世界时间北京伦敦纽约洛杉矶东京悉尼配网连接中未已同步周六日一二三四五模式正在请打开浏览器后手机成功失败重试检查网络长按按钮"
chars = list(dict.fromkeys(CHARS))

FONT_CANDIDATES = [
    os.environ.get("M5CLOCK_FONT"),
    r"C:\Windows\Fonts\Noto Sans SC Bold (TrueType).otf",
    "/usr/share/fonts/opentype/noto/NotoSansCJK-Bold.ttc",
    "/usr/share/fonts/truetype/noto/NotoSansCJK-Bold.ttc",
    "/System/Library/Fonts/PingFang.ttc",
]
FONT_PATH = next((p for p in FONT_CANDIDATES if p and Path(p).exists()), None)
if not FONT_PATH:
    raise SystemExit("No CJK font found. Set M5CLOCK_FONT to a bold CJK font path.")
SIZE = 16


def render(ch):
    font = ImageFont.truetype(FONT_PATH, SIZE, index=0)
    img = Image.new("L", (SIZE, SIZE), 0)
    dr = ImageDraw.Draw(img)
    bbox = dr.textbbox((0, 0), ch, font=font)
    w = bbox[2] - bbox[0]
    h = bbox[3] - bbox[1]
    x = (SIZE - w) // 2 - bbox[0]
    y = (SIZE - h) // 2 - bbox[1]
    dr.text((x, y), ch, font=font, fill=255)
    return img


def to_rows(img):
    rows = []
    for y in range(SIZE):
        b = 0
        for x in range(SIZE):
            if img.getpixel((x, y)) >= 100:
                b |= (1 << x)
        rows.append(b)
    return rows


def to_xbm_bytes(rows):
    """16x16 XBM = 2 bytes per row, least-significant byte first."""
    out = []
    for row in rows:
        out.append(row & 0xFF)
        out.append((row >> 8) & 0xFF)
    return out


glyphs = {}
preview = Image.new("L", (len(chars) * 18, 18), 0)
for i, ch in enumerate(chars):
    img = render(ch)
    rows = to_rows(img)
    glyphs[ch] = rows
    preview.paste(img, (i * 18 + 1, 1))

preview.resize((preview.width * 6, preview.height * 6), Image.Resampling.NEAREST).save("font_preview.png")

lines = []
lines.append("// Auto-generated 16x16 Chinese bitmap font (Noto Sans SC Bold, SIL OFL).")
lines.append("// Bit order: LSB = leftmost pixel, two bytes per row (XBM 16px). Suitable for drawXBitmap().")
lines.append("#pragma once")
lines.append("#include <stdint.h>")
lines.append("")
lines.append("struct CnGlyph { uint16_t cp; const uint8_t *bmp; };")
lines.append("")
names = []
for ch, rows in glyphs.items():
    cp = ord(ch)
    name = f"cn_{cp:04x}"
    names.append(name)
    data = to_xbm_bytes(rows)
    lines.append(f"static const uint8_t {name}[32] = {{")
    for r in range(0, 32, 8):
        lines.append("    " + ", ".join(f"0x{v:02X}" for v in data[r:r + 8]) + ",")
    lines.append("};")
    lines.append("")
lines.append("static const CnGlyph CN_GLYPHS[] = {")
for ch, name in zip(glyphs.keys(), names):
    lines.append(f"    {{0x{ord(ch):04X}, {name}}},")
lines.append("};")
lines.append("")
lines.append("static constexpr int CN_GLYPH_COUNT = sizeof(CN_GLYPHS) / sizeof(CN_GLYPHS[0]);")
lines.append("")
lines.append("// Returns nullptr when the code point is not in this compact subset.")
lines.append("inline const uint8_t *cnGlyph(uint16_t cp) {")
lines.append("    for (int i = 0; i < CN_GLYPH_COUNT; ++i) {")
lines.append("        if (CN_GLYPHS[i].cp == cp) return CN_GLYPHS[i].bmp;")
lines.append("    }")
lines.append("    return nullptr;")
lines.append("}")
lines.append("")

Path("firmware/src/cn_font.h").write_text("\n".join(lines), encoding="utf-8")
print("chars:", len(chars))
print("wrote", Path("firmware/src/cn_font.h").stat().st_size, "bytes")

