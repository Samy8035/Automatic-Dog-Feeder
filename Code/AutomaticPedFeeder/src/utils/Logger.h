#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include "../config.h"

enum LogLevel {
    LOG_ERROR = 0,
    LOG_WARNING = 1,
    LOG_INFO = 2,
    LOG_DEBUG = 3
};

class Logger {
private:
    LogLevel currentLevel;
    bool serialEnabled;
    bool fileEnabled;
    
public:
    Logger();
    void begin();
    
    void error(const String& message);
    void warning(const String& message);
    void info(const String& message);
    void debug(const String& message);
    
    void setLevel(LogLevel level) { currentLevel = level; }
    void enableSerial(bool enable) { serialEnabled = enable; }
    void enableFile(bool enable) { fileEnabled = enable; }
    
private:
    void log(LogLevel level, const String& message);
    String getLevelString(LogLevel level);
    String getTimestamp();
};

#endif // LOGGER_H