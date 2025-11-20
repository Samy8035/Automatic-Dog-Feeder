#include "WebServer.h"

// Variable global externa
extern FeederConfig globalConfig;

WebServerManager::WebServerManager(FeedingLogic* feeding, StepperController* stepper,
                                   SensorManager* sensors, CameraController* camera,
                                   FeedingScheduler* scheduler, ConfigManager* config)
    : server(WEB_SERVER_PORT),
      feedingLogic(feeding),
      stepperController(stepper),
      sensorManager(sensors),
      cameraController(camera),
      feedingScheduler(scheduler),
      configManager(config),
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
    
    server.on("/api/config/schedule", HTTP_POST,
        [this](AsyncWebServerRequest* request) {
            // Respuesta ya enviada en el body handler
        },
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, 
               size_t index, size_t total) {
            if (index == 0) {
                request->_tempObject = new String();
            }
            
            String* body = (String*)request->_tempObject;
            for (size_t i = 0; i < len; i++) {
                body->concat((char)data[i]);
            }
            
            if (index + len == total) {
                handleSaveSchedule(request, *body);
                delete body;
                request->_tempObject = nullptr;
            }
        }
    );
    
    server.on("/api/config/advanced", HTTP_POST,
        [this](AsyncWebServerRequest* request) {},
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len,
               size_t index, size_t total) {
            if (index == 0) {
                request->_tempObject = new String();
            }
            
            String* body = (String*)request->_tempObject;
            for (size_t i = 0; i < len; i++) {
                body->concat((char)data[i]);
            }
            
            if (index + len == total) {
                handleSaveAdvancedConfig(request, *body);
                delete body;
                request->_tempObject = nullptr;
            }
        }
    );
    
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

void WebServerManager::handleSaveSchedule(AsyncWebServerRequest* request, const String& body) {
    // ✅ Validar punteros antes de usar
    if (!feedingScheduler || !configManager) {
        sendJSONResponse(request, false, "Error interno del servidor");
        return;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);
    
    if (error) {
        sendJSONResponse(request, false, "JSON inválido");
        return;
    }
    
    if (doc.containsKey("autoEnabled")) {
        globalConfig.autoFeedingEnabled = doc["autoEnabled"];
        feedingScheduler->setEnabled(globalConfig.autoFeedingEnabled);  // ✅ Usar puntero
    }
    
    if (doc.containsKey("feedingInterval")) {
        globalConfig.feedingIntervalHours = doc["feedingInterval"];
        feedingScheduler->setFeedingInterval(globalConfig.feedingIntervalHours);  // ✅ Usar puntero
    }
    
    if (doc.containsKey("portionsPerDay")) {
        globalConfig.portionsPerDay = doc["portionsPerDay"];
        feedingScheduler->setMaxFeedingsPerDay(globalConfig.portionsPerDay);  // ✅ Usar puntero
    }
    
    configManager->saveConfig(globalConfig);  // ✅ Usar puntero
    sendJSONResponse(request, true, "Configuración guardada");
}

void WebServerManager::handleSaveAdvancedConfig(AsyncWebServerRequest* request, const String& body) {
    // ✅ Validar punteros antes de usar
    if (!feedingLogic || !configManager) {
        sendJSONResponse(request, false, "Error interno del servidor");
        return;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);
    
    if (error) {
        sendJSONResponse(request, false, "JSON inválido");
        return;
    }
    
    if (doc.containsKey("requirePresence")) {
        globalConfig.requirePresenceDetection = doc["requirePresence"];
        feedingLogic->requirePresence(globalConfig.requirePresenceDetection);
    }
    
    if (doc.containsKey("playSound")) {
        globalConfig.soundBeforeFeeding = doc["playSound"];
        feedingLogic->enableSound(globalConfig.soundBeforeFeeding);
    }
    
    if (doc.containsKey("tempAlerts")) {
        globalConfig.enableTemperatureAlerts = doc["tempAlerts"];
    }
    
    if (doc.containsKey("humidityAlerts")) {
        globalConfig.enableHumidityAlerts = doc["humidityAlerts"];
    }
    
    configManager->saveConfig(globalConfig);  // ✅ Usar puntero
    sendJSONResponse(request, true, "Configuración guardada");
}

void WebServerManager::handleResetDaily(AsyncWebServerRequest* request) {
    if (!feedingScheduler || !configManager) {  // ✅ Validar
        sendJSONResponse(request, false, "Error interno del servidor");
        return;
    }
    
    globalConfig.feedingsToday = 0;
    feedingScheduler->resetDailyCount();  // ✅ Usar puntero
    configManager->saveConfig(globalConfig);  // ✅ Guardar cambios
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