#ifndef CONFIG_H
#define CONFIG_H

#include <vector>
#include <Arduino.h>
#include "credentials.h"

// ========== CONFIGURACIÓN DE HARDWARE ==========

// Pines del Motor Stepper
#define STEPPER_STEP_PIN 2
#define STEPPER_DIR_PIN 4
#define STEPPER_ENABLE_PIN 15

// Sensor DHT22 (Temperatura y Humedad)
#define DHT_PIN 5
#define DHT_TYPE DHT22

// Sensor PIR (Movimiento Infrarrojo)
#define PIR_PIN 18

// Speaker/Buzzer
#define BUZZER_PIN 19

// ========== CONFIGURACIÓN DE CÁMARA ESP32S3_EYE ==========
#define CAMERA_MODEL_ESP32S3_EYE

#define PWDN_GPIO_NUM    -1
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM    15
#define SIOD_GPIO_NUM    4
#define SIOC_GPIO_NUM    5
#define Y2_GPIO_NUM      11
#define Y3_GPIO_NUM      9
#define Y4_GPIO_NUM      8
#define Y5_GPIO_NUM      10
#define Y6_GPIO_NUM      12
#define Y7_GPIO_NUM      18
#define Y8_GPIO_NUM      17
#define Y9_GPIO_NUM      16
#define VSYNC_GPIO_NUM   6
#define HREF_GPIO_NUM    7
#define PCLK_GPIO_NUM    13

// ========== CONFIGURACIÓN DEL CARRUSEL ==========

#define TOTAL_COMPARTMENTS 5
#define FEEDING_COMPARTMENT 4  // El compartimento con agujero (índice 0-4)
#define STEPS_PER_REVOLUTION 200
#define MICROSTEPS 16
#define STEPS_PER_COMPARTMENT ((STEPS_PER_REVOLUTION * MICROSTEPS) / TOTAL_COMPARTMENTS)

// Velocidad y aceleración del motor
#define STEPPER_MAX_SPEED 1000
#define STEPPER_ACCELERATION 500

// ========== CONFIGURACIÓN DE ALIMENTACIÓN ==========

#define DEFAULT_FEEDING_INTERVAL_HOURS 4
#define DEFAULT_PORTIONS_PER_DAY 4
#define MAX_WAIT_TIME_AFTER_SOUND 300000  // 5 minutos en ms
#define FEEDING_DURATION 5000  // Tiempo que el compartimento permanece abierto

// ========== CONFIGURACIÓN DE SENSORES ==========

#define TEMP_MIN_ALERT 5.0   // °C
#define TEMP_MAX_ALERT 35.0  // °C
#define HUMIDITY_MAX_ALERT 70.0  // %

#define PIR_DETECTION_TIMEOUT 2000  // ms para confirmar presencia

// ========== CONFIGURACIÓN DE SONIDO ==========

#define SOUND_FREQUENCY 2000  // Hz
#define SOUND_DURATION 500    // ms
#define SOUND_REPETITIONS 3
#define SOUND_PAUSE 300       // ms entre repeticiones

// ========== CONFIGURACIÓN DE RED ==========

#define WEB_SERVER_PORT 80

// ========== CONFIGURACIÓN DE ALMACENAMIENTO ==========

#define PREFS_NAMESPACE "feeder"
#define CONFIG_FILE "/config.json"

// ========== CONFIGURACIÓN DE LOGS ==========

#define ENABLE_SERIAL_LOG true
#define ENABLE_FILE_LOG false
#define LOG_LEVEL 2  // 0=ERROR, 1=WARNING, 2=INFO, 3=DEBUG

// ========== ESTRUCTURA DE CONFIGURACIÓN RUNTIME ==========

struct FeederConfig {
    // Horarios de alimentación
    int feedingIntervalHours;
    int portionsPerDay;
    bool autoFeedingEnabled;
    
    // Comportamiento
    bool requirePresenceDetection;
    bool soundBeforeFeeding;
    int maxWaitTimeMs;
    
    // Alertas
    bool enableTemperatureAlerts;
    bool enableHumidityAlerts;
    float tempMinAlert;
    float tempMaxAlert;
    float humidityMaxAlert;
    
    // Telegram
    bool telegramEnabled;
    String botToken;
    std::vector<long long> allowedUserIds;
    
    // Cámara
    bool cameraEnabled;
    int cameraQuality;  // 10-63 (menor = mejor calidad, 10=óptimo con PSRAM)
    
    // Estado del sistema
    int currentCompartment;
    int feedingsToday;
    unsigned long lastFeedingTime;
    unsigned long nextFeedingTime;
};

// ========== COMANDOS TELEGRAM ==========

#define CMD_START "/start"
#define CMD_STATUS "/estado"
#define CMD_FEED_NOW "/alimentar"
#define CMD_PHOTO "/foto"
#define CMD_CONFIG "/configurar"
#define CMD_SCHEDULE "/horario"
#define CMD_SENSORS "/sensores"
#define CMD_ENABLE_AUTO "/activar"
#define CMD_DISABLE_AUTO "/desactivar"
#define CMD_REFILL "/rellenar"
#define CMD_HELP "/ayuda"

#endif // CONFIG_H