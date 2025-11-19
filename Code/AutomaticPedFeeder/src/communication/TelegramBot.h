#ifndef TELEGRAM_BOT_H
#define TELEGRAM_BOT_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include "../config.h"
#include "../feeding/FeedingLogic.h"
#include "../hardware/StepperController.h"
#include "../hardware/SensorManager.h"
#include "../hardware/CameraController.h"

class TelegramBotManager {
private:
    WiFiClientSecure client;
    UniversalTelegramBot* bot;
    
    // Referencias a módulos
    FeedingLogic* feedingLogic;
    StepperController* stepperController;
    SensorManager* sensorManager;
    CameraController* cameraController;
    
    // Configuración
    String botToken;
    long allowedChatId;
    unsigned long lastUpdateTime;
    const unsigned long UPDATE_INTERVAL = 1000; // 1 segundo
    
    // Estado
    bool initialized;
    String lastMessageChatId;
    
public:
    TelegramBotManager(FeedingLogic* feeding, StepperController* stepper, 
                       SensorManager* sensors, CameraController* camera);
    ~TelegramBotManager();
    
    // Inicialización
    bool begin(const String& token, long chatId = 0);
    void update();
    
    // Envío de mensajes
    void sendMessage(const String& message);
    void sendMessage(const String& chatId, const String& message);
    void sendPhoto();
    void sendPhoto(const String& chatId);
    
private:
    void handleNewMessages(int numNewMessages);
    void handleCommand(const String& command, const String& chatId);
    
    // Comandos
    void cmdStart(const String& chatId);
    void cmdStatus(const String& chatId);
    void cmdFeedNow(const String& chatId);
    void cmdPhoto(const String& chatId);
    void cmdSensors(const String& chatId);
    void cmdSchedule(const String& chatId);
    void cmdEnableAuto(const String& chatId);
    void cmdDisableAuto(const String& chatId);
    void cmdRefill(const String& chatId);
    void cmdConfig(const String& chatId);
    void cmdHelp(const String& chatId);
    
    // Utilidades
    bool isAuthorized(const String& chatId);
    String getKeyboard();
};

#endif // TELEGRAM_BOT_H