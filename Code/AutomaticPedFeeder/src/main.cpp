#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include "config.h"
#include "hardware/StepperController.h"
#include "hardware/SensorManager.h"
#include "hardware/CameraController.h"
#include "feeding/FeedingLogic.h"
#include "feeding/FeedingScheduler.h"
#include "communication/WebServer.h"
#include "communication/TelegramBot.h"
#include "storage/ConfigManager.h"
#include "utils/Logger.h"

// ========== OBJETOS GLOBALES ==========
StepperController stepperController;
SensorManager sensorManager;
CameraController cameraController;
FeedingLogic feedingLogic(&stepperController, &sensorManager);
FeedingScheduler feedingScheduler(&feedingLogic);
WebServerManager webServer(&feedingLogic, &stepperController, &sensorManager, &cameraController);
TelegramBotManager telegramBot(&feedingLogic, &stepperController, &sensorManager, &cameraController);
ConfigManager configManager;
Logger logger;

// Configuración global
FeederConfig globalConfig;

// ========== CALLBACKS ==========

void onFeedingComplete(bool success) {
    logger.info("Alimentación completada: " + String(success ? "Éxito" : "Fallo"));
    
    if (success) {
        globalConfig.feedingsToday++;
        globalConfig.lastFeedingTime = millis();
        configManager.saveConfig(globalConfig);
        
        // Notificar por Telegram
        if (globalConfig.telegramEnabled) {
            telegramBot.sendMessage("✅ Alimentación completada exitosamente");
            
            if (globalConfig.cameraEnabled) {
                telegramBot.sendPhoto();
            }
        }
    }
}

void onFeedingError(String error) {
    logger.error("Error en alimentación: " + error);
    
    if (globalConfig.telegramEnabled) {
        telegramBot.sendMessage("❌ Error: " + error);
    }
}

void onEnvironmentAlert(String alert) {
    logger.warning("Alerta ambiental: " + alert);
    
    if (globalConfig.telegramEnabled) {
        telegramBot.sendMessage("⚠️ " + alert);
    }
}

void onPresenceDetected() {
    logger.info("Presencia detectada");
}

void onStepperMovementComplete() {
    logger.debug("Movimiento del motor completado");
}

void onFeedingStateChange(FeedingState newState) {
    logger.info("Estado de alimentación: " + feedingLogic.getStateString());
}

// ========== SETUP ==========

void setup() {
    Serial.begin(115200);
    logger.begin();
    logger.info("=== Iniciando Comedero Automático ===");
    
    // Cargar configuración
    logger.info("Cargando configuración...");
    globalConfig = configManager.loadConfig();
    
    // Inicializar WiFi
    logger.info("Conectando a WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        logger.info("WiFi conectado - IP: " + WiFi.localIP().toString());
    } else {
        logger.error("No se pudo conectar a WiFi");
    }
    
    // Inicializar hardware
    logger.info("Inicializando hardware...");
    
    if (stepperController.begin()) {
        logger.info("✓ Motor stepper inicializado");
        stepperController.setMovementCompleteCallback(onStepperMovementComplete);
    } else {
        logger.error("✗ Error al inicializar motor stepper");
    }
    
    if (sensorManager.begin()) {
        logger.info("✓ Sensores inicializados");
        sensorManager.setPresenceCallback(onPresenceDetected);
        sensorManager.setEnvironmentAlertCallback(onEnvironmentAlert);
    } else {
        logger.error("✗ Error al inicializar sensores");
    }
    
    if (globalConfig.cameraEnabled && cameraController.begin()) {
        logger.info("✓ Cámara inicializada");
    } else {
        logger.warning("Cámara no disponible");
    }
    
    // Inicializar lógica de alimentación
    logger.info("Configurando sistema de alimentación...");
    feedingLogic.begin();
    feedingLogic.enableSound(globalConfig.soundBeforeFeeding);
    feedingLogic.requirePresence(globalConfig.requirePresenceDetection);
    feedingLogic.setMaxWaitTime(globalConfig.maxWaitTimeMs);
    feedingLogic.setFeedingCompleteCallback(onFeedingComplete);
    feedingLogic.setFeedingErrorCallback(onFeedingError);
    feedingLogic.setStateChangeCallback(onFeedingStateChange);
    
    // Inicializar programador
    feedingScheduler.begin();
    feedingScheduler.setFeedingInterval(globalConfig.feedingIntervalHours);
    feedingScheduler.setEnabled(globalConfig.autoFeedingEnabled);
    
    // Inicializar servidor web
    if (webServer.begin()) {
        logger.info("✓ Servidor web iniciado en puerto " + String(WEB_SERVER_PORT));
    } else {
        logger.error("✗ Error al iniciar servidor web");
    }
    
    // Inicializar bot de Telegram
    if (globalConfig.telegramEnabled) {
        telegramBot.begin(globalConfig.botToken, globalConfig.allowedChatId);
        logger.info("✓ Bot de Telegram inicializado");
        telegramBot.sendMessage("🐕 Comedero automático iniciado y listo");
    }
    
    // Calibración inicial (opcional)
    if (globalConfig.currentCompartment == -1) {
        logger.info("Realizando calibración inicial...");
        stepperController.calibrate();
        globalConfig.currentCompartment = 0;
        configManager.saveConfig(globalConfig);
    }
    
    logger.info("=== Sistema listo ===");
    logger.info("Estado: " + feedingScheduler.getScheduleStatus());
    logger.info(sensorManager.getEnvironmentStatus());
}

// ========== LOOP ==========

void loop() {
    // Actualizar todos los módulos
    stepperController.update();
    sensorManager.update();
    feedingLogic.update();
    feedingScheduler.update();
    webServer.update();
    
    if (globalConfig.telegramEnabled) {
        telegramBot.update();
    }
    
    // Actualizar configuración global periódicamente
    static unsigned long lastConfigSave = 0;
    if (millis() - lastConfigSave > 60000) {  // Cada minuto
        globalConfig.currentCompartment = stepperController.getCurrentCompartment();
        configManager.saveConfig(globalConfig);
        lastConfigSave = millis();
    }
    
    // Reset diario del contador de alimentaciones
    static int lastDay = 0;
    int currentDay = day();
    if (currentDay != lastDay) {
        globalConfig.feedingsToday = 0;
        lastDay = currentDay;
        logger.info("Nuevo día - contador de alimentaciones reiniciado");
    }
    
    yield();  // Dar tiempo a otras tareas
}

// ========== FUNCIONES AUXILIARES ==========

void resetSystem() {
    logger.warning("Reiniciando sistema...");
    ESP.restart();
}

void factoryReset() {
    logger.warning("Restaurando valores de fábrica...");
    configManager.resetToDefaults();
    delay(1000);
    ESP.restart();
}

String getSystemStatus() {
    String status = "=== Estado del Sistema ===\n";
    status += "WiFi: " + String(WiFi.status() == WL_CONNECTED ? "Conectado" : "Desconectado") + "\n";
    status += "IP: " + WiFi.localIP().toString() + "\n";
    status += "Uptime: " + String(millis() / 1000) + "s\n";
    status += "Memoria libre: " + String(ESP.getFreeHeap()) + " bytes\n";
    status += "\n";
    
    status += "=== Alimentación ===\n";
    status += "Estado: " + feedingLogic.getStateString() + "\n";
    status += "Progreso: " + String(feedingLogic.getFeedingProgress(), 1) + "%\n";
    status += "Automático: " + String(globalConfig.autoFeedingEnabled ? "Activado" : "Desactivado") + "\n";
    status += "Alimentaciones hoy: " + String(globalConfig.feedingsToday) + "/" + String(globalConfig.portionsPerDay) + "\n";
    status += feedingScheduler.getScheduleStatus() + "\n";
    status += "\n";
    
    status += "=== Hardware ===\n";
    status += "Compartimento actual: " + String(stepperController.getCurrentCompartment()) + "\n";
    status += "Motor en movimiento: " + String(stepperController.isMotorMoving() ? "Sí" : "No") + "\n";
    status += sensorManager.getEnvironmentStatus() + "\n";
    status += "Presencia detectada: " + String(sensorManager.getPresenceData().isDetected ? "Sí" : "No") + "\n";
    
    return status;
}