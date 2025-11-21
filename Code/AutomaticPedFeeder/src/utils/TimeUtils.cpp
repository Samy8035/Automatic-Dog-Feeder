#include "TimeUtils.h"

void TimeUtils::init() {
    
    configTzTime("CET-1CEST,M3.5.0/2,M10.5.0/3",
                 "pool.ntp.org",
                 "time.nist.gov",
                 "time.google.com");
    
    // Intentar sincronizar de forma no bloqueante
    struct tm timeinfo;
    for (int i = 0; i < 10; i++) {  // Máximo 5 segundos
        if (getLocalTime(&timeinfo)) {
            return;  // Sincronizado
        }
        delay(500);
    }
    // Continuar sin bloquear - la sincronización seguirá en background
}

// ✅ Agregar método para verificar si NTP está sincronizado
bool TimeUtils::isSynced() {
    struct tm timeinfo;
    if (!getLocalTimeSafe(timeinfo)) return false;
    return timeinfo.tm_year > (2020 - 1900);  // Año válido
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
