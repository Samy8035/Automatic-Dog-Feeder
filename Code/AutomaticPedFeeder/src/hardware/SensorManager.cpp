#include "SensorManager.h"

SensorManager::SensorManager() 
    : dht(DHT_PIN, DHT_TYPE),
      alertsEnabled(true),
      tempMinAlert(TEMP_MIN_ALERT),
      tempMaxAlert(TEMP_MAX_ALERT),
      humidityMaxAlert(HUMIDITY_MAX_ALERT),
      lastDHTRead(0),
      lastPIRCheck(0),
      lastPIRState(false),
      presenceStartTime(0),
      lastAlert(ALERT_NONE) {
    
    environmentData = {0, 0, 0, false};
    presenceData = {false, 0, 0};
}

bool SensorManager::begin() {
    dht.begin();
    pinMode(PIR_PIN, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    
    delay(2000);
    updateDHT();
    
    return true;
}

void SensorManager::update() {
    unsigned long now = millis();
    
    // DHT22 cada 2 segundos
    if (now - lastDHTRead >= DHT_READ_INTERVAL) {
        updateDHT();
        lastDHTRead = now;
        
        if (alertsEnabled) {
            checkEnvironmentAlerts();
        }
    }
    
    // PIR cada 100ms
    if (now - lastPIRCheck >= PIR_CHECK_INTERVAL) {
        updatePIR();
        lastPIRCheck = now;
    }
}

void SensorManager::updateDHT() {
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    
    environmentData.valid = (!isnan(temp) && !isnan(hum));
    
    if (environmentData.valid) {
        environmentData.temperature = temp;
        environmentData.humidity = hum;
        environmentData.lastUpdate = millis();
    }
}

void SensorManager::updatePIR() {
    bool current = digitalRead(PIR_PIN);
    
    if (current && !lastPIRState) {
        // Inicio de detección
        presenceData.isDetected = true;
        presenceData.lastDetectionTime = millis();
        presenceStartTime = millis();
        
        if (presenceCallback) presenceCallback();
        
    } else if (!current && lastPIRState) {
        // Fin de detección
        presenceData.detectionDuration = millis() - presenceStartTime;
    }
    
    presenceData.isDetected = current;
    lastPIRState = current;
}

void SensorManager::checkEnvironmentAlerts() {
    AlertType alert = evaluateEnvironment();
    
    if (alert != ALERT_NONE && alert != lastAlert) {
        if (environmentAlertCallback) {
            environmentAlertCallback(getAlertMessage(alert));
        }
    }
    
    lastAlert = alert;
}

AlertType SensorManager::evaluateEnvironment() const {
    if (!environmentData.valid) return ALERT_NONE;
    
    if (environmentData.temperature < tempMinAlert) return ALERT_TEMP_LOW;
    if (environmentData.temperature > tempMaxAlert) return ALERT_TEMP_HIGH;
    if (environmentData.humidity > humidityMaxAlert) return ALERT_HUMIDITY_HIGH;
    
    return ALERT_NONE;
}

String SensorManager::getAlertMessage(AlertType alert) {
    switch (alert) {
        case ALERT_TEMP_LOW:
            return "⚠️ Temp baja: " + String(environmentData.temperature, 1) + "°C";
        case ALERT_TEMP_HIGH:
            return "⚠️ Temp alta: " + String(environmentData.temperature, 1) + "°C";
        case ALERT_HUMIDITY_HIGH:
            return "⚠️ Humedad alta: " + String(environmentData.humidity, 1) + "%";
        default:
            return "";
    }
}

bool SensorManager::waitForPresence(unsigned long timeoutMs) {
    unsigned long start = millis();
    
    while (millis() - start < timeoutMs) {
        update();
        
        if (isPresenceDetected()) {
            delay(PIR_DETECTION_TIMEOUT);
            update();
            
            if (isPresenceDetected()) return true;
        }
        
        delay(100);
    }
    
    return false;
}

String SensorManager::getEnvironmentStatus() const {
    if (!environmentData.valid) {
        return "Sensores: Sin datos";
    }
    
    String status = "Temp: " + String(environmentData.temperature, 1) + "°C | ";
    status += "Hum: " + String(environmentData.humidity, 1) + "%";
    
    if (!isEnvironmentOk()) status += " [ALERTA]";
    
    return status;
}

void SensorManager::playSound(int freq, int duration, int reps) {
    for (int i = 0; i < reps; i++) {
        tone(BUZZER_PIN, freq, duration);
        delay(duration);
        if (i < reps - 1) delay(SOUND_PAUSE);
    }
    noTone(BUZZER_PIN);
}

void SensorManager::playFeedingAlert() {
    playSound(SOUND_FREQUENCY, SOUND_DURATION, SOUND_REPETITIONS);
}