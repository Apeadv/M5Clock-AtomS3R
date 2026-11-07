# Third-party notices

This project uses the following open-source libraries. They are not vendored in
this repository; install them separately when building from source.

| Library | Version | License / link |
|---|---|---|
| M5GFX | 0.3.0 | MIT — https://github.com/m5stack/M5GFX |
| M5Chain | 1.0.11 | MIT — https://github.com/m5stack/M5Chain |
| Arduino ESP32 core | 3.1.3 | LGPL-2.1 / Apache-2.0 — https://github.com/espressif/arduino-esp32 |

The prebuilt firmware also contains code from the Arduino ESP32 core and the
libraries above. Their respective licenses continue to apply.

The 16×16 Chinese bitmap font in `firmware/src/cn_font.h` was generated from
**Noto Sans SC Bold**, which is licensed under the SIL Open Font License 1.1.
The generated bitmap subset is distributed under the same font license. The
full license text is in [`LICENSES/OFL-1.1.txt`](LICENSES/OFL-1.1.txt).
