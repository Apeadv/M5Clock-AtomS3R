#pragma once

#include <Arduino.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFi.h>

#include "settings.h"

enum class NetState {
    Offline,
    Connecting,
    Connected,
    Portal,
};

class WifiPortal {
public:
    void begin(Settings &settings);
    void loop();

    NetState state() const { return state_; }
    bool timeValid() const;
    String ip() const;
    String apSsid() const { return apSsid_; }
    String hostname() const { return hostname_; }

    void startNtp();

private:
    void startPortal();
    void onConnected();
    void ensureServer();
    void handleRoot();
    void handleSave();
    void handleReset();
    void handleLang();
    void handleStatus();
    void handleNotFound();
    void redirectPortal();
    String buildPage();
    String successPage(const String &title, const String &message, bool english);
    bool english() const { return settings_ != nullptr && settings_->lang == 1; }

    Settings *settings_ = nullptr;
    WebServer server_{80};
    DNSServer dns_;
    NetState state_ = NetState::Offline;
    bool serverStarted_ = false;
    bool everConnected_ = false;
    uint32_t connectStartMs_ = 0;
    uint32_t lastReconnectMs_ = 0;
    uint32_t lastNtpMs_ = 0;
    String apSsid_;
    String hostname_;
};
