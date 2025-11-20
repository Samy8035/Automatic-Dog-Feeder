#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "../config.h"
#include "../feeding/FeedingLogic.h"
#include "../feeding/FeedingScheduler.h"
#include "../storage/ConfigManager.h"
#include "../hardware/StepperController.h"
#include "../hardware/SensorManager.h"
#include "../hardware/CameraController.h"
#include "WiFi.h"

class WebServerManager {
private:
    AsyncWebServer server;
    
    // Referencias a módulos
    FeedingLogic* feedingLogic;
    StepperController* stepperController;
    SensorManager* sensorManager;
    CameraController* cameraController;
    FeedingScheduler* feedingScheduler;
    ConfigManager* configManager;
    
    // Estado
    bool initialized;
    
public:
    WebServerManager(FeedingLogic* feeding, StepperController* stepper,
                     SensorManager* sensors, CameraController* camera,
                    FeedingScheduler* scheduler, ConfigManager* config);
    
    // Inicialización
    bool begin();
    void update();
    
private:
    // Configuración de rutas
    void setupRoutes();
    void setupStaticRoutes();
    void setupAPIRoutes();
    void setupCameraRoutes();
    
    // Handlers de API
    void handleGetStatus(AsyncWebServerRequest* request);
    void handleGetConfig(AsyncWebServerRequest* request);
    void handleFeedNow(AsyncWebServerRequest* request);
    void handleCancelFeeding(AsyncWebServerRequest* request);
    void handleSaveSchedule(AsyncWebServerRequest* request, const String& body);
    void handleSaveAdvancedConfig(AsyncWebServerRequest* request, const String& body);
    void handleResetDaily(AsyncWebServerRequest* request);
    void handleReboot(AsyncWebServerRequest* request);
    
    // Handlers de cámara
    void handleCameraStream(AsyncWebServerRequest* request);
    void handleCameraCapture(AsyncWebServerRequest* request);
    
    // Utilidades
    String getStatusJSON();
    String getConfigJSON();
    String formatTimeRemaining(unsigned long ms);
    void sendJSONResponse(AsyncWebServerRequest* request, bool success, 
                         const String& message = "", const String& data = "");
};

#endif // WEB_SERVER_H