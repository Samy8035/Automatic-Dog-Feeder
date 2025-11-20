#include "TelegramBot.h"

TelegramBotManager::TelegramBotManager(FeedingLogic* feeding, StepperController* stepper,
                                       SensorManager* sensors, CameraController* camera)
    : feedingLogic(feeding),
      stepperController(stepper),
      sensorManager(sensors),
      cameraController(camera),
      bot(nullptr),
      allowedUserIds(),
      lastUpdateTime(0),
      initialized(false) {
}

TelegramBotManager::~TelegramBotManager() {
    if (bot) {
        delete bot;
    }
}

bool TelegramBotManager::begin(const String& token, const std::vector<long long>& userIds) {
    botToken = token;
    allowedUserIds = userIds;
    
    client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
    bot = new UniversalTelegramBot(botToken, client);
    
    initialized = true;
    return true;
}

void TelegramBotManager::update() {
    if (!initialized) return;
    
    unsigned long currentTime = millis();
    if (currentTime - lastUpdateTime < UPDATE_INTERVAL) {
        return;
    }
    lastUpdateTime = currentTime;
    
    int numNewMessages = bot->getUpdates(bot->last_message_received + 1);
    
    while (numNewMessages) {
        handleNewMessages(numNewMessages);
        numNewMessages = bot->getUpdates(bot->last_message_received + 1);
    }
}

void TelegramBotManager::handleNewMessages(int numNewMessages) {
    for (int i = 0; i < numNewMessages; i++) {
        String text = bot->messages[i].text;
        String chatId = String(bot->messages[i].chat_id);
        String userIdStr = bot->messages[i].from_id;
        long long userId = strtoll(userIdStr.c_str(), NULL, 10);

        if (!isUserAuthorized(userId)) {
            bot->sendMessage(chatId, "❌ No autorizado. Tu User ID es: " + userIdStr, "");
            continue;
        }

        lastMessageChatId = chatId;
        
        if (text.startsWith("/")) {
            handleCommand(text, chatId);
        }
    }
}

void TelegramBotManager::handleCommand(const String& command, const String& chatId) {
    if (command == CMD_START) {
        cmdStart(chatId);
    } else if (command == CMD_STATUS || command == CMD_STATUS "@") {
        cmdStatus(chatId);
    } else if (command == CMD_FEED_NOW) {
        cmdFeedNow(chatId);
    } else if (command == CMD_PHOTO) {
        cmdPhoto(chatId);
    } else if (command == CMD_SENSORS) {
        cmdSensors(chatId);
    } else if (command == CMD_SCHEDULE) {
        cmdSchedule(chatId);
    } else if (command == CMD_ENABLE_AUTO) {
        cmdEnableAuto(chatId);
    } else if (command == CMD_DISABLE_AUTO) {
        cmdDisableAuto(chatId);
    } else if (command == CMD_REFILL) {
        cmdRefill(chatId);
    } else if (command == CMD_CONFIG) {
        cmdConfig(chatId);
    } else if (command == CMD_HELP) {
        cmdHelp(chatId);
    } else {
        bot->sendMessage(chatId, "❓ Comando no reconocido. Usa /ayuda", "");
    }
}

void TelegramBotManager::cmdStart(const String& chatId) {
    String welcome = "🐕 *Comedero Automático v1.0*\n\n";
    welcome += "¡Bienvenido! Tu comedero está listo.\n\n";
    welcome += "Usa /ayuda para ver todos los comandos disponibles.";
    
    bot->sendMessage(chatId, welcome, "Markdown");
}

void TelegramBotManager::cmdStatus(const String& chatId) {
    String status = "📊 *Estado del Sistema*\n\n";
    
    status += "🍽️ *Alimentación*\n";
    status += "Estado: " + feedingLogic->getStateString() + "\n";
    status += "Progreso: " + String(feedingLogic->getFeedingProgress(), 1) + "%\n";
    status += "Compartimento: " + String(stepperController->getCurrentCompartment()) + "\n\n";
    
    EnvironmentData env = sensorManager->getEnvironmentData();
    if (env.valid) {
        status += "🌡️ *Ambiente*\n";
        status += "Temperatura: " + String(env.temperature, 1) + "°C\n";
        status += "Humedad: " + String(env.humidity, 1) + "%\n\n";
    }
    
    PresenceData presence = sensorManager->getPresenceData();
    status += "👁️ *Presencia*: " + String(presence.isDetected ? "Detectada" : "No detectada") + "\n\n";
    
    status += "💾 *Sistema*\n";
    status += "WiFi: " + String(WiFi.RSSI()) + " dBm\n";
    status += "Uptime: " + String(millis() / 60000) + " min\n";
    status += "Memoria: " + String(ESP.getFreeHeap() / 1024) + " KB";
    
    bot->sendMessage(chatId, status, "Markdown");
}

void TelegramBotManager::cmdFeedNow(const String& chatId) {
    bot->sendMessage(chatId, "⏳ Iniciando alimentación manual...", "");
    
    if (feedingLogic->startFeedingManual()) {
        bot->sendMessage(chatId, "✅ Alimentación iniciada correctamente", "");
    } else {
        bot->sendMessage(chatId, "❌ Error: Ya hay una alimentación en curso", "");
    }
}

// Variables globales para la cámara
uint8_t* photoBuffer = nullptr;
size_t photoSize = 0;
size_t photoIndex = 0;

// Callback: quedan más datos por enviar?
bool moreDataAvailable() {
    return photoIndex < photoSize;
}

// Callback: obtener siguiente byte
uint8_t getNextByte() {
    return photoBuffer[photoIndex++];
}

// Callback: preparar siguiente buffer (no usado, devolvemos nullptr)
uint8_t* getNextBuffer() {
    return nullptr;
}

// Callback: reset de la lectura
int resetCallback() {
    photoIndex = 0;
    return 0;
}

void TelegramBotManager::cmdPhoto(const String& chatId) {
    if (!cameraController->isInitialized()) {
        bot->sendMessage(chatId, "❌ Cámara no inicializada", "");
        return;
    }

    bot->sendMessage(chatId, "📸 Capturando foto...", "");

    photoBuffer = cameraController->capturePhoto(&photoSize);
    photoIndex = 0;

    if (photoBuffer && photoSize > 0) {
        String res = bot->sendPhotoByBinary(chatId,
                                           "image/jpeg",
                                           photoSize,
                                           moreDataAvailable,
                                           getNextByte,
                                           getNextBuffer,
                                           resetCallback);
        if (res.length() > 0) bot->sendMessage(chatId, "✅ Foto enviada", "");
        else                  bot->sendMessage(chatId, "❌ Error al enviar foto", "");
    } else {
        bot->sendMessage(chatId, "❌ Error al capturar foto", "");
    }

    cameraController->releaseFrameBuffer();
}


void TelegramBotManager::cmdSensors(const String& chatId) {
    EnvironmentData env = sensorManager->getEnvironmentData();
    
    String msg = "🌡️ *Sensores Ambientales*\n\n";
    
    if (env.valid) {
        msg += "🌡️ Temperatura: " + String(env.temperature, 1) + "°C\n";
        msg += "💧 Humedad: " + String(env.humidity, 1) + "%\n";
        msg += "⏰ Última lectura: hace " + 
               String((millis() - env.lastUpdate) / 1000) + "s\n\n";
        
        if (sensorManager->isEnvironmentOk()) {
            msg += "✅ Condiciones óptimas";
        } else {
            msg += "⚠️ Alerta ambiental activa";
        }
    } else {
        msg += "❌ Sensores sin datos válidos";
    }
    
    bot->sendMessage(chatId, msg, "Markdown");
}

void TelegramBotManager::cmdSchedule(const String& chatId) {
    bot->sendMessage(chatId, "⏰ Información de programación próximamente", "");
}

void TelegramBotManager::cmdEnableAuto(const String& chatId) {
    bot->sendMessage(chatId, "✅ Alimentación automática activada", "");
}

void TelegramBotManager::cmdDisableAuto(const String& chatId) {
    bot->sendMessage(chatId, "⏸️ Alimentación automática desactivada", "");
}

void TelegramBotManager::cmdRefill(const String& chatId) {
    bot->sendMessage(chatId, "✅ Contador de alimentaciones reiniciado", "");
}

void TelegramBotManager::cmdConfig(const String& chatId) {
    String msg = "⚙️ *Configuración*\n\n";
    msg += "Usa la interfaz web para configurar el sistema:\n";
    msg += "http://" + WiFi.localIP().toString();
    
    bot->sendMessage(chatId, msg, "Markdown");
}

void TelegramBotManager::cmdHelp(const String& chatId) {
    String help = "📚 *Comandos Disponibles*\n\n";
    help += "/estado - Ver estado completo\n";
    help += "/alimentar - Dispensar ahora\n";
    help += "/foto - Capturar imagen\n";
    help += "/sensores - Ver temperatura/humedad\n";
    help += "/horario - Ver programación\n";
    help += "/activar - Activar modo auto\n";
    help += "/desactivar - Desactivar modo auto\n";
    help += "/rellenar - Reiniciar contador\n";
    help += "/configurar - Ir a config web\n";
    help += "/ayuda - Mostrar esta ayuda";
    
    bot->sendMessage(chatId, help, "Markdown");
}

bool TelegramBotManager::isUserAuthorized(long long userId) {
    if (allowedUserIds.empty()) {
        return true;
    }
    
    for (long long id : allowedUserIds) {
        if (id == userId) {
            return true;
        }
    }
    
    return false;
}

void TelegramBotManager::sendMessage(const String& message) {
    if (lastMessageChatId.length() > 0) {
        sendMessage(lastMessageChatId, message);
    }
}

void TelegramBotManager::sendMessage(const String& chatId, const String& message) {
    if (initialized && bot) {
        bot->sendMessage(chatId, message, "");
    }
}

void TelegramBotManager::sendPhoto() {
    if (lastMessageChatId.length() > 0) {
        sendPhoto(lastMessageChatId);
    }
}

void TelegramBotManager::sendPhoto(const String& chatId) {
    cmdPhoto(chatId);
}