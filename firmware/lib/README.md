# Dependencies

This firmware was built with:

- **M5GFX 0.3.0** — https://github.com/m5stack/M5GFX
- **M5Chain 1.0.11** — https://github.com/m5stack/M5Chain
- **Arduino ESP32 core 3.1.3**

For Arduino IDE, install both libraries with the Library Manager.

For Arduino CLI, place them here as `lib/M5GFX` and `lib/M5Chain`, then compile
with `--libraries firmware/lib`.

The 16×16 Chinese bitmap font is already generated in `src/cn_font.h`.
Pillow and the Noto Sans SC Bold font are only needed if you change the UI text
and re-run `tools/gen_font.py`.
