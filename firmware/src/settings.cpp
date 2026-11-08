#include "settings.h"

#include <Preferences.h>

#include "clock_data.h"
#include "config.h"

namespace {
Preferences prefs;
constexpr const char *NS = "m5clock";

uint8_t clampU8(uint8_t value, uint8_t lo, uint8_t hi) {
    return value < lo ? lo : (value > hi ? hi : value);
}
}  // namespace

void Settings::load() {
    prefs.begin(NS, true);
    ssid = prefs.getString("ssid", "");
    pass = prefs.getString("pass", "");
    monoOrder = prefs.getUChar("order", 0) ? 1 : 0;
    lcdBrightness = clampU8(prefs.getUChar("lcd", cfg::DEFAULT_LCD_BRIGHTNESS), 20, 255);
    chainBrightness = clampU8(prefs.getUChar("chain", cfg::DEFAULT_CHAIN_BRIGHTNESS), 0, 7);
    lang = prefs.getUChar("lang", 0) ? 1 : 0;
    monoMode = clampU8(prefs.getUChar("mode", 0), 0, 2);
    region = clampU8(prefs.getUChar("region", 0), 0, REGION_MAX);
    customUtcOffset = prefs.getChar("utc", 8);
    if (customUtcOffset < -12) customUtcOffset = -12;
    if (customUtcOffset > 14) customUtcOffset = 14;
    audioSource = clampU8(prefs.getUChar("audio", 0), 0, 1);
    screenMode = clampU8(prefs.getUChar("screen", 0), 0, 1);
    syncScreen = clampU8(prefs.getUChar("sync", 1), 0, 1);
    scrollSpeed = clampU8(prefs.getUChar("speed", 1), 0, 2);
    textPosition = clampU8(prefs.getUChar("pos", 0), 0, 1);
    monoText = prefs.getString("text", "M5STACK");
    if (monoText.length() == 0) monoText = "M5STACK";
    prefs.end();
}

void Settings::save() const {
    prefs.begin(NS, false);
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    prefs.putUChar("order", monoOrder);
    prefs.putUChar("lcd", lcdBrightness);
    prefs.putUChar("chain", chainBrightness);
    prefs.putUChar("lang", lang);
    prefs.putUChar("mode", monoMode);
    prefs.putUChar("region", region);
    prefs.putChar("utc", customUtcOffset);
    prefs.putUChar("speed", scrollSpeed);
    prefs.putUChar("audio", audioSource);
    prefs.putUChar("screen", screenMode);
    prefs.putUChar("sync", syncScreen);
    prefs.putUChar("pos", textPosition);
    prefs.putString("text", monoText);
    prefs.end();
}

void Settings::clearWifi() {
    ssid = "";
    pass = "";
    prefs.begin(NS, false);
    prefs.remove("ssid");
    prefs.remove("pass");
    prefs.end();
}
