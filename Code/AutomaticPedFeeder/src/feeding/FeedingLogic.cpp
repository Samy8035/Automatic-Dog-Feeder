#include "FeedingLogic.h"

FeedingLogic::FeedingLogic(StepperController *stepper, SensorManager *sensors)
    : stepperController(stepper),
      sensorManager(sensors),
      currentState(FEEDING_IDLE),
      stateStartTime(0),
      soundEnabled(true),
      presenceRequired(true),
      maxWaitTimeMs(MAX_WAIT_TIME_AFTER_SOUND),
      feedingDurationMs(FEEDING_DURATION),
      targetCompartment(FEEDING_COMPARTMENT),
      feedingInProgress(false) {
}

void FeedingLogic::begin() {
    setState(FEEDING_IDLE);
}

void FeedingLogic::update() {
    if (!stepperController || !sensorManager) return;
    
    switch (currentState) {
        case FEEDING_IDLE:
            break;
            
        case FEEDING_SOUND_ALERT:
            sensorManager->playFeedingAlert();
            setState(presenceRequired ? FEEDING_WAITING_PRESENCE : FEEDING_MOVING_CAROUSEL);
            break;
            
        case FEEDING_WAITING_PRESENCE:
            if (sensorManager->isPresenceDetected()) {
                setState(FEEDING_MOVING_CAROUSEL);
            } else if (getStateElapsedTime() > maxWaitTimeMs) {
                completeFeedingError("Timeout: mascota no detectada");
            }
            break;
            
        case FEEDING_MOVING_CAROUSEL:
            if (!stepperController->isMotorMoving()) {
                if (stepperController->getCurrentCompartment() == targetCompartment) {
                    setState(FEEDING_DISPENSING);
                } else {
                    stepperController->moveToCompartment(targetCompartment);
                }
            }
            break;
            
        case FEEDING_DISPENSING:
            if (getStateElapsedTime() >= feedingDurationMs) {
                setState(FEEDING_RETURNING);
            }
            break;
            
        case FEEDING_RETURNING:
            if (!stepperController->isMotorMoving()) {
                int nextComp = (targetCompartment + 1) % TOTAL_COMPARTMENTS;
                if (stepperController->getCurrentCompartment() == nextComp) {
                    completeFeedingSuccess();
                } else {
                    stepperController->moveToCompartment(nextComp);
                }
            }
            break;
            
        case FEEDING_COMPLETE:
        case FEEDING_ERROR:
            setState(FEEDING_IDLE);
            break;
    }
}

bool FeedingLogic::startFeeding() {
    if (feedingInProgress) return false;
    
    feedingInProgress = true;
    
    if (soundEnabled) {
        setState(FEEDING_SOUND_ALERT);
    } else if (presenceRequired) {
        setState(FEEDING_WAITING_PRESENCE);
    } else {
        setState(FEEDING_MOVING_CAROUSEL);
    }
    
    return true;
}

bool FeedingLogic::startFeedingManual() {
    bool wasRequired = presenceRequired;
    presenceRequired = false;
    bool result = startFeeding();
    presenceRequired = wasRequired;
    return result;
}

void FeedingLogic::cancelFeeding() {
    if (stepperController) stepperController->stopMotor();
    completeFeedingError("Cancelado por usuario");
}

void FeedingLogic::setState(FeedingState newState) {
    if (currentState != newState) {
        currentState = newState;
        stateStartTime = millis();
        
        if (stateChangeCallback) {
            stateChangeCallback(newState);
        }
    }
}

void FeedingLogic::completeFeedingSuccess() {
    feedingInProgress = false;
    setState(FEEDING_COMPLETE);
    
    if (feedingCompleteCallback) {
        feedingCompleteCallback(true);
    }
}

void FeedingLogic::completeFeedingError(String error) {
    feedingInProgress = false;
    lastError = error;
    setState(FEEDING_ERROR);
    
    if (feedingErrorCallback) {
        feedingErrorCallback(error);
    }
    
    if (feedingCompleteCallback) {
        feedingCompleteCallback(false);
    }
}

unsigned long FeedingLogic::getStateElapsedTime() const {
    return millis() - stateStartTime;
}

String FeedingLogic::getStateString() const {
    const char* states[] = {
        "Inactivo", "Reproduciendo alerta", "Esperando presencia",
        "Moviendo carrusel", "Dispensando comida", "Regresando posición",
        "Completado", "Error"
    };
    return states[currentState];
}

float FeedingLogic::getFeedingProgress() const {
    if (!feedingInProgress) return 0.0;
    
    switch (currentState) {
        case FEEDING_SOUND_ALERT:
            return 10.0;
        case FEEDING_WAITING_PRESENCE:
            return 20.0 + (30.0 * getStateElapsedTime() / maxWaitTimeMs);
        case FEEDING_MOVING_CAROUSEL:
            return 50.0 + (20.0 * stepperController->getProgress() / 100.0);
        case FEEDING_DISPENSING:
            return 70.0 + (20.0 * getStateElapsedTime() / feedingDurationMs);
        case FEEDING_RETURNING:
            return 90.0 + (10.0 * stepperController->getProgress() / 100.0);
        case FEEDING_COMPLETE:
            return 100.0;
        default:
            return 0.0;
    }
}