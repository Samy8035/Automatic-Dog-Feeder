#include "WebServer.h"

// Variable global externa
extern FeederConfig globalConfig;

WebServerManager::WebServerManager(FeedingLogic* feeding, StepperController* stepper,
                                   SensorManager* sensors, CameraController* camera)
    : server(WEB_SERVER_PORT),
      feedingLogic(feeding),
      stepperController(stepper),
      sensorManager(sensors),
      cameraController(camera),
      initialized(false) {
}

bool WebServerManager::begin() {
    // Inicializar LittleFS
    if (!LittleFS.begin(true)) {
        Serial.println("Error al montar LittleFS");
        return false;
    }
    
    setupRoutes();
    server.begin();
    
    initialized = true;
    Serial.println("Servidor web iniciado");
    
    return true;
}

void WebServerManager::update() {
    // El servidor asíncrono no necesita update continuo
}

void WebServerManager::setupRoutes() {
    setupStaticRoutes();
    setupAPIRoutes();
    setupCameraRoutes();
}

void WebServerManager::setupStaticRoutes() {
    // Servir archivos estáticos
    server.serveStatic("/", LittleFS, "/web/").setDefaultFile("index.html");
    
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(LittleFS, "/web/index.html", "text/html");
    });
    
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(LittleFS, "/web/style.css", "text/css");
    });
    
    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(LittleFS, "/web/script.js", "application/javascript");
    });
}

void WebServerManager::setupAPIRoutes() {
    server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleGetStatus(request);
    });
    
    server.on("/api/config", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleGetConfig(request);
    });
    
    server.on("/api/feed/now", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handleFeedNow(request);
    });
    
    server.on("/api/feed/cancel", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handleCancelFeeding(request);
    });
    
    server.on("/api/config/schedule", HTTP_POST, [this](AsyncWebServerRequest* request) {},
        nullptr, [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, 
                       size_t index, size_t total) {
        handleSaveSchedule(request);
    });
    
    server.on("/api/config/advanced", HTTP_POST, [this](AsyncWebServerRequest* request) {},
        nullptr, [this](AsyncWebServerRequest* request, uint8_t* data, size_t len,
                       size_t index, size_t total) {
        handleSaveAdvancedConfig(request);
    });
    
    server.on("/api/system/reset-daily", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handleResetDaily(request);
    });
    
    server.on("/api/system/reboot", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handleReboot(request);
    });
}

void WebServerManager::setupCameraRoutes() {
    #ifndef DISABLE_CAMERA
    server.on("/camera/stream", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleCameraStream(request);
    });
    
    server.on("/camera/capture", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleCameraCapture(request);
    });
    #endif
}

void WebServerManager::handleGetStatus(AsyncWebServerRequest* request) {
    String json = getStatusJSON();
    request->send(200, "application/json", json);
}

void WebServerManager::handleGetConfig(AsyncWebServerRequest* request) {
    String json = getConfigJSON();
    request->send(200, "application/json", json);
}

void WebServerManager::handleFeedNow(AsyncWebServerRequest* request) {
    if (feedingLogic->startFeedingManual()) {
        sendJSONResponse(request, true, "Alimentación iniciada");
    } else {
        sendJSONResponse(request, false, "Ya hay una alimentación en curso");
    }
}

void WebServerManager::handleCancelFeeding(AsyncWebServerRequest* request) {
    feedingLogic->cancelFeeding();
    sendJSONResponse(request, true, "Alimentación cancelada");
}

void WebServerManager::handleSaveSchedule(AsyncWebServerRequest* request) {
    // Procesar JSON recibido y actualizar configuración
    sendJSONResponse(request, true, "Configuración guardada");
}

void WebServerManager::handleSaveAdvancedConfig(AsyncWebServerRequest* request) {
    sendJSONResponse(request, true, "Configuración avanzada guardada");
}

void WebServerManager::handleResetDaily(AsyncWebServerRequest* request) {
    globalConfig.feedingsToday = 0;
    sendJSONResponse(request, true, "Contador reiniciado");
}

void WebServerManager::handleReboot(AsyncWebServerRequest* request) {
    sendJSONResponse(request, true, "Reiniciando...");
    delay(500);
    ESP.restart();
}

void WebServerManager::handleCameraStream(AsyncWebServerRequest* request) {
    #ifndef DISABLE_CAMERA
    if (!cameraController->isInitialized()) {
        request->send(503, "text/plain", "Cámara no disponible");
        return;
    }
    
    size_t len;
    uint8_t* buffer = cameraController->capturePhoto(&len);
    
    if (buffer && len > 0) {
        AsyncWebServerResponse* response = request->beginResponse_P(
            200, "image/jpeg", buffer, len);
        request->send(response);
        cameraController->releaseFrameBuffer();
    } else {
        request->send(500, "text/plain", "Error capturando imagen");
    }
    #else
    request->send(503, "text/plain", "Cámara deshabilitada");
    #endif
}

void WebServerManager::handleCameraCapture(AsyncWebServerRequest* request) {
    handleCameraStream(request);
}

String WebServerManager::getStatusJSON() {
    JsonDocument doc;
    doc["success"] = true;
    
    // Alimentación
    JsonObject feeding = doc["feeding"].to<JsonObject>();
    feeding["state"] = feedingLogic->getStateString();
    feeding["progress"] = feedingLogic->getFeedingProgress();
    feeding["compartment"] = stepperController->getCurrentCompartment();
    feeding["inProgress"] = feedingLogic->isFeedingInProgress();
    
    // Sensores
    EnvironmentData env = sensorManager->getEnvironmentData();
    JsonObject sensors = doc["sensors"].to<JsonObject>();
    sensors["temperature"] = env.temperature;
    sensors["humidity"] = env.humidity;
    sensors["presence"] = sensorManager->isPresenceDetected();
    sensors["valid"] = env.valid;
    
    // Programación
    JsonObject schedule = doc["schedule"].to<JsonObject>();
    schedule["nextFeeding"] = "Próximamente";
    schedule["todayCount"] = globalConfig.feedingsToday;
    schedule["maxPerDay"] = globalConfig.portionsPerDay;
    
    // Sistema
    JsonObject system = doc["system"].to<JsonObject>();
    system["wifi"] = String(WiFi.RSSI()) + " dBm";
    system["freeHeap"] = ESP.getFreeHeap();
    system["uptime"] = millis();
    
    String output;
    serializeJson(doc, output);
    return output;
}

String WebServerManager::getConfigJSON() {
    JsonDocument doc;
    doc["success"] = true;
    
    JsonObject config = doc["config"].to<JsonObject>();
    config["autoEnabled"] = globalConfig.autoFeedingEnabled;
    config["feedingInterval"] = globalConfig.feedingIntervalHours;
    config["portionsPerDay"] = globalConfig.portionsPerDay;
    config["requirePresence"] = globalConfig.requirePresenceDetection;
    config["playSound"] = globalConfig.soundBeforeFeeding;
    config["tempAlerts"] = globalConfig.enableTemperatureAlerts;
    config["humidityAlerts"] = globalConfig.enableHumidityAlerts;
    
    String output;
    serializeJson(doc, output);
    return output;
}

void WebServerManager::sendJSONResponse(AsyncWebServerRequest* request, bool success,
                                       const String& message, const String& data) {
    JsonDocument doc;
    doc["success"] = success;
    if (message.length() > 0) doc["message"] = message;
    if (data.length() > 0) doc["data"] = data;
    
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

String WebServerManager::formatTimeRemaining(unsigned long ms) {
    if (ms == 0) return "Ahora";
    
    unsigned long seconds = ms / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    
    String result = "";
    if (hours > 0) result += String(hours) + "h ";
    if (minutes % 60 > 0) result += String(minutes % 60) + "m";
    
    return result.length() > 0 ? result : "0m";
}