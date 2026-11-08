#include "wifi_portal.h"

#include <ESPmDNS.h>

#include "clock_data.h"
#include "config.h"

namespace {

String htmlEscape(const String &in) {
    String out;
    out.reserve(in.length() + 16);
    for (size_t i = 0; i < in.length(); ++i) {
        char c = in[i];
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            case '\'': out += "&#39;"; break;
            default: out += c; break;
        }
    }
    return out;
}

String sanitizeMonoText(const String &in) {
    String out;
    out.reserve(32);
    for (size_t i = 0; i < in.length() && out.length() < 32; ++i) {
        char c = in[i];
        if (c >= 32 && c <= 126) out += c;
    }
    out.trim();
    return out;
}

String stateName(NetState state) {
    switch (state) {
        case NetState::Connected: return "connected";
        case NetState::Connecting: return "connecting";
        case NetState::Portal: return "portal";
        default: return "offline";
    }
}

}  // namespace

void WifiPortal::begin(Settings &settings) {
    settings_ = &settings;

    WiFi.persistent(false);
    WiFi.setAutoReconnect(false);
    WiFi.mode(WIFI_STA);

    uint8_t mac[6] = {0};
    WiFi.macAddress(mac);
    char suffix[8];
    snprintf(suffix, sizeof(suffix), "%02x%02x", mac[4], mac[5]);
    apSsid_ = String(cfg::AP_PREFIX) + suffix;
    apSsid_.toUpperCase();
    hostname_ = String("m5-clock-") + suffix;
    WiFi.setHostname(hostname_.c_str());

    if (settings.hasWifi()) {
        Serial.printf("[WiFi] connecting to %s\n", settings.ssid.c_str());
        WiFi.begin(settings.ssid.c_str(), settings.pass.c_str());
        state_ = NetState::Connecting;
        connectStartMs_ = millis();
        lastReconnectMs_ = connectStartMs_;
    } else {
        Serial.println("[WiFi] no credentials, starting portal");
        startPortal();
    }
}

void WifiPortal::startPortal() {
    if (state_ == NetState::Portal) return;

    Serial.println("[WiFi] starting setup AP");
    WiFi.disconnect(true);
    delay(100);
    WiFi.mode(WIFI_AP_STA);
    delay(100);

    bool ok = WiFi.softAP(apSsid_.c_str());
    Serial.printf("[WiFi] softAP(%s) -> %d, IP=%s\n", apSsid_.c_str(), ok, WiFi.softAPIP().toString().c_str());
    delay(150);

    dns_.start(53, "*", WiFi.softAPIP());
    state_ = NetState::Portal;
    ensureServer();
}

void WifiPortal::onConnected() {
    if (state_ == NetState::Connected) return;

    state_ = NetState::Connected;
    everConnected_ = true;
    WiFi.setSleep(false);
    Serial.printf("[WiFi] connected, IP=%s RSSI=%d\n", WiFi.localIP().toString().c_str(), WiFi.RSSI());

    ensureServer();

    if (MDNS.begin(hostname_.c_str())) {
        MDNS.addService("http", "tcp", 80);
        Serial.printf("[WiFi] mDNS: http://%s.local/\n", hostname_.c_str());
    } else {
        Serial.println("[WiFi] mDNS start failed");
    }

    startNtp();
}

void WifiPortal::startNtp() {
    configTime(0, 0, cfg::NTP_SERVER_1, cfg::NTP_SERVER_2, cfg::NTP_SERVER_3);
    resetTimezoneToUtc();
    lastNtpMs_ = millis();
    Serial.println("[NTP] sync requested");
}

bool WifiPortal::timeValid() const {
    return time(nullptr) >= cfg::TIME_VALID_EPOCH;
}

String WifiPortal::ip() const {
    if (state_ == NetState::Portal) return WiFi.softAPIP().toString();
    return WiFi.localIP().toString();
}

void WifiPortal::loop() {
    if (settings_ == nullptr) return;

    if (state_ == NetState::Portal) {
        dns_.processNextRequest();
        server_.handleClient();
        return;
    }

    if (state_ == NetState::Connected) {
        if (WiFi.status() == WL_CONNECTED) {
            server_.handleClient();
            if (!timeValid() && millis() - lastNtpMs_ >= cfg::NTP_RETRY_INTERVAL_MS) {
                startNtp();
            }
            return;
        }
        state_ = NetState::Connecting;
        connectStartMs_ = millis() - cfg::WIFI_CONNECT_TIMEOUT_MS;  // do not fall back to AP after a drop
        lastReconnectMs_ = millis();
        WiFi.reconnect();
    }

    if (state_ == NetState::Connecting) {
        if (WiFi.status() == WL_CONNECTED) {
            onConnected();
            return;
        }
        if (millis() - lastReconnectMs_ >= cfg::WIFI_RETRY_INTERVAL_MS) {
            WiFi.reconnect();
            lastReconnectMs_ = millis();
        }
        if (!everConnected_ && millis() - connectStartMs_ >= cfg::WIFI_CONNECT_TIMEOUT_MS) {
            startPortal();
        }
    }
}

void WifiPortal::ensureServer() {
    if (serverStarted_) return;

    server_.on("/", HTTP_GET, [this]() { handleRoot(); });
    server_.on("/save", HTTP_POST, [this]() { handleSave(); });
    server_.on("/reset", HTTP_POST, [this]() { handleReset(); });
    server_.on("/lang", HTTP_GET, [this]() { handleLang(); });
    server_.on("/status", HTTP_GET, [this]() { handleStatus(); });

    auto captive = [this]() {
        if (state_ == NetState::Portal) {
            redirectPortal();
        } else {
            server_.send(204, "text/plain", "");
        }
    };
    server_.on("/generate_204", HTTP_GET, captive);
    server_.on("/hotspot-detect.html", HTTP_GET, captive);
    server_.on("/connecttest.txt", HTTP_GET, captive);
    server_.on("/ncsi.txt", HTTP_GET, captive);
    server_.on("/favicon.ico", HTTP_GET, [this]() { server_.send(204, "text/plain", ""); });

    server_.onNotFound([this]() { handleNotFound(); });
    server_.begin();
    serverStarted_ = true;
    Serial.println("[HTTP] server started");
}

void WifiPortal::redirectPortal() {
    server_.sendHeader("Location", "http://192.168.4.1/", true);
    server_.send(302, "text/plain", "");
}

void WifiPortal::handleRoot() {
    server_.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    server_.sendHeader("Pragma", "no-cache");
    server_.send(200, "text/html; charset=utf-8", buildPage());
}

void WifiPortal::handleSave() {
    const bool en = english();
    String ssid = server_.arg("ssid");
    ssid.trim();
    if (ssid.length() == 0) {
        server_.send(400, "text/html; charset=utf-8",
                     successPage(en ? "Wi-Fi name required" : "缺少 Wi-Fi 名称",
                                 en ? "Go back and select or enter a Wi-Fi name."
                                    : "请返回上一页，选择或输入一个 Wi-Fi 名称。",
                                 en));
        return;
    }

    String pass = server_.arg("pass");
    if (pass.length() == 0 && ssid == settings_->ssid) {
        pass = settings_->pass;
    }

    settings_->ssid = ssid;
    settings_->pass = pass;
    settings_->monoOrder = server_.arg("order").toInt() ? 1 : 0;
    settings_->chainBrightness = constrain(server_.arg("chain").toInt(), 0, 7);
    settings_->lcdBrightness = constrain(server_.arg("lcd").toInt(), 30, 255);
    settings_->region = constrain(server_.arg("region").toInt(), 0, REGION_MAX);
    settings_->customUtcOffset = constrain(server_.arg("utcOffset").toInt(), -12, 14);
    settings_->audioSource = constrain(server_.arg("audioSource").toInt(), 0, 1);
    settings_->screenMode = constrain(server_.arg("screenMode").toInt(), 0, 1);
    settings_->syncScreen = constrain(server_.arg("syncScreen").toInt(), 0, 1);
    settings_->scrollSpeed = constrain(server_.arg("speed").toInt(), 0, 2);
    settings_->textPosition = constrain(server_.arg("textPosition").toInt(), 0, 1);
    String monoText = sanitizeMonoText(server_.arg("monoText"));
    if (monoText.length() > 0) settings_->monoText = monoText;
    settings_->save();

    Serial.printf("[CFG] saved ssid=%s order=%u chain=%u lcd=%u screen=%u region=%u utc=%d audio=%u speed=%u pos=%u text='%s'\n",
                  settings_->ssid.c_str(), settings_->monoOrder, settings_->chainBrightness,
                  settings_->lcdBrightness, settings_->screenMode, settings_->region,
                  settings_->customUtcOffset, settings_->audioSource, settings_->scrollSpeed,
                  settings_->textPosition, settings_->monoText.c_str());

    server_.send(200, "text/html; charset=utf-8",
                 successPage(en ? "Saved" : "保存成功",
                             en ? "The device is connecting to Wi-Fi and rebooting. Please wait..."
                                : "设备正在连接 Wi-Fi 并重启，请稍候…",
                             en));
    server_.client().flush();
    delay(1200);
    ESP.restart();
}

void WifiPortal::handleReset() {
    const bool en = english();
    settings_->clearWifi();
    server_.send(200, "text/html; charset=utf-8",
                 successPage(en ? "Wi-Fi cleared" : "已清除 Wi-Fi",
                             en ? "The device will reboot into setup mode. Reconnect to configure it."
                                : "设备重启后将进入配网模式，请重新连接。",
                             en));
    server_.client().flush();
    delay(900);
    ESP.restart();
}

void WifiPortal::handleLang() {
    if (settings_ == nullptr) return;
    settings_->lang = server_.arg("to") == "en" ? 1 : 0;
    settings_->save();
    Serial.printf("[HTTP] language -> %s\n", settings_->lang ? "English" : "中文");

    // Render the page directly instead of redirecting. Some captive-portal
    // browsers cache the redirect target, which made the switch appear to do
    // nothing. No-store headers plus a direct 200 response make it reliable.
    server_.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    server_.sendHeader("Pragma", "no-cache");
    server_.send(200, "text/html; charset=utf-8", buildPage());
}

void WifiPortal::handleStatus() {
    String json = "{\"state\":\"";
    json += stateName(state_);
    json += "\",\"ip\":\"";
    json += ip();
    json += "\",\"rssi\":";
    json += String(WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0);
    json += ",\"time\":";
    json += timeValid() ? "true" : "false";
    json += "}";
    server_.send(200, "application/json", json);
}

void WifiPortal::handleNotFound() {
    if (state_ == NetState::Portal) {
        redirectPortal();
    } else {
        server_.send(404, "text/plain; charset=utf-8", "Not found");
    }
}

String WifiPortal::successPage(const String &title, const String &message, bool en) {
    String p;
    p.reserve(1200);
    p += "<!DOCTYPE html><html lang=\"";
    p += en ? "en" : "zh-CN";
    p += "\"><head><meta charset=\"utf-8\">";
    p += "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">";
    p += "<title>";
    p += htmlEscape(title);
    p += "</title><style>";
    p += "body{margin:0;min-height:100vh;display:grid;place-items:center;background:#071018;color:#eaf6ff;"
         "font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;padding:24px}";
    p += ".box{max-width:420px;width:100%;background:#0e1b26;border:1px solid #1d3546;border-radius:22px;"
         "padding:28px;box-shadow:0 20px 60px rgba(0,0,0,.45)}";
    p += "h2{margin:0 0 10px;font-size:22px}p{margin:0;color:#9db2c4;line-height:1.7}";
    p += ".bar{height:4px;border-radius:2px;background:linear-gradient(90deg,#23d5e6,#2d8cff);margin-top:22px}";
    p += "</style></head><body><div class=\"box\"><h2>";
    p += htmlEscape(title);
    p += "</h2><p>";
    p += htmlEscape(message);
    p += "</p><div class=\"bar\"></div></div></body></html>";
    return p;
}

String WifiPortal::buildPage() {
    const bool en = english();
    const bool portal = state_ == NetState::Portal;
    const String ssid = htmlEscape(settings_->ssid);
    const String monoTextValue = htmlEscape(settings_->monoText);
    const String ap = htmlEscape(apSsid_);
    const String deviceIp = htmlEscape(ip());

    String statusText;
    String statusClass;
    String heading;
    String sub;
    String meta;

    if (portal) {
        statusText = en ? "SETUP" : "配网模式";
        statusClass = "warn";
        heading = en ? "Connect Wi-Fi" : "连接 Wi-Fi";
        sub = en ? "Configure the network for your world clock" : "为世界时钟配置网络";
        meta = "AP: " + ap + (en ? " · Open 192.168.4.1" : " · 浏览器打开 192.168.4.1");
    } else if (state_ == NetState::Connected) {
        statusText = en ? "CONNECTED" : "已连接";
        statusClass = "ok";
        heading = en ? "Device settings" : "设备设置";
        sub = en ? "The world clock is running" : "世界时钟正在运行";
        meta = "IP: " + deviceIp + " · " + htmlEscape(hostname_) + ".local";
    } else if (state_ == NetState::Connecting) {
        statusText = en ? "CONNECTING" : "连接中";
        statusClass = "warn";
        heading = en ? "Connecting" : "正在连接";
        sub = en ? "Please wait, or reconfigure the network" : "请稍候，或重新配置网络";
        meta = "SSID: " + ssid;
    } else {
        statusText = en ? "OFFLINE" : "未连接";
        statusClass = "bad";
        heading = en ? "Network offline" : "网络未连接";
        sub = en ? "Please configure Wi-Fi again" : "请重新配置 Wi-Fi";
        meta = (en ? "Device AP: " : "设备热点: ") + ap;
    }

    String scanOptions;
    if (portal) {
        int n = WiFi.scanNetworks();
        for (int i = 0; i < n; ++i) {
            String name = WiFi.SSID(i);
            if (name.length() == 0) continue;
            String safe = htmlEscape(name);
            if (name == settings_->ssid && ssid.length()) {
                scanOptions += "<option value=\"" + safe + "\" selected>";
            } else {
                scanOptions += "<option value=\"" + safe + "\">";
            }
            scanOptions += safe + " (" + String(WiFi.RSSI(i)) + " dBm)</option>";
        }
        WiFi.scanDelete();
    }

    const String passPlaceholder = settings_->pass.length()
                                       ? (en ? "Leave blank to keep the current password"
                                             : "留空则保持原密码")
                                       : (en ? "Leave blank for an open network"
                                             : "开放网络可留空");

    String regionOptions;
    for (int i = 0; i < REGION_FIXED_COUNT; ++i) {
        regionOptions += "<option value=\"" + String(i) + "\"";
        if (settings_->region == i) regionOptions += " selected";
        regionOptions += ">";
        regionOptions += en ? REGIONS[i].en : REGIONS[i].cn;
        regionOptions += "</option>";
    }
    regionOptions += "<option value=\"" + String(REGION_CUSTOM) + "\"";
    if (settings_->region == REGION_CUSTOM) regionOptions += " selected";
    regionOptions += ">";
    regionOptions += en ? "Custom UTC+X" : "自定义 UTC+X";
    regionOptions += "</option>";

    String p;
    p.reserve(11000);
    p += "<!DOCTYPE html><html lang=\"";
    p += en ? "en" : "zh-CN";
    p += "\"><head><meta charset=\"utf-8\">";
    p += "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1,viewport-fit=cover\">";
    p += "<meta name=\"theme-color\" content=\"#071018\">";
    p += "<title>";
    p += en ? "M5 World Clock · Setup" : "M5 世界时钟 · 设置";
    p += "</title><style>";
    p += ":root{--bg:#071018;--card:#0d1a25;--line:#1b3142;--text:#eaf6ff;--muted:#8ba1b3;"
         "--cyan:#23d5e6;--blue:#2d8cff;--amber:#ffb547;--red:#ff5c72;--green:#35e08a}";
    p += "*{box-sizing:border-box}html,body{margin:0;min-height:100%}";
    p += "body{background:radial-gradient(900px 500px at 50% -10%,#123246 0%,#071018 55%,#040a0f 100%);"
         "color:var(--text);font-family:-apple-system,BlinkMacSystemFont,'Segoe UI','PingFang SC',"
         "'Microsoft YaHei',sans-serif;display:grid;place-items:center;padding:22px}";
    p += ".card{width:min(460px,100%);background:linear-gradient(180deg,rgba(16,32,44,.96),rgba(9,20,29,.98));"
         "border:1px solid var(--line);border-radius:26px;padding:24px;box-shadow:0 28px 80px rgba(0,0,0,.55);"
         "backdrop-filter:blur(12px)}";
    p += ".top{display:flex;align-items:center;justify-content:space-between;gap:12px}";
    p += ".brand{display:flex;align-items:center;gap:10px;font-size:12px;letter-spacing:.18em;color:#a9c2d4;"
         "font-weight:700}";
    p += ".logo{display:grid;place-items:center;width:34px;height:34px;border-radius:11px;"
         "background:linear-gradient(135deg,var(--cyan),var(--blue));color:#04121a;font-size:13px;"
         "letter-spacing:0;font-weight:900}";
    p += ".pill{font-size:12px;padding:7px 11px;border-radius:999px;border:1px solid var(--line);"
         "color:var(--muted);white-space:nowrap}";
    p += ".pill.ok{color:var(--green);border-color:rgba(53,224,138,.35);background:rgba(53,224,138,.08)}";
    p += ".pill.warn{color:var(--amber);border-color:rgba(255,181,71,.35);background:rgba(255,181,71,.08)}";
    p += ".pill.bad{color:var(--red);border-color:rgba(255,92,114,.35);background:rgba(255,92,114,.08)}";
    p += ".langs{display:flex;gap:7px;margin:14px 0 0;font-size:12px}";
    p += ".langs a{padding:6px 11px;border:1px solid var(--line);border-radius:999px;color:var(--muted);"
         "text-decoration:none;transition:color .2s,border-color .2s,background .2s}";
    p += ".langs a.active{color:var(--cyan);border-color:rgba(35,213,230,.4);"
         "background:rgba(35,213,230,.08)}";
    p += "a{color:var(--cyan)}";
    p += "h1{font-size:27px;line-height:1.2;margin:20px 0 6px;letter-spacing:-.02em}";
    p += ".sub{color:var(--muted);font-size:14px;margin:0 0 14px}";
    p += ".meta{font-size:12px;color:#a9c2d4;background:#08131c;border:1px solid #152a3a;border-radius:12px;"
         "padding:10px 12px;line-height:1.6;word-break:break-all;margin-bottom:18px}";
    p += "label{display:flex;align-items:center;justify-content:space-between;gap:10px;font-size:13px;"
         "color:#b9cddd;margin:14px 0 7px}label b{color:var(--cyan);font-weight:700}";
    p += "input,select{width:100%;border:1px solid #1d3546;background:#07131c;color:var(--text);"
         "border-radius:13px;padding:13px 14px;font-size:16px;outline:none;transition:border-color .2s,"
         "box-shadow .2s}input:focus,select:focus{border-color:var(--cyan);box-shadow:0 0 0 3px rgba(35,213,230,.12)}";
    p += "input::placeholder{color:#5f7486}input[type=range]{padding:0;height:30px;accent-color:var(--cyan);"
         "background:transparent;border:0;box-shadow:none}";
    p += ".row{display:grid;grid-template-columns:1fr 1fr;gap:12px}";
    p += "button{width:100%;margin-top:18px;border:0;border-radius:14px;padding:14px 16px;font-size:16px;"
         "font-weight:800;letter-spacing:.02em;color:#03131c;cursor:pointer;"
         "background:linear-gradient(135deg,var(--cyan),var(--blue));box-shadow:0 12px 28px rgba(35,150,220,.25)}";
    p += "button:active{transform:translateY(1px)}button.ghost{margin-top:10px;background:transparent;"
         "border:1px solid #243d50;color:#9fb6c8;box-shadow:none;font-weight:600}";
    p += ".foot{font-size:11px;color:#637b8d;line-height:1.7;margin:18px 0 0;text-align:center}";
    p += ".modebox{margin-top:18px;padding-top:14px;border-top:1px dashed #1d3546}";
    p += ".help{font-size:11px;color:#637b8d;line-height:1.5;margin:6px 2px 0}";
    p += "@media(max-width:380px){.row{grid-template-columns:1fr}h1{font-size:24px}.card{padding:20px}}";
    p += "</style></head><body><main class=\"card\">";
    p += "<div class=\"top\"><div class=\"brand\"><span class=\"logo\">M5</span><span>WORLD CLOCK</span></div>";
    p += "<span class=\"pill " + statusClass + "\">" + statusText + "</span></div>";
    p += "<h1>" + heading + "</h1><p class=\"sub\">" + sub + "</p>";
    p += "<div class=\"meta\">" + meta + "</div>";

    p += "<div class=\"langs\">";
    p += "<a class=\"";
    p += en ? "" : "active";
    p += "\" href=\"/lang?to=zh\">中文</a>";
    p += "<a class=\"";
    p += en ? "active" : "";
    p += "\" href=\"/lang?to=en\">English</a></div>";

    const char *lblSsid = en ? "Wi-Fi name" : "Wi-Fi 名称";
    const char *lblPass = en ? "Wi-Fi password" : "Wi-Fi 密码";
    const char *lblOrder = en ? "Mono order" : "Mono 顺序";
    const char *lblChain = en ? "Mono brightness" : "Mono 亮度";
    const char *lblLcd = en ? "Screen brightness" : "屏幕亮度";
    const char *lblScreen = en ? "AtomS3R screen" : "AtomS3R 屏幕";
    const char *optSpectrum = en ? "Music spectrum" : "音乐频谱";
    const char *optWorld = en ? "World clock" : "世界时间";
    const char *lblSync = en ? "Sync S3R screen" : "S3R 同步切换";
    const char *syncOn = en ? "Follow Chain spectrum" : "切到频谱时同步显示";
    const char *syncOff = en ? "Keep S3R screen" : "保持 S3R 当前界面";
    const char *lblAudio = en ? "Spectrum audio source" : "频谱音频来源";
    const char *audioUsb = en ? "USB PC" : "USB 电脑";
    const char *audioPdm = en ? "PDM microphone" : "PDM 麦克风";
    const char *lblRegion = en ? "Dot-matrix time zone" : "点阵屏时区";
    const char *lblCustomUtc = en ? "Custom UTC offset (hours)" : "自定义 UTC 偏移（小时）";
    const char *hintCustomUtc = en ? "Integer hours, -12 to +14. For example, 3 means UTC+3."
                                  : "整数小时，-12 到 +14。例如 3 表示 UTC+3。";
    const char *lblSpeed = en ? "Subtitle scroll speed" : "字幕滚动速度";
    const char *speedSlow = en ? "Slow" : "慢";
    const char *speedNormal = en ? "Normal" : "正常";
    const char *speedFast = en ? "Fast" : "快";
    const char *lblPos = en ? "Dot-matrix vertical position" : "点阵屏垂直位置";
    const char *posTop = en ? "Top" : "上方";
    const char *posBottom = en ? "Bottom (shift down 1 row)" : "下方（整体下移 1 行）";
    const char *lblCustom = en ? "Spectrum subtitle (ASCII)" : "频谱字幕（ASCII）";
    const char *customHelp = en
                                 ? "Shown at the bottom of the music spectrum. Up to 21 characters are centered; longer text scrolls."
                                 : "显示在音乐频谱底部。21 个字符以内居中，更长自动滚动。";
    const char *customPlaceholder = en ? "M5STACK" : "例如：M5STACK";
    const char *optLeft = en ? "Left → Right" : "左 → 右";
    const char *optRight = en ? "Right → Left" : "右 → 左";
    const char *btnSave = en ? "Save & Connect" : "保存并连接";
    const char *btnReset = en ? "Reset Wi-Fi" : "重新配网";
    const char *confirmReset = en
                                   ? "Clear the saved Wi-Fi and reboot into setup mode?"
                                   : "清除已保存的 Wi-Fi，重启后进入配网模式？";
    const char *ssidPlaceholder = en ? "Select or enter Wi-Fi name" : "选择或输入 Wi-Fi 名称";

    p += "<form method=\"POST\" action=\"/save\" autocomplete=\"off\">";
    p += "<label>";
    p += lblSsid;
    p += "</label>";
    p += "<input name=\"ssid\" list=\"wifi-list\" value=\"" + ssid + "\" placeholder=\"" + ssidPlaceholder +
         "\" autocapitalize=\"none\" spellcheck=\"false\" required>";
    p += "<datalist id=\"wifi-list\">" + scanOptions + "</datalist>";
    p += "<label>";
    p += lblPass;
    p += "</label>";
    p += "<input type=\"password\" name=\"pass\" value=\"\" placeholder=\"" + passPlaceholder +
         "\" autocomplete=\"new-password\">";

    p += "<div class=\"row\">";
    p += "<div><label>";
    p += lblOrder;
    p += "</label><select name=\"order\">";
    p += "<option value=\"0\"" + String(settings_->monoOrder == 0 ? " selected" : "") + ">";
    p += optLeft;
    p += "</option><option value=\"1\"" + String(settings_->monoOrder == 1 ? " selected" : "") + ">";
    p += optRight;
    p += "</option></select></div>";
    p += "<div><label>";
    p += lblChain;
    p += " <b id=\"cv\">" + String(settings_->chainBrightness) + "</b></label>";
    p += "<input id=\"chain\" type=\"range\" name=\"chain\" min=\"0\" max=\"7\" value=\"" +
         String(settings_->chainBrightness) + "\" oninput=\"document.getElementById('cv').textContent=this.value\">";
    p += "</div></div>";

    p += "<label>";
    p += lblLcd;
    p += " <b id=\"lv\">" + String(settings_->lcdBrightness) + "</b></label>";
    p += "<input type=\"range\" name=\"lcd\" min=\"30\" max=\"255\" value=\"" +
         String(settings_->lcdBrightness) +
         "\" oninput=\"document.getElementById('lv').textContent=this.value\">";

    p += "<div class=\"modebox\">";
    p += "<label>";
    p += lblScreen;
    p += "</label><select name=\"screenMode\">";
    p += "<option value=\"0\"" + String(settings_->screenMode == 0 ? " selected" : "") + ">";
    p += optWorld;
    p += "</option><option value=\"1\"" + String(settings_->screenMode == 1 ? " selected" : "") + ">";
    p += optSpectrum;
    p += "</option></select>";
    p += "<label>";
    p += lblSync;
    p += "</label><select name=\"syncScreen\">";
    p += "<option value=\"1\"" + String(settings_->syncScreen == 1 ? " selected" : "") + ">";
    p += syncOn;
    p += "</option><option value=\"0\"" + String(settings_->syncScreen == 0 ? " selected" : "") + ">";
    p += syncOff;
    p += "</option></select>";
    p += "<p class=\"help\">";
    p += en ? "When Chain Mono switches to spectrum, the S3R screen follows automatically."
            : "Chain Mono 短按切到频谱时，S3R 屏幕自动切到频谱。";
    p += "</p>";

    p += "<label>";
    p += lblAudio;
    p += "</label><select name=\"audioSource\">";
    p += "<option value=\"0\"" + String(settings_->audioSource == 0 ? " selected" : "") + ">";
    p += audioUsb;
    p += "</option><option value=\"1\"" + String(settings_->audioSource == 1 ? " selected" : "") + ">";
    p += audioPdm;
    p += "</option></select>";
    p += "<p class=\"help\">";
    p += en ? "USB PC uses the companion script over USB serial. PDM uses Unit Mini PDM on Grove Port A (Data G1 / Clock G2)."
            : "USB 电脑通过 USB 串口连接电脑端脚本；PDM 使用 Unit Mini PDM，接 Grove Port A（Data G1 / Clock G2）。";
    p += "</p>";
    p += "<p class=\"help\">";
    p += en ? "Short-press the AtomS3R button to switch Chain Mono: clock / subtitle / spectrum."
            : "短按 AtomS3R 按键切换 Chain Mono 显示：时钟 / 字幕 / 频谱。";
    p += "</p>";

    p += "<label>";
    p += lblRegion;
    p += "</label><select id=\"region\" name=\"region\" onchange=\"document.getElementById('customUtcWrap').style.display=this.value==='7'?'block':'none'\">" +
         regionOptions + "</select>";
    p += "<div id=\"customUtcWrap\" style=\"display:";
    p += settings_->region == REGION_CUSTOM ? "block" : "none";
    p += "\"><label>";
    p += lblCustomUtc;
    p += "</label><input type=\"number\" name=\"utcOffset\" min=\"-12\" max=\"14\" step=\"1\" value=\"" +
         String(settings_->customUtcOffset) + "\">";
    p += "<p class=\"help\">";
    p += hintCustomUtc;
    p += "</p></div>";

    p += "<label>";
    p += lblPos;
    p += "</label><select name=\"textPosition\">";
    p += "<option value=\"0\"" + String(settings_->textPosition == 0 ? " selected" : "") + ">";
    p += posTop;
    p += "</option><option value=\"1\"" + String(settings_->textPosition == 1 ? " selected" : "") + ">";
    p += posBottom;
    p += "</option></select>";

    p += "<div id=\"customTextWrap\" style=\"display:block\"><label>";
    p += lblCustom;
    p += "</label><input name=\"monoText\" maxlength=\"32\" value=\"" + monoTextValue +
         "\" placeholder=\"" + customPlaceholder + "\" autocapitalize=\"none\" spellcheck=\"false\">";
    p += "<p class=\"help\">";
    p += customHelp;
    p += "</p>";
    p += "<label>";
    p += lblSpeed;
    p += "</label><select name=\"speed\">";
    p += "<option value=\"0\"" + String(settings_->scrollSpeed == 0 ? " selected" : "") + ">";
    p += speedSlow;
    p += "</option><option value=\"1\"" + String(settings_->scrollSpeed == 1 ? " selected" : "") + ">";
    p += speedNormal;
    p += "</option><option value=\"2\"" + String(settings_->scrollSpeed == 2 ? " selected" : "") + ">";
    p += speedFast;
    p += "</option></select></div></div>";

    p += "<button type=\"submit\">";
    p += btnSave;
    p += "</button></form>";

    p += "<form method=\"POST\" action=\"/reset\" onsubmit=\"return confirm('";
    p += confirmReset;
    p += "')\"><button class=\"ghost\" type=\"submit\">";
    p += btnReset;
    p += "</button></form>";
    p += "<p class=\"foot\">AtomS3R · 3× Chain Mono<br>";
    p += en ? "The device reboots automatically after saving." : "配置保存后设备会自动重启。";
    p += "</p>";
    p += "</main></body></html>";
    return p;
}
