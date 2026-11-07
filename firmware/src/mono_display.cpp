#include "mono_display.h"

#include <cstring>

#include "config.h"

namespace {

// 3x7 digits. Each row uses bit2 = left column, bit0 = right column.
const uint8_t DIGITS[10][7] = {
    {0b111, 0b101, 0b101, 0b101, 0b101, 0b101, 0b111},  // 0
    {0b010, 0b110, 0b010, 0b010, 0b010, 0b010, 0b111},  // 1
    {0b111, 0b001, 0b001, 0b111, 0b100, 0b100, 0b111},  // 2
    {0b111, 0b001, 0b001, 0b111, 0b001, 0b001, 0b111},  // 3
    {0b101, 0b101, 0b101, 0b111, 0b001, 0b001, 0b001},  // 4
    {0b111, 0b100, 0b100, 0b111, 0b001, 0b001, 0b111},  // 5
    {0b111, 0b100, 0b100, 0b111, 0b101, 0b101, 0b111},  // 6
    {0b111, 0b001, 0b001, 0b010, 0b010, 0b010, 0b010},  // 7
    {0b111, 0b101, 0b101, 0b111, 0b101, 0b101, 0b111},  // 8
    {0b111, 0b101, 0b101, 0b111, 0b001, 0b001, 0b111},  // 9
};

inline void putPixel(uint8_t frame[8][3], int x, int y) {
    if (x < 0 || x >= 24 || y < 0 || y > 7) return;
    frame[y][x >> 3] |= static_cast<uint8_t>(0x80u >> (x & 7));
}

void drawDigit(uint8_t frame[8][3], int x, int digit, int y = 0) {
    if (digit < 0 || digit > 9) return;
    for (int row = 0; row < 7; ++row) {
        uint8_t bits = DIGITS[digit][row];
        for (int col = 0; col < 3; ++col) {
            if (bits & (0b100 >> col)) putPixel(frame, x + col, y + row);
        }
    }
}

void drawColon(uint8_t frame[8][3], int x) {
    putPixel(frame, x, 2);
    putPixel(frame, x, 5);
}

}  // namespace

bool MonoDisplay::begin(uint8_t brightness) {
    brightness_ = brightness;
    chain_.begin(&Serial2, 115200, cfg::PIN_CHAIN_RX, cfg::PIN_CHAIN_TX);
    delay(50);

    for (int attempt = 0; attempt < 3; ++attempt) {
        if (discover(brightness)) return true;
        delay(250);
    }
    return false;
}

bool MonoDisplay::reconnect(uint8_t brightness) {
    if (millis() - lastProbeMs_ < 3000) return count_ > 0;
    lastProbeMs_ = millis();
    brightness_ = brightness;
    return discover(brightness) || count_ > 0;
}

bool MonoDisplay::configure(uint8_t id, uint8_t brightness) {
    uint8_t op = 0;
    bool ok = true;

    if (chain_.setMonoMode(id, MONO_PIXEL_MODE, &op) != CHAIN_OK || op != 1) {
        Serial.printf("[MONO %u] set pixel mode failed (op=%u)\n", id, op);
        ok = false;
    }
    op = 0;
    if (chain_.setMonoRotation(id, MONO_ROTATION_0, &op) != CHAIN_OK || op != 1) {
        Serial.printf("[MONO %u] set rotation failed (op=%u)\n", id, op);
        ok = false;
    }
    op = 0;
    if (chain_.setMonoBrightness(id, static_cast<mono_brightness_level_t>(brightness), &op) != CHAIN_OK || op != 1) {
        Serial.printf("[MONO %u] set brightness failed (op=%u)\n", id, op);
        ok = false;
    }
    op = 0;
    chain_.setMonoClear(id, &op);
    return ok;
}

bool MonoDisplay::discover(uint8_t brightness) {
    count_ = 0;

    if (!chain_.isDeviceConnected(2, 60)) {
        Serial.println("[MONO] chain heartbeat failed");
        return false;
    }

    uint16_t total = 0;
    if (chain_.getDeviceNum(&total) != CHAIN_OK || total == 0 || total > 32) {
        Serial.printf("[MONO] device count invalid: %u\n", total);
        return false;
    }

    device_info_t infos[32] = {};
    device_list_t list = {total, infos};
    if (!chain_.getDeviceList(&list)) {
        Serial.println("[MONO] getDeviceList failed");
        return false;
    }

    uint8_t found = 0;
    for (uint16_t i = 0; i < total && found < 3; ++i) {
        Serial.printf("[MONO] id=%u type=%u\n", infos[i].id, infos[i].device_type);
        if (infos[i].device_type == CHAIN_MONO_TYPE_CODE) {
            ids_[found++] = static_cast<uint8_t>(infos[i].id);
        }
    }

    if (found == 0) {
        Serial.println("[MONO] no Chain Mono found");
        return false;
    }

    uint8_t okCount = 0;
    for (uint8_t i = 0; i < found; ++i) {
        if (configure(ids_[i], brightness)) {
            ids_[okCount++] = ids_[i];
        }
    }
    count_ = okCount;
    Serial.printf("[MONO] ready: %u module(s)\n", count_);
    return count_ > 0;
}

void MonoDisplay::setBrightness(uint8_t brightness) {
    brightness_ = brightness > 7 ? 7 : brightness;
    for (uint8_t i = 0; i < count_; ++i) {
        uint8_t op = 0;
        chain_.setMonoBrightness(ids_[i], static_cast<mono_brightness_level_t>(brightness_),
                                 &op, CHAIN_SAVE_FLASH_DISABLE);
    }
}

void MonoDisplay::buildFrame(const struct tm &t, bool colonOn, uint8_t frame[8][3], int modules) const {
    memset(frame, 0, sizeof(uint8_t) * 8 * 3);

    if (modules >= 3) {
        // 24x8: each 8px module carries one pair and (except seconds) its colon.
        // One blank column between the two digits makes the pair easier to read.
        // Module 1: "HH:", module 2: "MM:", module 3: "SS ".
        drawDigit(frame, 0, t.tm_hour / 10);
        drawDigit(frame, 4, t.tm_hour % 10);
        if (colonOn) drawColon(frame, 7);

        drawDigit(frame, 8, t.tm_min / 10);
        drawDigit(frame, 12, t.tm_min % 10);
        if (colonOn) drawColon(frame, 15);

        drawDigit(frame, 16, t.tm_sec / 10);
        drawDigit(frame, 20, t.tm_sec % 10);
    } else if (modules == 2) {
        // 16x8: "HH:" + "MM " with a one-pixel gap inside each pair.
        drawDigit(frame, 0, t.tm_hour / 10);
        drawDigit(frame, 4, t.tm_hour % 10);
        if (colonOn) drawColon(frame, 7);
        drawDigit(frame, 8, t.tm_min / 10);
        drawDigit(frame, 12, t.tm_min % 10);
    } else {
        // 8x8: "HH" with a one-pixel gap between the digits.
        drawDigit(frame, 0, t.tm_hour / 10);
        drawDigit(frame, 4, t.tm_hour % 10);
    }
}

void MonoDisplay::pushFrame(uint8_t frame[8][3], int modules) {
    for (int m = 0; m < modules; ++m) {
        int physical = order_ ? (modules - 1 - m) : m;
        uint8_t buffer[8];
        for (int row = 0; row < 8; ++row) {
            buffer[row] = frame[row][physical];
        }
        uint8_t op = 0;
        chain_status_t status = chain_.setMonoBufferRefresh(ids_[m], buffer, &op);
        if (status != CHAIN_OK || op != 1) {
            Serial.printf("[MONO %u] refresh failed status=%d op=%u\n", ids_[m], status, op);
        }
    }
}

void MonoDisplay::showTime(const struct tm &beijing, bool colonOn) {
    if (count_ == 0) return;
    uint8_t frame[8][3];
    int modules = count_ > 3 ? 3 : count_;
    buildFrame(beijing, colonOn, frame, modules);
    pushFrame(frame, modules);
}

void MonoDisplay::clear() {
    uint8_t frame[8][3] = {};
    pushFrame(frame, count_ > 3 ? 3 : count_);
}
