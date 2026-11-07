#include "ui.h"

#include <cstdio>
#include <cstring>

#include "cn_font.h"
#include "clock_data.h"
#include "config.h"

namespace {

constexpr uint16_t C_BG      = 0x0861;  // deep blue-black
constexpr uint16_t C_PANEL   = 0x10A2;
constexpr uint16_t C_LINE    = 0x2945;
constexpr uint16_t C_WHITE   = 0xFFFF;
constexpr uint16_t C_GREY    = 0x8410;
constexpr uint16_t C_DIM     = 0x4A49;
constexpr uint16_t C_CYAN    = 0x07FF;
constexpr uint16_t C_AMBER   = 0xFD20;
constexpr uint16_t C_GREEN   = 0x07E0;
constexpr uint16_t C_RED     = 0xF800;

uint16_t decodeUtf8(const char *&p) {
    uint8_t c = static_cast<uint8_t>(*p++);
    if (c < 0x80) return c;
    if ((c & 0xE0) == 0xC0) {
        uint16_t cp = (c & 0x1F) << 6;
        cp |= static_cast<uint8_t>(*p++) & 0x3F;
        return cp;
    }
    if ((c & 0xF0) == 0xE0) {
        uint16_t cp = (c & 0x0F) << 12;
        cp |= (static_cast<uint8_t>(*p++) & 0x3F) << 6;
        cp |= static_cast<uint8_t>(*p++) & 0x3F;
        return cp;
    }
    if ((c & 0xF8) == 0xF0) {
        uint16_t cp = (c & 0x07) << 12;
        cp |= (static_cast<uint8_t>(*p++) & 0x3F) << 6;
        cp |= static_cast<uint8_t>(*p++) & 0x3F;
        cp |= static_cast<uint8_t>(*p++) & 0x3F;
        return cp;
    }
    return 0xFFFD;
}

int cnTextWidth(const char *text) {
    int n = 0;
    while (*text) {
        decodeUtf8(text);
        ++n;
    }
    return n * 16;
}

void drawCnText(M5Canvas &c, int x, int y, const char *text, uint16_t color) {
    while (*text) {
        uint16_t cp = decodeUtf8(text);
        const uint8_t *bmp = cnGlyph(cp);
        if (bmp) c.drawXBitmap(x, y, bmp, 16, 16, color);
        x += 16;
    }
}

void drawCnCentered(M5Canvas &c, int y, const char *text, uint16_t color) {
    drawCnText(c, (128 - cnTextWidth(text)) / 2, y, text, color);
}

void formatHm(char *buf, size_t len, int hour, int minute) {
    snprintf(buf, len, "%02d:%02d", hour, minute);
}

}  // namespace

bool ClockUi::begin() {
    if (!display_.begin()) {
        Serial.println("[UI] display.begin() failed");
        return false;
    }
    display_.setRotation(3);
    display_.setBrightness(cfg::DEFAULT_LCD_BRIGHTNESS);
    display_.fillScreen(C_BG);

    canvas_.setPsram(false);
    canvas_.setColorDepth(16);
    if (!canvas_.createSprite(128, 128)) {
        Serial.println("[UI] sprite allocation failed");
        return false;
    }
    canvas_.setTextWrap(false);
    ready_ = true;
    return true;
}

void ClockUi::setBrightness(uint8_t value) {
    if (ready_) display_.setBrightness(value);
}

void ClockUi::drawWifiIcon(WifiUiState state) {
    uint16_t color = C_GREY;
    if (state == WifiUiState::Connected) color = C_GREEN;
    else if (state == WifiUiState::Connecting) color = C_AMBER;
    else if (state == WifiUiState::Portal) color = C_RED;

    const int x = 106;
    const int base = 13;
    for (int i = 0; i < 4; ++i) {
        int h = 3 + i * 2;
        canvas_.fillRect(x + i * 4, base - h, 2, h, color);
    }
    if (state == WifiUiState::Portal) {
        canvas_.drawLine(x, base - 10, x + 14, base + 1, C_RED);
        canvas_.drawLine(x + 14, base - 10, x, base + 1, C_RED);
    }
}

void ClockUi::drawHeader(bool english, WifiUiState wifi) {
    canvas_.setTextSize(1);
    canvas_.setTextDatum(lgfx::top_left);

    if (english) {
        canvas_.setFont(&lgfx::fonts::Font2);
        canvas_.setTextColor(C_CYAN);
        canvas_.drawString("WORLD", 6, 1);
        canvas_.drawString("TIME", 52, 1);
        canvas_.setFont(&lgfx::fonts::Font0);
        canvas_.setTextColor(C_AMBER);
        canvas_.drawString("UTC", 87, 5);
    } else {
        drawCnText(canvas_, 6, 1, "世界时间", C_CYAN);
        canvas_.setFont(&lgfx::fonts::Font0);
        canvas_.setTextColor(C_AMBER);
        canvas_.drawString("UTC", 78, 5);
    }
    drawWifiIcon(wifi);
}

void ClockUi::render(const UiStatus &status, time_t utcNow) {
    if (!ready_) return;

    if (status.wifi == WifiUiState::Portal) {
        renderPortal(status);
    } else if (!status.timeValid) {
        renderOffline(status);
    } else {
        renderClock(status, utcNow);
    }
    canvas_.pushSprite(0, 0);
}

void ClockUi::renderPortal(const UiStatus &status) {
    if (status.english) {
        renderEnglishPortal(status);
        return;
    }
    canvas_.fillScreen(C_BG);
    canvas_.drawFastHLine(0, 0, 128, C_LINE);

    drawCnText(canvas_, 6, 5, "配网模式", C_CYAN);
    canvas_.setFont(&lgfx::fonts::Font0);
    canvas_.setTextSize(1);
    canvas_.setTextDatum(lgfx::top_left);
    canvas_.setTextColor(C_AMBER);
    canvas_.drawString("SETUP", 94, 9);

    canvas_.setTextColor(C_GREY);
    canvas_.drawString("AP", 6, 34);
    canvas_.setTextColor(C_WHITE);
    canvas_.drawString(status.apSsid.c_str(), 24, 34);

    canvas_.setFont(&lgfx::fonts::Font2);
    canvas_.setTextSize(1);
    canvas_.setTextDatum(lgfx::middle_center);
    canvas_.setTextColor(C_AMBER);
    canvas_.drawString("192.168.4.1", 64, 54);

    canvas_.setFont(&lgfx::fonts::Font0);
    canvas_.setTextDatum(lgfx::top_left);
    canvas_.setTextColor(C_WHITE);
    canvas_.drawString("1. Join this Wi-Fi", 10, 72);
    canvas_.drawString("2. Open browser", 10, 84);

    drawCnCentered(canvas_, 100, "请打开浏览器", C_CYAN);
}

void ClockUi::renderOffline(const UiStatus &status) {
    if (status.english) {
        renderEnglishOffline(status);
        return;
    }
    canvas_.fillScreen(C_BG);
    canvas_.drawFastHLine(0, 0, 128, C_LINE);

    drawCnText(canvas_, 6, 1, "世界时间", C_CYAN);
    canvas_.setFont(&lgfx::fonts::Font0);
    canvas_.setTextSize(1);
    canvas_.setTextDatum(lgfx::top_left);
    canvas_.setTextColor(C_AMBER);
    canvas_.drawString("UTC", 78, 5);
    drawWifiIcon(status.wifi);

    canvas_.setFont(&lgfx::fonts::Font2);
    canvas_.setTextSize(2);
    canvas_.setTextDatum(lgfx::middle_center);
    canvas_.setTextColor(C_DIM);
    canvas_.drawString("--:--:--", 64, 46);

    if (status.wifi == WifiUiState::Connecting) {
        drawCnCentered(canvas_, 72, "连接中", C_AMBER);
        canvas_.setFont(&lgfx::fonts::Font0);
        canvas_.setTextSize(1);
        canvas_.setTextDatum(lgfx::middle_center);
        canvas_.setTextColor(C_GREY);
        canvas_.drawString("NTP", 64, 96);
    } else if (status.wifi == WifiUiState::Connected) {
        drawCnCentered(canvas_, 72, "正在同步", C_AMBER);
        canvas_.setFont(&lgfx::fonts::Font0);
        canvas_.setTextSize(1);
        canvas_.setTextDatum(lgfx::middle_center);
        canvas_.setTextColor(C_GREY);
        canvas_.drawString("NTP", 64, 96);
    } else {
        drawCnCentered(canvas_, 72, "未连接", C_RED);
        drawCnCentered(canvas_, 94, "请检查网络", C_GREY);
        drawCnCentered(canvas_, 112, "长按按钮配网", C_CYAN);
    }
}

void ClockUi::renderClock(const UiStatus &status, time_t utcNow) {
    if (status.english) {
        renderEnglishClock(status, utcNow);
        return;
    }
    struct tm utc = {};
    utcTime(utcNow, utc);

    canvas_.fillScreen(C_BG);
    canvas_.drawFastHLine(0, 0, 128, C_LINE);

    drawCnText(canvas_, 6, 1, "世界时间", C_CYAN);

    canvas_.setFont(&lgfx::fonts::Font0);
    canvas_.setTextSize(1);
    canvas_.setTextDatum(lgfx::top_left);
    canvas_.setTextColor(C_AMBER);
    canvas_.drawString("UTC", 78, 5);
    drawWifiIcon(status.wifi);

    char date[16];
    formatDate(date, sizeof(date), utc);
    canvas_.setTextColor(C_GREY);
    canvas_.drawString(date, 6, 19);

    static const char *weekdays[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
    canvas_.setTextColor(C_GREY);
    canvas_.drawString(weekdays[utc.tm_wday % 7], 70, 19);

    char mono[8];
    snprintf(mono, sizeof(mono), "M%d", status.chainCount);
    canvas_.setTextColor(status.chainCount >= 3 ? C_GREEN : (status.chainCount == 0 ? C_RED : C_AMBER));
    canvas_.drawString(mono, 112, 19);

    char hm[8];
    char ss[4];
    snprintf(hm, sizeof(hm), "%02d:%02d", utc.tm_hour, utc.tm_min);
    snprintf(ss, sizeof(ss), "%02d", utc.tm_sec);

    canvas_.setFont(&lgfx::fonts::Font2);
    canvas_.setTextSize(2);
    canvas_.setTextDatum(lgfx::top_left);
    int fullWidth = canvas_.textWidth("00:00:00");
    int x = (128 - fullWidth) / 2;
    if (x < 0) x = 0;
    canvas_.setTextColor(C_WHITE);
    canvas_.drawString(hm, x, 30);
    int hmWidth = canvas_.textWidth(hm);
    canvas_.setTextColor(C_AMBER);
    canvas_.drawString(":", x + hmWidth, 30);
    int colonWidth = canvas_.textWidth(":");
    canvas_.drawString(ss, x + hmWidth + colonWidth, 30);

    canvas_.drawFastHLine(8, 63, 112, C_LINE);

    for (int i = 0; i < 3; ++i) {
        int idx = status.page * 3 + i;
        if (idx >= CITY_COUNT) break;

        int y = 64 + i * 20;
        if ((i & 1) == 0) canvas_.fillRect(0, y, 128, 19, C_PANEL);
        canvas_.fillRect(0, y + 2, 2, 15, idx == 0 ? C_AMBER : C_CYAN);

        const City &city = CITIES[idx];
        drawCnText(canvas_, 6, y + 2, city.cn, C_WHITE);

        CityTime ct = cityTime(city, utcNow);
        char off[8];
        if (ct.valid) {
            snprintf(off, sizeof(off), "%+d%s", ct.offset, ct.dst ? "*" : "");
        } else {
            snprintf(off, sizeof(off), "--");
        }

        canvas_.setFont(&lgfx::fonts::Font0);
        canvas_.setTextSize(1);
        canvas_.setTextDatum(lgfx::top_left);
        canvas_.setTextColor(C_GREY);
        canvas_.drawString(off, 6 + cnTextWidth(city.cn) + 4, y + 6);

        char cityHm[8];
        if (ct.valid) {
            formatHm(cityHm, sizeof(cityHm), ct.hour, ct.minute);
        } else {
            snprintf(cityHm, sizeof(cityHm), "--:--");
        }
        canvas_.setFont(&lgfx::fonts::Font2);
        canvas_.setTextDatum(lgfx::top_right);
        canvas_.setTextColor(C_WHITE);
        canvas_.drawString(cityHm, 122, y + 2);
    }
    resetTimezoneToUtc();

    for (int p = 0; p < 2; ++p) {
        canvas_.fillCircle(61 + p * 6, 126, 1, p == status.page ? C_CYAN : C_DIM);
    }
}

void ClockUi::renderEnglishPortal(const UiStatus &status) {
    canvas_.fillScreen(C_BG);
    canvas_.drawFastHLine(0, 0, 128, C_LINE);

    canvas_.setFont(&lgfx::fonts::Font2);
    canvas_.setTextSize(1);
    canvas_.setTextDatum(lgfx::top_left);
    canvas_.setTextColor(C_CYAN);
    canvas_.drawString("WIFI SETUP", 6, 3);
    drawWifiIcon(WifiUiState::Portal);

    canvas_.setFont(&lgfx::fonts::Font0);
    canvas_.setTextColor(C_GREY);
    canvas_.drawString("AP", 6, 34);
    canvas_.setTextColor(C_WHITE);
    canvas_.drawString(status.apSsid.c_str(), 24, 34);

    canvas_.setFont(&lgfx::fonts::Font2);
    canvas_.setTextDatum(lgfx::middle_center);
    canvas_.setTextColor(C_AMBER);
    canvas_.drawString("192.168.4.1", 64, 54);

    canvas_.setFont(&lgfx::fonts::Font0);
    canvas_.setTextDatum(lgfx::top_left);
    canvas_.setTextColor(C_WHITE);
    canvas_.drawString("1. Join this Wi-Fi", 10, 76);
    canvas_.drawString("2. Open browser", 10, 90);
    canvas_.drawString("3. Configure clock", 10, 104);
}

void ClockUi::renderEnglishOffline(const UiStatus &status) {
    canvas_.fillScreen(C_BG);
    canvas_.drawFastHLine(0, 0, 128, C_LINE);
    drawHeader(true, status.wifi);

    canvas_.setFont(&lgfx::fonts::Font2);
    canvas_.setTextSize(2);
    canvas_.setTextDatum(lgfx::middle_center);
    canvas_.setTextColor(C_DIM);
    canvas_.drawString("--:--:--", 64, 46);

    canvas_.setTextSize(1);
    if (status.wifi == WifiUiState::Connecting) {
        canvas_.setTextColor(C_AMBER);
        canvas_.drawString("CONNECTING", 64, 74);
        canvas_.setFont(&lgfx::fonts::Font0);
        canvas_.setTextColor(C_GREY);
        canvas_.drawString("NTP", 64, 98);
    } else if (status.wifi == WifiUiState::Connected) {
        canvas_.setTextColor(C_AMBER);
        canvas_.drawString("SYNCING NTP", 64, 74);
        canvas_.setFont(&lgfx::fonts::Font0);
        canvas_.setTextColor(C_GREY);
        canvas_.drawString("NTP", 64, 98);
    } else {
        canvas_.setTextColor(C_RED);
        canvas_.drawString("NO WIFI", 64, 74);
        canvas_.setFont(&lgfx::fonts::Font0);
        canvas_.setTextColor(C_GREY);
        canvas_.drawString("CHECK NETWORK", 64, 94);
        canvas_.setTextColor(C_CYAN);
        canvas_.drawString("HOLD BUTTON 3s", 64, 110);
    }
}

void ClockUi::renderEnglishClock(const UiStatus &status, time_t utcNow) {
    struct tm utc = {};
    utcTime(utcNow, utc);

    canvas_.fillScreen(C_BG);
    canvas_.drawFastHLine(0, 0, 128, C_LINE);
    drawHeader(true, status.wifi);

    canvas_.setFont(&lgfx::fonts::Font0);
    canvas_.setTextSize(1);
    canvas_.setTextDatum(lgfx::top_left);
    canvas_.setTextColor(C_GREY);

    char date[16];
    formatDate(date, sizeof(date), utc);
    canvas_.drawString(date, 6, 19);

    static const char *weekdays[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
    canvas_.drawString(weekdays[utc.tm_wday % 7], 70, 19);

    char mono[8];
    snprintf(mono, sizeof(mono), "M%d", status.chainCount);
    canvas_.setTextColor(status.chainCount >= 3 ? C_GREEN : (status.chainCount == 0 ? C_RED : C_AMBER));
    canvas_.drawString(mono, 112, 19);

    char hm[8];
    char ss[4];
    snprintf(hm, sizeof(hm), "%02d:%02d", utc.tm_hour, utc.tm_min);
    snprintf(ss, sizeof(ss), "%02d", utc.tm_sec);

    canvas_.setFont(&lgfx::fonts::Font2);
    canvas_.setTextSize(2);
    canvas_.setTextDatum(lgfx::top_left);
    int fullWidth = canvas_.textWidth("00:00:00");
    int x = (128 - fullWidth) / 2;
    if (x < 0) x = 0;
    canvas_.setTextColor(C_WHITE);
    canvas_.drawString(hm, x, 30);
    int hmWidth = canvas_.textWidth(hm);
    canvas_.setTextColor(C_AMBER);
    canvas_.drawString(":", x + hmWidth, 30);
    int colonWidth = canvas_.textWidth(":");
    canvas_.drawString(ss, x + hmWidth + colonWidth, 30);

    canvas_.drawFastHLine(8, 63, 112, C_LINE);

    static const char *names[CITY_COUNT] = {"BEIJING", "LONDON", "NEW YORK",
                                           "LOS ANGELES", "TOKYO", "SYDNEY"};
    for (int i = 0; i < 3; ++i) {
        int idx = status.page * 3 + i;
        if (idx >= CITY_COUNT) break;

        int y = 64 + i * 20;
        if ((i & 1) == 0) canvas_.fillRect(0, y, 128, 19, C_PANEL);
        canvas_.fillRect(0, y + 2, 2, 15, idx == 0 ? C_AMBER : C_CYAN);

        const City &city = CITIES[idx];
        CityTime ct = cityTime(city, utcNow);

        canvas_.setFont(&lgfx::fonts::Font0);
        canvas_.setTextSize(1);
        canvas_.setTextDatum(lgfx::top_left);
        canvas_.setTextColor(C_WHITE);
        canvas_.drawString(names[idx], 6, y + 3);

        char off[8];
        if (ct.valid) {
            snprintf(off, sizeof(off), "%+d%s", ct.offset, ct.dst ? "*" : "");
        } else {
            snprintf(off, sizeof(off), "--");
        }
        canvas_.setTextColor(C_GREY);
        canvas_.drawString(off, 6, y + 11);

        char cityHm[8];
        if (ct.valid) {
            formatHm(cityHm, sizeof(cityHm), ct.hour, ct.minute);
        } else {
            snprintf(cityHm, sizeof(cityHm), "--:--");
        }
        canvas_.setFont(&lgfx::fonts::Font2);
        canvas_.setTextDatum(lgfx::top_right);
        canvas_.setTextColor(C_WHITE);
        canvas_.drawString(cityHm, 122, y + 2);
    }
    resetTimezoneToUtc();

    for (int p = 0; p < 2; ++p) {
        canvas_.fillCircle(61 + p * 6, 126, 1, p == status.page ? C_CYAN : C_DIM);
    }
}
