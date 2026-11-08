#pragma once

#include <Arduino.h>

struct Settings {
    String ssid;
    String pass;
    uint8_t monoOrder = 0;  // 0 = chain id 1..3 left-to-right, 1 = reversed
    uint8_t lcdBrightness = 180;
    uint8_t chainBrightness = 6;
    uint8_t lang = 0;         // 0 = 中文, 1 = English
    uint8_t monoMode = 0;     // 0 = 时钟, 1 = 自定义字符
    uint8_t region = 0;          // 0..6 fixed regions, 7 = custom UTC offset
    int8_t customUtcOffset = 8;  // used when region == 7, -12..+14 hours
    uint8_t audioSource = 0;     // 0 = USB PC, 1 = PDM microphone
    uint8_t screenMode = 0;      // 0 = world clock, 1 = music spectrum
    uint8_t syncScreen = 1;      // follow Chain spectrum with the S3R screen
    uint8_t scrollSpeed = 1;     // 0 = slow, 1 = normal, 2 = fast
    uint8_t textPosition = 0;    // 0 = top, 1 = bottom
    String monoText = "M5STACK";

    void load();
    void save() const;
    void clearWifi();
    bool hasWifi() const { return ssid.length() > 0; }
};
