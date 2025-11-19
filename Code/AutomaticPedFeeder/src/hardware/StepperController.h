#ifndef STEPPER_CONTROLLER_H
#define STEPPER_CONTROLLER_H

#include <Arduino.h>
#include <AccelStepper.h>
#include "../config.h"

enum MotorState {
    MOTOR_IDLE,
    MOTOR_MOVING,
    MOTOR_CALIBRATING,
    MOTOR_ERROR
};

class StepperController {
private:
    AccelStepper stepper;
    int currentCompartment;
    MotorState state;
    bool enabled;
    
    // Callbacks
    void (*movementCompleteCallback)();
    void (*errorCallback)(String);
    
    // Control interno
    long targetPosition;
    unsigned long lastMovementTime;
    
public:
    StepperController();
    
    // Inicialización
    bool begin();
    void update();
    
    // Control del motor
    bool moveToCompartment(int compartmentIndex);
    bool moveToNextCompartment();
    bool moveToPreviousCompartment();
    void stopMotor();
    void enableMotor(bool enable);
    bool calibrate();
    
    // Estado y consultas
    int getCurrentCompartment() const { return currentCompartment; }
    MotorState getState() const { return state; }
    bool isMotorMoving() const { return state == MOTOR_MOVING; }
    bool isEnabled() const { return enabled; }
    float getProgress() const;
    long getStepsToTarget() const;
    
    // Configuración
    void setMaxSpeed(float speed);
    void setAcceleration(float acceleration);
    void setCurrentCompartment(int compartment) { currentCompartment = compartment; }
    
    // Callbacks
    void setMovementCompleteCallback(void (*callback)()) { movementCompleteCallback = callback; }
    void setErrorCallback(void (*callback)(String)) { errorCallback = callback; }
    
private:
    long compartmentToSteps(int compartment);
    void handleMovementComplete();
    void triggerError(String error);
};

#endif // STEPPER_CONTROLLER_H