#include "StepperController.h"

StepperController::StepperController() 
    : stepper(AccelStepper::DRIVER, STEPPER_STEP_PIN, STEPPER_DIR_PIN),
      currentCompartment(0),
      state(MOTOR_IDLE),
      enabled(false),
      movementCompleteCallback(nullptr),
      errorCallback(nullptr),
      targetPosition(0),
      lastMovementTime(0) {
}

bool StepperController::begin() {
    pinMode(STEPPER_ENABLE_PIN, OUTPUT);
    digitalWrite(STEPPER_ENABLE_PIN, HIGH); // Deshabilitado por defecto
    
    stepper.setMaxSpeed(STEPPER_MAX_SPEED);
    stepper.setAcceleration(STEPPER_ACCELERATION);
    stepper.setCurrentPosition(0);
    
    enabled = false;
    state = MOTOR_IDLE;
    
    return true;
}

void StepperController::update() {
    if (!enabled || state == MOTOR_IDLE) {
        return;
    }
    
    if (state == MOTOR_MOVING) {
        if (stepper.distanceToGo() == 0) {
            handleMovementComplete();
        } else {
            stepper.run();
        }
    }
    
    // Timeout de seguridad (30 segundos)
    if (state == MOTOR_MOVING && (millis() - lastMovementTime > 30000)) {
        triggerError("Timeout en movimiento del motor");
        stopMotor();
    }
}

bool StepperController::moveToCompartment(int compartmentIndex) {
    if (compartmentIndex < 0 || compartmentIndex >= TOTAL_COMPARTMENTS) {
        triggerError("Índice de compartimento inválido: " + String(compartmentIndex));
        return false;
    }
    
    if (state == MOTOR_MOVING) {
        triggerError("Motor ya en movimiento");
        return false;
    }
    
    enableMotor(true);
    
    targetPosition = compartmentToSteps(compartmentIndex);
    stepper.moveTo(targetPosition);
    
    state = MOTOR_MOVING;
    lastMovementTime = millis();
    
    return true;
}

bool StepperController::moveToNextCompartment() {
    int nextCompartment = (currentCompartment + 1) % TOTAL_COMPARTMENTS;
    return moveToCompartment(nextCompartment);
}

bool StepperController::moveToPreviousCompartment() {
    int prevCompartment = (currentCompartment - 1 + TOTAL_COMPARTMENTS) % TOTAL_COMPARTMENTS;
    return moveToCompartment(prevCompartment);
}

void StepperController::stopMotor() {
    stepper.stop();
    stepper.setCurrentPosition(stepper.currentPosition());
    state = MOTOR_IDLE;
    enableMotor(false);
}

void StepperController::enableMotor(bool enable) {
    enabled = enable;
    digitalWrite(STEPPER_ENABLE_PIN, enable ? LOW : HIGH);
}

bool StepperController::calibrate() {
    state = MOTOR_CALIBRATING;
    stepper.setCurrentPosition(0);
    currentCompartment = 0;
    targetPosition = 0;
    state = MOTOR_IDLE;
    return true;
}

float StepperController::getProgress() const {
    if (state != MOTOR_MOVING) return 100.0;
    
    long totalSteps = abs(targetPosition - stepper.currentPosition());
    long remainingSteps = abs(stepper.distanceToGo());
    
    if (totalSteps == 0) return 100.0;
    
    return 100.0 * (1.0 - (float)remainingSteps / totalSteps);
}

long StepperController::getStepsToTarget() const {
    return stepper.distanceToGo();
}

void StepperController::setMaxSpeed(float speed) {
    stepper.setMaxSpeed(speed);
}

void StepperController::setAcceleration(float acceleration) {
    stepper.setAcceleration(acceleration);
}

long StepperController::compartmentToSteps(int compartment) {
    return compartment * STEPS_PER_COMPARTMENT;
}

void StepperController::handleMovementComplete() {
    // Calcular en qué compartimento estamos
    long currentPos = stepper.currentPosition();
    currentCompartment = (currentPos / STEPS_PER_COMPARTMENT) % TOTAL_COMPARTMENTS;
    
    state = MOTOR_IDLE;
    enableMotor(false);
    
    if (movementCompleteCallback) {
        movementCompleteCallback();
    }
}

void StepperController::triggerError(String error) {
    state = MOTOR_ERROR;
    if (errorCallback) {
        errorCallback(error);
    }
}