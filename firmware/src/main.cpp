#include <Arduino.h>
#include <time.h>

#include "config.h"
#include "mono_display.h"
#include "settings.h"
#include "ui.h"
#include "wifi_portal.h"

Settings settings;
ClockUi ui;
MonoDisplay monos;
WifiPortal net;

namespace {

uint32_t pageStartedMs = 0;
uint32_t lastMonoMs = 0;
uint32_t lastUiMs = 0;
uint32_t lastChainProbeMs = 0;
uint32_t lastSerialMs = 0;
int page = 0;
bool wasTimeValid = false;
bool lastPortal = false;

bool buttonDown = false;
uint32_t buttonDownMs = 0;
bool buttonLongHandled = false;

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
            page = (page + 1) % 2;
            pageStartedMs = now;
            Serial.printf("[BTN] short press: page %d\n", page);
        }
    }
}

void buildStatus(UiStatus &status, time_t now) {
    status.timeValid = now >= cfg::TIME_VALID_EPOCH;
    status.wifi = toUiState(net.state());
    status.chainCount = monos.count();
    status.page = page;
    status.english = settings.lang == 1;
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
    Serial.printf("[CFG] ssid='%s' order=%u lcd=%u chain=%u\n", settings.ssid.c_str(), settings.monoOrder,
                  settings.lcdBrightness, settings.chainBrightness);

    if (!ui.begin()) {
        Serial.println("[UI] fatal: display init failed");
    }
    ui.setBrightness(settings.lcdBrightness);

    monos.setOrder(settings.monoOrder);
    monos.begin(settings.chainBrightness);

    net.begin(settings);
    pageStartedMs = millis();
    lastUiMs = 0;
}

void loop() {
    net.loop();
    updateButton();

    time_t now = time(nullptr);
    const bool timeValid = now >= cfg::TIME_VALID_EPOCH;

    if (timeValid) {
        // Beijing is fixed UTC+8 (no DST). The Chain Mono modules show this time.
        time_t beijingEpoch = now + 8 * 3600;
        struct tm beijing = {};
        gmtime_r(&beijingEpoch, &beijing);

        if (!wasTimeValid) {
            wasTimeValid = true;
            Serial.println("[TIME] clock synchronized");
        }

        if (millis() - lastMonoMs >= 500) {
            bool colonOn = ((millis() / 500) & 1) == 0;
            monos.showTime(beijing, colonOn);
            lastMonoMs = millis();
        }
    } else {
        if (wasTimeValid) {
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
    if (millis() - lastUiMs >= 1000 || portal != lastPortal) {
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
