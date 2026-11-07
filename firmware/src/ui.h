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
    bool english = false;
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
    void drawHeader(bool english, WifiUiState wifi);
    void drawWifiIcon(WifiUiState state);

    M5GFX display_;
    M5Canvas canvas_{&display_};
    bool ready_ = false;
};
