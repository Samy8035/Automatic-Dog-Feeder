#include "TimeUtils.h"

void TimeUtils::init() {
    // Zona horaria Madrid CET/CEST
    setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
    tzset();

    // Servidores NTP
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    // Esperar un poco para sincronizar (no bloqueante total)
    struct tm timeinfo;
    int tries = 0;
    while (!getLocalTime(&timeinfo) && tries < 20) {
        delay(500);
        tries++;
    }
}

bool TimeUtils::getLocalTimeSafe(struct tm &timeinfo) {
    if (getLocalTime(&timeinfo)) {
        return true;
    }
    return false;
}

int TimeUtils::getCurrentDay() {
    struct tm timeinfo;
    if (getLocalTimeSafe(timeinfo)) return timeinfo.tm_mday;
    return -1;
}

int TimeUtils::getCurrentHour() {
    struct tm timeinfo;
    if (getLocalTimeSafe(timeinfo)) return timeinfo.tm_hour;
    return -1;
}

int TimeUtils::getCurrentMinute() {
    struct tm timeinfo;
    if (getLocalTimeSafe(timeinfo)) return timeinfo.tm_min;
    return -1;
}

int TimeUtils::getCurrentSecond() {
    struct tm timeinfo;
    if (getLocalTimeSafe(timeinfo)) return timeinfo.tm_sec;
    return -1;
}

unsigned long TimeUtils::getUnixTime() {
    struct tm timeinfo;
    if (getLocalTimeSafe(timeinfo)) {
        return mktime(&timeinfo);
    }
    return 0;
}

String TimeUtils::getTimeString() {
    struct tm timeinfo;
    if (getLocalTimeSafe(timeinfo)) {
        char buffer[16];
        strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
        return String(buffer);
    }
    return "00:00:00";
}

String TimeUtils::timeAgo(unsigned long pastUnixTime) {
    unsigned long now = getUnixTime();
    if (now == 0 || pastUnixTime == 0) return "desconocido";

    unsigned long diff = now - pastUnixTime;

    if (diff < 60) return String(diff) + "s";
    if (diff < 3600) return String(diff / 60) + "m";
    if (diff < 86400) return String(diff / 3600) + "h";

    return String(diff / 86400) + "d";
}
