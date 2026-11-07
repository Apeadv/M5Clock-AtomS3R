#pragma once

#include <Arduino.h>

struct Settings {
    String ssid;
    String pass;
    uint8_t monoOrder = 0;  // 0 = chain id 1..3 left-to-right, 1 = reversed
    uint8_t lcdBrightness = 180;
    uint8_t chainBrightness = 6;
    uint8_t lang = 0;  // 0 = 中文, 1 = English

    void load();
    void save() const;
    void clearWifi();
    bool hasWifi() const { return ssid.length() > 0; }
};
