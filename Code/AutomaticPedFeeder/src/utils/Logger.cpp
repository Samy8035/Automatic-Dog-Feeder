#include "Logger.h"

Logger::Logger() 
    : currentLevel((LogLevel)LOG_LEVEL),
      serialEnabled(ENABLE_SERIAL_LOG),
      fileEnabled(ENABLE_FILE_LOG) {
}

void Logger::begin() {
    if (serialEnabled && !Serial) {
        Serial.begin(115200);
        delay(100);
    }
    info("Logger inicializado");
}

void Logger::error(const String& message) {
    log(LOG_ERROR, message);
}

void Logger::warning(const String& message) {
    log(LOG_WARNING, message);
}

void Logger::info(const String& message) {
    log(LOG_INFO, message);
}

void Logger::debug(const String& message) {
    log(LOG_DEBUG, message);
}

void Logger::log(LogLevel level, const String& message) {
    if (level > currentLevel) return;
    
    String logMessage = "[" + getTimestamp() + "] ";
    logMessage += getLevelString(level) + " ";
    logMessage += message;
    
    if (serialEnabled) {
        Serial.println(logMessage);
    }
    
    // TODO: Implementar escritura a archivo si fileEnabled
}

String Logger::getLevelString(LogLevel level) {
    switch (level) {
        case LOG_ERROR:   return "[ERROR]  ";
        case LOG_WARNING: return "[WARNING]";
        case LOG_INFO:    return "[INFO]   ";
        case LOG_DEBUG:   return "[DEBUG]  ";
        default:          return "[UNKNOWN]";
    }
}

String Logger::getTimestamp() {
    unsigned long ms = millis();
    unsigned long seconds = ms / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    
    seconds %= 60;
    minutes %= 60;
    hours %= 24;
    
    char buffer[12];
    sprintf(buffer, "%02lu:%02lu:%02lu", hours, minutes, seconds);
    return String(buffer);
}