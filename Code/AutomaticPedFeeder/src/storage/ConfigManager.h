#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "../config.h"

class ConfigManager {
private:
    Preferences preferences;
    
public:
    ConfigManager();
    
    // Gestión de configuración
    bool begin();
    FeederConfig loadConfig();
    bool saveConfig(const FeederConfig& config);
    bool resetToDefaults();
    
    // Operaciones individuales
    bool saveInt(const char* key, int value);
    bool saveBool(const char* key, bool value);
    bool saveFloat(const char* key, float value);
    bool saveString(const char* key, const String& value);
    bool saveLong(const char* key, long value);
    bool saveULong(const char* key, unsigned long value);
    
    int getInt(const char* key, int defaultValue = 0);
    bool getBool(const char* key, bool defaultValue = false);
    float getFloat(const char* key, float defaultValue = 0.0);
    String getString(const char* key, const String& defaultValue = "");
    long getLong(const char* key, long defaultValue = 0);
    unsigned long getULong(const char* key, unsigned long defaultValue = 0);
    
    // Utilidades
    void printConfig(const FeederConfig& config);
    String configToJson(const FeederConfig& config);
    bool jsonToConfig(const String& json, FeederConfig& config);
    
private:
    FeederConfig getDefaultConfig();
};

#endif // CONFIG_MANAGER_H