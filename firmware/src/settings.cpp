#include "settings.h"

#include <Preferences.h>

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
