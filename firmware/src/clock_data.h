#pragma once

#include <Arduino.h>
#include <time.h>

struct City {
    const char *cn;       // two/three Chinese characters
    const char *tz;       // POSIX TZ rule, including DST where applicable
    int8_t stdOffset;     // standard UTC offset in hours
};

struct CityTime {
    int hour = 0;
    int minute = 0;
    int second = 0;
    bool dst = false;
    int offset = 0;
    bool valid = false;
};

constexpr int CITY_COUNT = 6;
extern const City CITIES[CITY_COUNT];

// Applies the city's POSIX TZ rule and converts the UTC epoch.
CityTime cityTime(const City &city, time_t utcNow);
void resetTimezoneToUtc();

// UTC helpers.
void utcTime(time_t utcNow, struct tm &out);
const char *weekdayCn(int wday);
void formatDate(char *buf, size_t len, const struct tm &t);
