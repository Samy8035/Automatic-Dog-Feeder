#include "SensorManager.h"

SensorManager::SensorManager() 
    : dht(DHT_PIN, DHT_TYPE),
      alertsEnabled(true),
      tempMinAlert(TEMP_MIN_ALERT),
      tempMaxAlert(TEMP_MAX_ALERT),
      humidityMaxAlert(HUMIDITY_MAX_ALERT),
      lastDHTRead(0),
      lastPIRCheck(0),
      presenceCallback(nullptr),
      environmentAlertCallback(nullptr),
      lastPIRState(false),
      presenceStartTime(0),
      lastAlert(ALERT_NONE) {
    
    environmentData.temperature = 0;
    environmentData.humidity = 0;
    environmentData.lastUpdate = 0;
    environmentData.valid = false;
    
    presenceData.isDetected = false;
    presenceData.lastDetectionTime = 0;
    presenceData.detectionDuration = 0;
}

bool SensorManager::begin() {
    // Inicializar DHT22
    dht.begin();
    
    // Configurar PIR
    pinMode(PIR_PIN, INPUT);
    
    // Configurar Buzzer
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    
    // Primera lectura del DHT
    delay(2000);
    updateDHT();
    
    return true;
}

void SensorManager::update() {
    unsigned long currentTime = millis();
    
    // Actualizar DHT22
    if (currentTime - lastDHTRead >= DHT_READ_INTERVAL) {
        updateDHT();
        lastDHTRead = currentTime;
        
        if (alertsEnabled) {
            checkEnvironmentAlerts();
        }
    }
    
    // Actualizar PIR
    if (currentTime - lastPIRCheck >= PIR_CHECK_INTERVAL) {
        updatePIR();
        lastPIRCheck = currentTime;
    }
}

void SensorManager::updateDHT() {
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    
    if (!isnan(temp) && !isnan(hum)) {
        environmentData.temperature = temp;
        environmentData.humidity = hum;
        environmentData.lastUpdate = millis();
        environmentData.valid = true;
    } else {
        environmentData.valid = false;
    }
}

void SensorManager::updatePIR() {
    bool currentState = digitalRead(PIR_PIN);
    
    if (currentState && !lastPIRState) {
        // Detección iniciada
        presenceData.isDetected = true;
        presenceData.lastDetectionTime = millis();
        presenceStartTime = millis();
        
        if (presenceCallback) {
            presenceCallback();
        }
    } else if (!currentState && lastPIRState) {
        // Detección terminada
        presenceData.detectionDuration = millis() - presenceStartTime;
    }
    
    presenceData.isDetected = currentState;
    lastPIRState = currentState;
}

void SensorManager::checkEnvironmentAlerts() {
    AlertType currentAlert = evaluateEnvironment();
    
    if (currentAlert != ALERT_NONE && currentAlert != lastAlert) {
        if (environmentAlertCallback) {
            environmentAlertCallback(getAlertMessage(currentAlert));
        }
        lastAlert = currentAlert;
    } else if (currentAlert == ALERT_NONE) {
        lastAlert = ALERT_NONE;
    }
}

AlertType SensorManager::evaluateEnvironment() {
    if (!environmentData.valid) return ALERT_NONE;
    
    if (environmentData.temperature < tempMinAlert) {
        return ALERT_TEMP_LOW;
    }
    if (environmentData.temperature > tempMaxAlert) {
        return ALERT_TEMP_HIGH;
    }
    if (environmentData.humidity > humidityMaxAlert) {
        return ALERT_HUMIDITY_HIGH;
    }
    
    return ALERT_NONE;
}

String SensorManager::getAlertMessage(AlertType alert) {
    switch (alert) {
        case ALERT_TEMP_LOW:
            return "Temperatura baja: " + String(environmentData.temperature, 1) + "°C";
        case ALERT_TEMP_HIGH:
            return "Temperatura alta: " + String(environmentData.temperature, 1) + "°C";
        case ALERT_HUMIDITY_HIGH:
            return "Humedad alta: " + String(environmentData.humidity, 1) + "%";
        default:
            return "";
    }
}

EnvironmentData SensorManager::getEnvironmentData() {
    return environmentData;
}

PresenceData SensorManager::getPresenceData() {
    return presenceData;
}

bool SensorManager::isPresenceDetected() {
    return presenceData.isDetected;
}

bool SensorManager::waitForPresence(unsigned long timeoutMs) {
    unsigned long startTime = millis();
    
    while (millis() - startTime < timeoutMs) {
        update();
        if (isPresenceDetected()) {
            delay(PIR_DETECTION_TIMEOUT); // Confirmar presencia estable
            update();
            if (isPresenceDetected()) {
                return true;
            }
        }
        delay(100);
    }
    
    return false;
}

String SensorManager::getEnvironmentStatus() const {
    if (!environmentData.valid) {
        return "Sensores: Sin datos válidos";
    }
    
    String status = "Temp: " + String(environmentData.temperature, 1) + "°C | ";
    status += "Hum: " + String(environmentData.humidity, 1) + "%";
    
    if (!isEnvironmentOk()) {
        status += " [ALERTA]";
    }
    
    return status;
}

bool SensorManager::isEnvironmentOk() const {
    return evaluateEnvironment() == ALERT_NONE;
}

void SensorManager::playSound(int frequency, int duration, int repetitions) {
    for (int i = 0; i < repetitions; i++) {
        tone(BUZZER_PIN, frequency, duration);
        delay(duration);
        
        if (i < repetitions - 1) {
            delay(SOUND_PAUSE);
        }
    }
    noTone(BUZZER_PIN);
}

void SensorManager::playFeedingAlert() {
    playSound(SOUND_FREQUENCY, SOUND_DURATION, SOUND_REPETITIONS);
}