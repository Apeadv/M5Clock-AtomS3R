import re
from pathlib import Path

src = Path("tools/glcdfont.c").read_text(encoding="utf-8")
body = src[src.index("font[]") :]
vals = [int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]{2})", body)]
assert len(vals) >= 95 * 5, len(vals)
chars = [vals[32 * 5 + i * 5 : 32 * 5 + (i + 1) * 5] for i in range(95)]

lines = [
    "// 5x7 ASCII font, generated from Adafruit GFX glcdfont.c (BSD-3-Clause).",
    "// Index = ASCII 32..126. Each glyph is 5 column bytes, bit 0 = top row.",
    "#pragma once",
    "#include <stdint.h>",
    "",
    "static const uint8_t FONT5X7[95][5] = {",
]
for i, glyph in enumerate(chars):
    code = i + 32
    comment = chr(code) if 32 <= code <= 126 else "?"
    if comment == "\\":
        comment = "backslash"
    lines.append(f"    {{{', '.join(f'0x{b:02X}' for b in glyph)}}},  // {code} '{comment}'")
lines += [
    "};",
    "",
    "inline const uint8_t *glyph5x7(char c) {",
    "    if (c < 32 || c > 126) return FONT5X7[0];",
    "    return FONT5X7[c - 32];",
    "}",
    "",
]
Path("firmware/src/text_font.h").write_text("\n".join(lines), encoding="utf-8")
print("values", len(vals), "wrote", Path("firmware/src/text_font.h").stat().st_size)
