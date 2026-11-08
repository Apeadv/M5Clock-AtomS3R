# Changelog

## [1.4.0] — 2026-11-08

### Added

- Music spectrum for the three Chain Mono modules and the AtomS3R LCD.
- The AtomS3R screen switches between world clock and music spectrum; the three Chain Mono modules always show the clock.
- Animated S3R spectrum: gradient bars, falling peaks, reflection, bass pulse, and LIVE indicator.
- USB PC spectrum input via `pc/m5clock_spectrum.py` and `pc/M5Clock-Spectrum.bat` (default).
- Windows autostart installer and automatic reconnect/wait for the AtomS3R.
- Optional PDM microphone input (M5Stack Unit Mini PDM) on Grove Port A.
- 256-point FFT with 12 logarithmic bands (60 Hz–7 kHz).
- 12-band color spectrum on the S3R, with the custom subtitle integrated at the bottom.
- `NO PC` / `NO MIC` status when no data is available.

### Changed

- Short press now cycles clock / custom text / music spectrum.

## [1.3.2] — 2026-11-07

### Added

- “Dot-matrix time zone” selector in the setup page.
- Beijing, London, New York, Los Angeles, Tokyo, Sydney, UTC, and custom UTC+X for the Mono clock.
- Custom UTC offset input from -12 to +14 integer hours.
- Slow / normal / fast scroll-speed setting.
- Double-click on the AtomS3R button cycles the scroll speed.
- Dot-matrix vertical position: top or bottom (shifted down one row), for both clock and custom text.
- Fixed the setup page Chinese / English language switch.
- S3R city list now puts the selected time zone first with a yellow bar; other cities use blue.
- UTC or custom UTC+X keeps Beijing first with a blue bar.
- Time zone, scroll speed, and vertical position are stored in NVS.

## [1.2.0] — 2026-11-07

### Added

- Custom dot-matrix text mode for the three Chain Mono modules.
- “Dot-matrix mode” branch in the setup page: Beijing clock or custom ASCII text.
- 5×7 ASCII text renderer with centered short text and horizontal scrolling for longer text.
- Short press on the AtomS3R button switches between clock and custom text mode.
- Custom text is stored in NVS and restored after reboot.

### Changed

- Short press no longer advances the world-clock page; the page continues to rotate automatically every 7 seconds.

## [1.1.0] — 2026-11-07

### Added

- Chinese / English language switch on the setup page.
- English rendering on the AtomS3R screen (`WORLD TIME`, English city names, English status messages).
- English portal / offline instructions.
- Language preference stored in NVS and applied without reflashing.

### Changed

- Added one blank column between the two digits inside each Chain Mono module.
- Moved each colon to the right edge of its module (`HH:`, `MM:`, `SS`).
- Updated the M5Burner image to a complete 8MB factory image.

### Removed

- The moving seconds dot on the bottom row of the Chain Mono display.

## [1.0.0] — 2026-11-07

- Initial release.
- Three Chain Mono modules show Beijing time `HH:MM:SS`.
- AtomS3R shows UTC and six world cities with DST support.
- AP setup portal, NTP, adjustable brightness and Mono order.

