#ifndef FEEDING_LOGIC_H
#define FEEDING_LOGIC_H

#include <Arduino.h>
#include "../config.h"
#include "../hardware/StepperController.h"
#include "../hardware/SensorManager.h"

enum FeedingState {
    FEEDING_IDLE,
    FEEDING_SOUND_ALERT,
    FEEDING_WAITING_PRESENCE,
    FEEDING_MOVING_CAROUSEL,
    FEEDING_DISPENSING,
    FEEDING_RETURNING,
    FEEDING_COMPLETE,
    FEEDING_ERROR
};

class FeedingLogic {
private:
    StepperController* stepperController;
    SensorManager* sensorManager;
    
    // Estado
    FeedingState currentState;
    FeedingState previousState;
    unsigned long stateStartTime;
    
    // Configuración
    bool soundEnabled;
    bool presenceRequired;
    int maxWaitTimeMs;
    int feedingDurationMs;
    
    // Callbacks
    void (*feedingCompleteCallback)(bool);
    void (*feedingErrorCallback)(String);
    void (*stateChangeCallback)(FeedingState);
    
    // Datos de alimentación actual
    int targetCompartment;
    bool feedingInProgress;
    String lastError;
    
public:
    FeedingLogic(StepperController* stepper, SensorManager* sensors);
    
    // Inicialización
    void begin();
    void update();
    
    // Control de alimentación
    bool startFeeding();
    bool startFeedingManual();
    void cancelFeeding();
    
    // Estado
    FeedingState getState() const { return currentState; }
    String getStateString() const;
    bool isFeedingInProgress() const { return feedingInProgress; }
    float getFeedingProgress() const;
    String getLastError() const { return lastError; }
    
    // Configuración
    void enableSound(bool enable) { soundEnabled = enable; }
    void requirePresence(bool require) { presenceRequired = require; }
    void setMaxWaitTime(int timeMs) { maxWaitTimeMs = timeMs; }
    void setFeedingDuration(int timeMs) { feedingDurationMs = timeMs; }
    
    // Callbacks
    void setFeedingCompleteCallback(void (*callback)(bool)) { 
        feedingCompleteCallback = callback; 
    }
    void setFeedingErrorCallback(void (*callback)(String)) { 
        feedingErrorCallback = callback; 
    }
    void setStateChangeCallback(void (*callback)(FeedingState)) { 
        stateChangeCallback = callback; 
    }
    
private:
    void setState(FeedingState newState);
    void handleIdleState();
    void handleSoundAlertState();
    void handleWaitingPresenceState();
    void handleMovingCarouselState();
    void handleDispensingState();
    void handleReturningState();
    void handleCompleteState();
    void handleErrorState();
    
    void completeFeedingSuccess();
    void completeFeedingError(String error);
    unsigned long getStateElapsedTime() const;
};

#endif // FEEDING_LOGIC_H