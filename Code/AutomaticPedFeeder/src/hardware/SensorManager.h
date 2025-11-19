#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <DHT.h>
#include "../config.h"

struct EnvironmentData {
    float temperature;
    float humidity;
    unsigned long lastUpdate;
    bool valid;
};

struct PresenceData {
    bool isDetected;
    unsigned long lastDetectionTime;
    unsigned long detectionDuration;
};

enum AlertType {
    ALERT_NONE,
    ALERT_TEMP_LOW,
    ALERT_TEMP_HIGH,
    ALERT_HUMIDITY_HIGH
};

class SensorManager {
private:
    DHT dht;
    
    // Estado de sensores
    EnvironmentData environmentData;
    PresenceData presenceData;
    
    // Configuración
    bool alertsEnabled;
    float tempMinAlert;
    float tempMaxAlert;
    float humidityMaxAlert;
    
    // Control de lecturas
    unsigned long lastDHTRead;
    unsigned long lastPIRCheck;
    const unsigned long DHT_READ_INTERVAL = 2000;
    const unsigned long PIR_CHECK_INTERVAL = 100;
    
    // Callbacks
    void (*presenceCallback)();
    void (*environmentAlertCallback)(String);
    
    // Estado interno
    bool lastPIRState;
    unsigned long presenceStartTime;
    AlertType lastAlert;
    
public:
    SensorManager();
    
    // Inicialización
    bool begin();
    void update();
    
    // Lecturas
    EnvironmentData getEnvironmentData();
    PresenceData getPresenceData();
    bool isPresenceDetected();
    bool waitForPresence(unsigned long timeoutMs);
    
    // Configuración de alertas
    void enableAlerts(bool enable) { alertsEnabled = enable; }
    void setTempAlerts(float min, float max) { tempMinAlert = min; tempMaxAlert = max; }
    void setHumidityAlert(float max) { humidityMaxAlert = max; }
    
    // Estado
    String getEnvironmentStatus() const;
    bool isEnvironmentOk() const;
    
    // Callbacks
    void setPresenceCallback(void (*callback)()) { presenceCallback = callback; }
    void setEnvironmentAlertCallback(void (*callback)(String)) { 
        environmentAlertCallback = callback; 
    }
    
    // Buzzer/Speaker
    void playSound(int frequency, int duration, int repetitions = 1);
    void playFeedingAlert();
    
private:
    void updateDHT();
    void updatePIR();
    void checkEnvironmentAlerts();
    AlertType evaluateEnvironment() const;
    String getAlertMessage(AlertType alert);
};

#endif // SENSOR_MANAGER_H