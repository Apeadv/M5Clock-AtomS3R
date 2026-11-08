#pragma once

#include <Arduino.h>
#include <time.h>

#include "M5Chain.h"

class MonoDisplay {
public:
    bool begin(uint8_t brightness);
    bool reconnect(uint8_t brightness);
    void setOrder(uint8_t order) { order_ = order ? 1 : 0; }
    void setBrightness(uint8_t brightness);

    int count() const { return count_; }
    bool ready() const { return count_ > 0; }

    void showTime(const struct tm &region, bool colonOn, uint8_t yOffset);
    void showText(const char *text, uint32_t nowMs, uint16_t msPerPixel, uint8_t yOffset);
    void showSpectrum(const uint8_t levels[12]);
    void clear();

private:
    bool discover(uint8_t brightness);
    bool configure(uint8_t id, uint8_t brightness);
    void buildFrame(const struct tm &t, bool colonOn, uint8_t yOffset, uint8_t frame[8][3],
                    int modules) const;
    void buildTextFrame(const char *text, uint32_t nowMs, uint16_t msPerPixel,
                        uint8_t yOffset, uint8_t frame[8][3], int modules) const;
    void pushFrame(uint8_t frame[8][3], int modules);

    Chain chain_;
    uint8_t ids_[3] = {0, 0, 0};
    uint8_t count_ = 0;
    uint8_t order_ = 0;
    uint8_t brightness_ = 6;
    uint32_t lastProbeMs_ = 0;
};
