#include <Arduino.h>
#include <time.h>

#include "audio_spectrum.h"
#include "usb_spectrum.h"
#include "clock_data.h"
#include "config.h"
#include "mono_display.h"
#include "settings.h"
#include "ui.h"
#include "wifi_portal.h"

Settings settings;
ClockUi ui;
MonoDisplay monos;
WifiPortal net;
AudioSpectrum audio;
UsbSpectrum usbAudio;

namespace {

uint32_t pageStartedMs = 0;
uint32_t lastMonoMs = 0;
uint32_t customModeStartMs = 0;
uint32_t lastUiMs = 0;
uint32_t lastChainProbeMs = 0;
uint32_t lastSerialMs = 0;
int page = 0;
bool wasTimeValid = false;
bool lastPortal = false;
uint8_t lastMonoMode = 255;
uint8_t screenBeforeSpectrum = 0;

bool buttonDown = false;
uint32_t buttonDownMs = 0;
bool buttonLongHandled = false;
bool pendingSingle = false;
uint32_t pendingSingleAt = 0;

uint16_t scrollMsForSpeed(uint8_t speed) {
    switch (speed) {
        case 0: return 200;
        case 2: return 70;
        default: return 120;
    }
}

const char *speedName(uint8_t speed) {
    switch (speed) {
        case 0: return "slow";
        case 2: return "fast";
        default: return "normal";
    }
}

WifiUiState toUiState(NetState state) {
    switch (state) {
        case NetState::Connected: return WifiUiState::Connected;
        case NetState::Connecting: return WifiUiState::Connecting;
        case NetState::Portal: return WifiUiState::Portal;
        default: return WifiUiState::Offline;
    }
}

void updateButton() {
    const bool down = digitalRead(cfg::PIN_BUTTON) == LOW;
    const uint32_t now = millis();

    if (down && !buttonDown) {
        buttonDown = true;
        buttonDownMs = now;
        buttonLongHandled = false;
        return;
    }

    if (down && buttonDown && !buttonLongHandled && now - buttonDownMs >= cfg::BUTTON_LONG_PRESS_MS) {
        buttonLongHandled = true;
        Serial.println("[BTN] long press: clearing Wi-Fi and restarting");
        settings.clearWifi();
        delay(150);
        ESP.restart();
        return;
    }

    if (!down && buttonDown) {
        buttonDown = false;
        if (!buttonLongHandled && now - buttonDownMs >= 40) {
            if (pendingSingle && now - pendingSingleAt <= cfg::BUTTON_DOUBLE_CLICK_MS) {
                pendingSingle = false;
                settings.scrollSpeed = (settings.scrollSpeed + 1) % 3;
                settings.save();
                lastMonoMs = 0;
                Serial.printf("[BTN] double click: scroll speed=%s\n", speedName(settings.scrollSpeed));
            } else {
                pendingSingle = true;
                pendingSingleAt = now;
            }
        }
    }
}

void processPendingSingleClick() {
    if (!pendingSingle || buttonDown) return;
    if (millis() - pendingSingleAt < cfg::BUTTON_DOUBLE_CLICK_MS) return;

    pendingSingle = false;
    const uint8_t previousMode = settings.monoMode;
    const uint8_t nextMode = (previousMode + 1) % 3;

    if (settings.syncScreen) {
        if (nextMode == 2 && previousMode != 2) {
            screenBeforeSpectrum = settings.screenMode;
            settings.screenMode = 1;
        } else if (nextMode != 2 && previousMode == 2) {
            settings.screenMode = screenBeforeSpectrum;
        }
    }

    settings.monoMode = nextMode;
    settings.save();
    lastMonoMs = 0;
    lastUiMs = 0;
    const char *modeName = nextMode == 0 ? "clock"
                           : nextMode == 1 ? "subtitle"
                                           : "spectrum";
    Serial.printf("[BTN] single click: chain mode=%s, screen=%s\n", modeName,
                  settings.screenMode ? "spectrum" : "world clock");
}

void buildStatus(UiStatus &status, time_t now) {
    status.timeValid = now >= cfg::TIME_VALID_EPOCH;
    status.wifi = toUiState(net.state());
    status.chainCount = monos.count();
    status.page = page;
    status.region = settings.region;
    status.english = settings.lang == 1;
    status.spectrum = settings.screenMode == 1;
    status.audioSource = settings.audioSource;
    if (settings.audioSource == 0) {
        status.audioReady = usbAudio.ready();
        if (status.audioReady) {
            for (int i = 0; i < 12; ++i) status.bands[i] = usbAudio.levels()[i];
        }
    } else {
        status.audioReady = audio.ready() && audio.signal();
        for (int i = 0; i < 12; ++i) status.bands[i] = audio.levels()[i];
    }
    status.subtitle = settings.monoText;
    status.subtitleMsPerPixel = scrollMsForSpeed(settings.scrollSpeed) / 3;
    status.apSsid = net.apSsid();
    status.ip = net.ip();
    (void)now;
}

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(150);
    Serial.println();
    Serial.println("M5Stack AtomS3R world clock");
    Serial.println("Chain Mono x3 + 128x128 world clock");

    pinMode(cfg::PIN_BUTTON, INPUT_PULLUP);

    settings.load();
    if (settings.syncScreen && settings.monoMode == 2 && settings.screenMode == 0) {
        settings.screenMode = 1;
        settings.save();
    }
    Serial.printf("[CFG] ssid='%s' order=%u lcd=%u chain=%u screen=%u region=%u utc=%d speed=%u pos=%u text='%s' audioSource=%u audioReady=%d\n",
                  settings.ssid.c_str(), settings.monoOrder, settings.lcdBrightness,
                  settings.chainBrightness, settings.screenMode, settings.region,
                  settings.customUtcOffset, settings.scrollSpeed, settings.textPosition,
                  settings.monoText.c_str(), settings.audioSource, audio.ready());

    if (!ui.begin()) {
        Serial.println("[UI] fatal: display init failed");
    }
    ui.setBrightness(settings.lcdBrightness);

    monos.setOrder(settings.monoOrder);
    monos.begin(settings.chainBrightness);
    if (settings.audioSource == 1) {
        audio.begin();
    }

    net.begin(settings);
    pageStartedMs = millis();
    lastUiMs = 0;
}

void loop() {
    net.loop();
    updateButton();
    processPendingSingleClick();

    time_t now = time(nullptr);
    const bool timeValid = now >= cfg::TIME_VALID_EPOCH;

    const bool spectrumActive = settings.screenMode == 1;
    if (spectrumActive) {
        if (settings.audioSource == 0) {
            usbAudio.loop();
        } else {
            audio.loop();
        }
    }

    const uint8_t monoMode = settings.monoMode;
    if (monoMode != lastMonoMode) {
        lastMonoMode = monoMode;
        lastMonoMs = 0;
        if (monoMode == 1) customModeStartMs = millis();
        if (monoMode == 0 && !timeValid) monos.clear();
    }

    if (monoMode == 2) {
        // Chain Mono spectrum, independent from the S3R screen.
        if (millis() - lastMonoMs >= 80) {
            static const uint8_t zeroBands[12] = {};
            const uint8_t *bands = settings.audioSource == 0 ? usbAudio.levels() : audio.levels();
            if (settings.audioSource == 0 && !usbAudio.ready()) bands = zeroBands;
            monos.showSpectrum(bands);
            lastMonoMs = millis();
        }
    } else if (monoMode == 1) {
        if (millis() - lastMonoMs >= 120) {
            monos.showText(settings.monoText.c_str(), millis() - customModeStartMs,
                           scrollMsForSpeed(settings.scrollSpeed), settings.textPosition);
            lastMonoMs = millis();
        }
    } else {
        if (timeValid) {
            struct tm region = regionTime(now, settings.region, settings.customUtcOffset);

            if (!wasTimeValid) {
                wasTimeValid = true;
                Serial.println("[TIME] clock synchronized");
            }

            if (millis() - lastMonoMs >= 500) {
                bool colonOn = ((millis() / 500) & 1) == 0;
                monos.showTime(region, colonOn, settings.textPosition);
                lastMonoMs = millis();
            }
        } else if (wasTimeValid) {
            wasTimeValid = false;
            monos.clear();
        }
    }

    if (!monos.ready() && millis() - lastChainProbeMs >= 5000) {
        lastChainProbeMs = millis();
        if (monos.reconnect(settings.chainBrightness)) {
            monos.setOrder(settings.monoOrder);
            lastMonoMs = 0;
            Serial.printf("[MONO] reconnected: %d module(s)\n", monos.count());
        }
    }

    if (millis() - pageStartedMs >= cfg::PAGE_ROTATE_MS) {
        pageStartedMs = millis();
        page = (page + 1) % 2;
    }

    UiStatus status;
    buildStatus(status, now);

    bool portal = net.state() == NetState::Portal;
    const uint32_t uiInterval = status.spectrum ? 33 : 1000;
    if (millis() - lastUiMs >= uiInterval || portal != lastPortal) {
        lastUiMs = millis();
        lastPortal = portal;
        ui.render(status, now);
    }

    if (millis() - lastSerialMs >= 30000) {
        lastSerialMs = millis();
        Serial.printf("[STATUS] wifi=%d ip=%s time=%d mono=%d page=%d heap=%lu\n",
                      static_cast<int>(net.state()), net.ip().c_str(), timeValid, monos.count(), page,
                      static_cast<unsigned long>(ESP.getFreeHeap()));
    }

    delay(20);
}
