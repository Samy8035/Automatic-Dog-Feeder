#include "FeedingLogic.h"

FeedingLogic::FeedingLogic(StepperController* stepper, SensorManager* sensors)
    : stepperController(stepper),
      sensorManager(sensors),
      currentState(FEEDING_IDLE),
      previousState(FEEDING_IDLE),
      stateStartTime(0),
      soundEnabled(true),
      presenceRequired(true),
      maxWaitTimeMs(MAX_WAIT_TIME_AFTER_SOUND),
      feedingDurationMs(FEEDING_DURATION),
      feedingCompleteCallback(nullptr),
      feedingErrorCallback(nullptr),
      stateChangeCallback(nullptr),
      targetCompartment(FEEDING_COMPARTMENT),
      feedingInProgress(false),
      lastError("") {
}

void FeedingLogic::begin() {
    setState(FEEDING_IDLE);
}

void FeedingLogic::update() {
    switch (currentState) {
        case FEEDING_IDLE:
            handleIdleState();
            break;
        case FEEDING_SOUND_ALERT:
            handleSoundAlertState();
            break;
        case FEEDING_WAITING_PRESENCE:
            handleWaitingPresenceState();
            break;
        case FEEDING_MOVING_CAROUSEL:
            handleMovingCarouselState();
            break;
        case FEEDING_DISPENSING:
            handleDispensingState();
            break;
        case FEEDING_RETURNING:
            handleReturningState();
            break;
        case FEEDING_COMPLETE:
            handleCompleteState();
            break;
        case FEEDING_ERROR:
            handleErrorState();
            break;
    }
}

bool FeedingLogic::startFeeding() {
    if (feedingInProgress) {
        return false;
    }
    
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
    presenceRequired = false;
    bool result = startFeeding();
    presenceRequired = true;
    return result;
}

void FeedingLogic::cancelFeeding() {
    stepperController->stopMotor();
    completeFeedingError("Alimentación cancelada por usuario");
}

void FeedingLogic::setState(FeedingState newState) {
    if (currentState != newState) {
        previousState = currentState;
        currentState = newState;
        stateStartTime = millis();
        
        if (stateChangeCallback) {
            stateChangeCallback(newState);
        }
    }
}

void FeedingLogic::handleIdleState() {
    // Espera inicio de alimentación
}

void FeedingLogic::handleSoundAlertState() {
    sensorManager->playFeedingAlert();
    
    if (presenceRequired) {
        setState(FEEDING_WAITING_PRESENCE);
    } else {
        setState(FEEDING_MOVING_CAROUSEL);
    }
}

void FeedingLogic::handleWaitingPresenceState() {
    sensorManager->update();
    
    if (sensorManager->isPresenceDetected()) {
        setState(FEEDING_MOVING_CAROUSEL);
        return;
    }
    
    if (getStateElapsedTime() > maxWaitTimeMs) {
        completeFeedingError("Timeout: mascota no detectada");
    }
}

void FeedingLogic::handleMovingCarouselState() {
    if (!stepperController->isMotorMoving()) {
        if (stepperController->getCurrentCompartment() == targetCompartment) {
            setState(FEEDING_DISPENSING);
        } else {
            stepperController->moveToCompartment(targetCompartment);
        }
    }
}

void FeedingLogic::handleDispensingState() {
    if (getStateElapsedTime() >= feedingDurationMs) {
        setState(FEEDING_RETURNING);
    }
}

void FeedingLogic::handleReturningState() {
    if (!stepperController->isMotorMoving()) {
        int nextCompartment = (targetCompartment + 1) % TOTAL_COMPARTMENTS;
        if (stepperController->getCurrentCompartment() == nextCompartment) {
            completeFeedingSuccess();
        } else {
            stepperController->moveToCompartment(nextCompartment);
        }
    }
}

void FeedingLogic::handleCompleteState() {
    setState(FEEDING_IDLE);
}

void FeedingLogic::handleErrorState() {
    setState(FEEDING_IDLE);
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
    switch (currentState) {
        case FEEDING_IDLE: return "Inactivo";
        case FEEDING_SOUND_ALERT: return "Reproduciendo alerta";
        case FEEDING_WAITING_PRESENCE: return "Esperando presencia";
        case FEEDING_MOVING_CAROUSEL: return "Moviendo carrusel";
        case FEEDING_DISPENSING: return "Dispensando comida";
        case FEEDING_RETURNING: return "Regresando posición";
        case FEEDING_COMPLETE: return "Completado";
        case FEEDING_ERROR: return "Error";
        default: return "Desconocido";
    }
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