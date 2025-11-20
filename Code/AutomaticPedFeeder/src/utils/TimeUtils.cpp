#include "TimeUtils.h"

void TimeUtils::init() {
    setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
    tzset();
    
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    
    // Esperar hasta 5 segundos por sincronización
    struct tm timeinfo;
    for (int i = 0; i < 10; i++) {
        if (getLocalTime(&timeinfo) && timeinfo.tm_year > (2020 - 1900)) {
            return;  // Sincronizado exitosamente
        }
        delay(500);
    }
}

bool TimeUtils::isSynced() {
    struct tm timeinfo;
    return getLocalTime(&timeinfo) && timeinfo.tm_year > (2020 - 1900);
}

bool TimeUtils::getLocalTimeSafe(struct tm &timeinfo) {
    return getLocalTime(&timeinfo);
}

int TimeUtils::getCurrentDay() {
    struct tm t;
    return getLocalTimeSafe(t) ? t.tm_mday : -1;
}

int TimeUtils::getCurrentHour() {
    struct tm t;
    return getLocalTimeSafe(t) ? t.tm_hour : -1;
}

String TimeUtils::getTimeString() {
    struct tm t;
    if (getLocalTimeSafe(t)) {
        char buf[16];
        strftime(buf, sizeof(buf), "%H:%M:%S", &t);
        return String(buf);
    }
    return "00:00:00";
}

String TimeUtils::timeAgo(unsigned long past) {
    unsigned long now = getUnixTime();
    if (now == 0 || past == 0) return "?";

    unsigned long diff = now - past;

    if (diff < 60) return String(diff) + "s";
    if (diff < 3600) return String(diff / 60) + "m";
    if (diff < 86400) return String(diff / 3600) + "h";

    return String(diff / 86400) + "d";
}