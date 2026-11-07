#include "clock_data.h"

#include <cstdio>
#include <cstring>

// POSIX TZ strings are deliberately used instead of fixed offsets so that
// London / New York / Los Angeles / Sydney switch automatically with DST.
const City CITIES[CITY_COUNT] = {
    {"北京", "CST-8", 8},
    {"伦敦", "GMT0BST,M3.5.0/1,M10.5.0/2", 0},
    {"纽约", "EST5EDT,M3.2.0/2,M11.1.0/2", -5},
    {"洛杉矶", "PST8PDT,M3.2.0/2,M11.1.0/2", -8},
    {"东京", "JST-9", 9},
    {"悉尼", "AEST-10AEDT,M10.1.0/2,M4.1.0/3", 10},
};

CityTime cityTime(const City &city, time_t utcNow) {
    CityTime result;
    if (utcNow <= 0) return result;

    setenv("TZ", city.tz, 1);
    tzset();

    struct tm local = {};
    if (localtime_r(&utcNow, &local) == nullptr) return result;

    result.hour = local.tm_hour;
    result.minute = local.tm_min;
    result.second = local.tm_sec;
    result.dst = local.tm_isdst > 0;
    result.offset = city.stdOffset + (result.dst ? 1 : 0);
    result.valid = true;
    return result;
}

void resetTimezoneToUtc() {
    setenv("TZ", "UTC0", 1);
    tzset();
}

void utcTime(time_t utcNow, struct tm &out) {
    gmtime_r(&utcNow, &out);
}

const char *weekdayCn(int wday) {
    static const char *names[7] = {"周日", "周一", "周二", "周三", "周四", "周五", "周六"};
    if (wday < 0 || wday > 6) return "---";
    return names[wday];
}

void formatDate(char *buf, size_t len, const struct tm &t) {
    snprintf(buf, len, "%04d-%02d-%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
}
