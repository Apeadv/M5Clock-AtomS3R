#pragma once

#include <Arduino.h>
#include <time.h>

struct City {
    const char *cn;       // two/three Chinese characters
    const char *en;       // English display name
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
constexpr int REGION_FIXED_COUNT = 7;
constexpr int REGION_CUSTOM = 7;
constexpr int REGION_MAX = 7;
extern const City CITIES[CITY_COUNT];
extern const City REGIONS[REGION_FIXED_COUNT];

// Applies the city's POSIX TZ rule and converts the UTC epoch.
CityTime cityTime(const City &city, time_t utcNow);

// Returns a tm containing the selected region's hour/minute/second.
// REGION_CUSTOM uses customOffsetHours (-12..+14).
struct tm regionTime(time_t utcNow, uint8_t region, int8_t customOffsetHours);
void resetTimezoneToUtc();

// UTC helpers.
void utcTime(time_t utcNow, struct tm &out);
const char *weekdayCn(int wday);
void formatDate(char *buf, size_t len, const struct tm &t);
