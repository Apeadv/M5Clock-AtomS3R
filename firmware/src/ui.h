#pragma once

#include <Arduino.h>
#include <time.h>

#include "M5GFX.h"

enum class WifiUiState {
    Offline,
    Connecting,
    Connected,
    Portal,
};

struct UiStatus {
    bool timeValid = false;
    WifiUiState wifi = WifiUiState::Offline;
    int chainCount = 0;
    int page = 0;
    uint8_t region = 0;
    bool english = false;
    bool spectrum = false;
    bool audioReady = false;
    uint8_t audioSource = 0;
    uint8_t bands[12] = {};
    String subtitle;
    uint16_t subtitleMsPerPixel = 40;
    String apSsid;
    String ip;
};

class ClockUi {
public:
    bool begin();
    void setBrightness(uint8_t value);
    void render(const UiStatus &status, time_t utcNow);

private:
    void renderPortal(const UiStatus &status);
    void renderOffline(const UiStatus &status);
    void renderClock(const UiStatus &status, time_t utcNow);
    void renderEnglishPortal(const UiStatus &status);
    void renderEnglishOffline(const UiStatus &status);
    void renderEnglishClock(const UiStatus &status, time_t utcNow);
    void renderSpectrum(const UiStatus &status);
    void drawHeader(bool english, WifiUiState wifi);
    void drawWifiIcon(WifiUiState state);

    M5GFX display_;
    M5Canvas canvas_{&display_};
    bool ready_ = false;
    float spectrumBars_[12] = {};
    float spectrumPeaks_[12] = {};
    uint32_t spectrumLastMs_ = 0;
    bool spectrumActive_ = false;
};
