#pragma once

#include <Arduino.h>
#include <time.h>

namespace cfg {

// AtomS3R + Atomic toChain Base wiring.
// The Chain port uses the two Grove pins on the AtomS3R bottom header.
constexpr int PIN_CHAIN_RX = 6;
constexpr int PIN_CHAIN_TX = 5;
constexpr int PIN_BUTTON = 41;

// Grove Port A for an external PDM microphone (M5Stack Unit Mini PDM):
// Data -> G1, Clock -> G2, 5V/GND from the Grove connector.
constexpr int PIN_PDM_DATA = 1;
constexpr int PIN_PDM_CLK = 2;

// Chain Mono supports 0..7 brightness levels. Level 6 is bright but not blinding.
constexpr uint8_t DEFAULT_CHAIN_BRIGHTNESS = 6;
constexpr uint8_t DEFAULT_LCD_BRIGHTNESS = 180;

// World-clock page rotation.
constexpr uint32_t PAGE_ROTATE_MS = 7000;

// Wi-Fi / NTP.
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
constexpr uint32_t WIFI_RETRY_INTERVAL_MS = 15000;
constexpr uint32_t NTP_RETRY_INTERVAL_MS = 30000;

// Button.
constexpr uint32_t BUTTON_DEBOUNCE_MS = 25;
constexpr uint32_t BUTTON_DOUBLE_CLICK_MS = 350;
constexpr uint32_t BUTTON_LONG_PRESS_MS = 3000;

// A sane lower bound for "the clock has been set" (2023-11-14).
constexpr time_t TIME_VALID_EPOCH = 1700000000;

constexpr const char *AP_PREFIX = "M5-Clock-";
constexpr const char *HOSTNAME = "m5-clock";
constexpr const char *NTP_SERVER_1 = "ntp.aliyun.com";
constexpr const char *NTP_SERVER_2 = "ntp.tencent.com";
constexpr const char *NTP_SERVER_3 = "pool.ntp.org";

}  // namespace cfg
