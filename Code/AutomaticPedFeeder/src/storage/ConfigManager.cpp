#include "ConfigManager.h"

ConfigManager::ConfigManager() {
}

bool ConfigManager::begin() {
    return preferences.begin(PREFS_NAMESPACE, false);
}

FeederConfig ConfigManager::loadConfig() {
    begin();
    
    FeederConfig config = getDefaultConfig();
    
    config.feedingIntervalHours = getInt("feedInterval", DEFAULT_FEEDING_INTERVAL_HOURS);
    config.portionsPerDay = getInt("portionsDay", DEFAULT_PORTIONS_PER_DAY);
    config.autoFeedingEnabled = getBool("autoEnabled", true);
    
    config.requirePresenceDetection = getBool("reqPresence", true);
    config.soundBeforeFeeding = getBool("soundBefore", true);
    config.maxWaitTimeMs = getInt("maxWaitMs", MAX_WAIT_TIME_AFTER_SOUND);
    
    config.enableTemperatureAlerts = getBool("tempAlerts", true);
    config.enableHumidityAlerts = getBool("humAlerts", true);
    config.tempMinAlert = getFloat("tempMin", TEMP_MIN_ALERT);
    config.tempMaxAlert = getFloat("tempMax", TEMP_MAX_ALERT);
    config.humidityMaxAlert = getFloat("humMax", HUMIDITY_MAX_ALERT);
    
    config.telegramEnabled = getBool("tgEnabled", true);
    config.botToken = getString("botToken", BOT_TOKEN);
    config.allowedUserIds = ALLOWED_USER_IDS;
    
    config.cameraEnabled = getBool("camEnabled", true);
    config.cameraQuality = getInt("camQuality", 10);
    
    config.currentCompartment = getInt("curCompart", 0);
    config.feedingsToday = getInt("feedToday", 0);
    config.lastFeedingTime = getULong("lastFeedTime", 0);
    config.nextFeedingTime = getULong("nextFeedTime", 0);
    
    preferences.end();
    
    return config;
}

bool ConfigManager::saveConfig(const FeederConfig& config) {
    begin();
    
    saveInt("feedInterval", config.feedingIntervalHours);
    saveInt("portionsDay", config.portionsPerDay);
    saveBool("autoEnabled", config.autoFeedingEnabled);
    
    saveBool("reqPresence", config.requirePresenceDetection);
    saveBool("soundBefore", config.soundBeforeFeeding);
    saveInt("maxWaitMs", config.maxWaitTimeMs);
    
    saveBool("tempAlerts", config.enableTemperatureAlerts);
    saveBool("humAlerts", config.enableHumidityAlerts);
    saveFloat("tempMin", config.tempMinAlert);
    saveFloat("tempMax", config.tempMaxAlert);
    saveFloat("humMax", config.humidityMaxAlert);
    
    saveBool("tgEnabled", config.telegramEnabled);
    saveString("botToken", config.botToken);
    
    saveBool("camEnabled", config.cameraEnabled);
    saveInt("camQuality", config.cameraQuality);
    
    saveInt("curCompart", config.currentCompartment);
    saveInt("feedToday", config.feedingsToday);
    saveULong("lastFeedTime", config.lastFeedingTime);
    saveULong("nextFeedTime", config.nextFeedingTime);
    
    preferences.end();
    
    return true;
}

bool ConfigManager::resetToDefaults() {
    begin();
    preferences.clear();
    preferences.end();
    
    FeederConfig defaultConfig = getDefaultConfig();
    return saveConfig(defaultConfig);
}

bool ConfigManager::saveInt(const char* key, int value) {
    return preferences.putInt(key, value) > 0;
}

bool ConfigManager::saveBool(const char* key, bool value) {
    return preferences.putBool(key, value);
}

bool ConfigManager::saveFloat(const char* key, float value) {
    return preferences.putFloat(key, value) > 0;
}

bool ConfigManager::saveString(const char* key, const String& value) {
    return preferences.putString(key, value) > 0;
}

bool ConfigManager::saveLong(const char* key, long value) {
    return preferences.putLong(key, value) > 0;
}

bool ConfigManager::saveULong(const char* key, unsigned long value) {
    return preferences.putULong(key, value) > 0;
}

int ConfigManager::getInt(const char* key, int defaultValue) {
    return preferences.getInt(key, defaultValue);
}

bool ConfigManager::getBool(const char* key, bool defaultValue) {
    return preferences.getBool(key, defaultValue);
}

float ConfigManager::getFloat(const char* key, float defaultValue) {
    return preferences.getFloat(key, defaultValue);
}

String ConfigManager::getString(const char* key, const String& defaultValue) {
    return preferences.getString(key, defaultValue);
}

long ConfigManager::getLong(const char* key, long defaultValue) {
    return preferences.getLong(key, defaultValue);
}

unsigned long ConfigManager::getULong(const char* key, unsigned long defaultValue) {
    return preferences.getULong(key, defaultValue);
}

FeederConfig ConfigManager::getDefaultConfig() {
    FeederConfig config;
    
    config.feedingIntervalHours = DEFAULT_FEEDING_INTERVAL_HOURS;
    config.portionsPerDay = DEFAULT_PORTIONS_PER_DAY;
    config.autoFeedingEnabled = true;
    
    config.requirePresenceDetection = true;
    config.soundBeforeFeeding = true;
    config.maxWaitTimeMs = MAX_WAIT_TIME_AFTER_SOUND;
    
    config.enableTemperatureAlerts = true;
    config.enableHumidityAlerts = true;
    config.tempMinAlert = TEMP_MIN_ALERT;
    config.tempMaxAlert = TEMP_MAX_ALERT;
    config.humidityMaxAlert = HUMIDITY_MAX_ALERT;
    
    config.telegramEnabled = true;
    config.botToken = BOT_TOKEN;
    config.allowedUserIds = ALLOWED_USER_IDS;
    
    config.cameraEnabled = true;
    config.cameraQuality = 10;
    
    config.currentCompartment = 0;
    config.feedingsToday = 0;
    config.lastFeedingTime = 0;
    config.nextFeedingTime = 0;
    
    return config;
}

void ConfigManager::printConfig(const FeederConfig& config) {
    Serial.println("=== Configuración Actual ===");
    Serial.printf("Intervalo: %dh | Porciones/día: %d\n", 
                  config.feedingIntervalHours, config.portionsPerDay);
    Serial.printf("Auto: %s | Presencia: %s | Sonido: %s\n",
                  config.autoFeedingEnabled ? "Sí" : "No",
                  config.requirePresenceDetection ? "Sí" : "No",
                  config.soundBeforeFeeding ? "Sí" : "No");
    Serial.printf("Telegram: %s | Cámara: %s\n",
                  config.telegramEnabled ? "Sí" : "No",
                  config.cameraEnabled ? "Sí" : "No");
}

String ConfigManager::configToJson(const FeederConfig& config) {
    JsonDocument doc;
    
    doc["feedingIntervalHours"] = config.feedingIntervalHours;
    doc["portionsPerDay"] = config.portionsPerDay;
    doc["autoFeedingEnabled"] = config.autoFeedingEnabled;
    doc["requirePresenceDetection"] = config.requirePresenceDetection;
    doc["soundBeforeFeeding"] = config.soundBeforeFeeding;
    doc["maxWaitTimeMs"] = config.maxWaitTimeMs;
    doc["currentCompartment"] = config.currentCompartment;
    doc["feedingsToday"] = config.feedingsToday;
    
    String output;
    serializeJson(doc, output);
    return output;
}

bool ConfigManager::jsonToConfig(const String& json, FeederConfig& config) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        return false;
    }
    
    if (doc.containsKey("feedingIntervalHours"))
        config.feedingIntervalHours = doc["feedingIntervalHours"];
    if (doc.containsKey("portionsPerDay"))
        config.portionsPerDay = doc["portionsPerDay"];
    if (doc.containsKey("autoFeedingEnabled"))
        config.autoFeedingEnabled = doc["autoFeedingEnabled"];
    if (doc.containsKey("requirePresenceDetection"))
        config.requirePresenceDetection = doc["requirePresenceDetection"];
    if (doc.containsKey("soundBeforeFeeding"))
        config.soundBeforeFeeding = doc["soundBeforeFeeding"];
    if (doc.containsKey("maxWaitTimeMs"))
        config.maxWaitTimeMs = doc["maxWaitTimeMs"];
    
    return true;
}