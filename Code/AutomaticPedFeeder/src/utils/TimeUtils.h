#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <Arduino.h>
#include <time.h>

class TimeUtils {
private:
    // Métodos internos seguros
    static bool getLocalTimeSafe(struct tm &timeinfo);

public:
    // Inicializar zona horaria y NTP
    static void init();

    // Fecha/Hora actual
    static int getCurrentDay();        // Día del mes
    static int getCurrentHour();       // Hora 0–23
    static int getCurrentMinute();     // Minuto 0–59
    static int getCurrentSecond();     // Segundo 0–59
    static bool isSynced();

    // Tiempos tipo UNIX
    static unsigned long getUnixTime();  // Timestamp actual

    // Formatos de tiempo legible
    static String getTimeString();       // HH:MM:SS
    static String timeAgo(unsigned long pastUnixTime); // "hace X min"
};

#endif // TIME_UTILS_H
