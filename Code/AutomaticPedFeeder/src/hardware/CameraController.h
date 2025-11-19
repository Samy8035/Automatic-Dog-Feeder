#ifndef CAMERA_CONTROLLER_H
#define CAMERA_CONTROLLER_H

#include <Arduino.h>

#ifndef DISABLE_CAMERA
#include "esp_camera.h"
#endif

#include "../config.h"

enum CameraState {
    CAMERA_UNINITIALIZED,
    CAMERA_READY,
    CAMERA_CAPTURING,
    CAMERA_ERROR
};

class CameraController {
private:
    CameraState state;
    bool initialized;
    int quality;
    
    #ifndef DISABLE_CAMERA
    camera_fb_t* lastFrameBuffer;
    #endif
    
public:
    CameraController();
    ~CameraController();
    
    // Inicialización
    bool begin();
    bool begin(int jpegQuality);
    
    // Captura
    uint8_t* capturePhoto(size_t* length);
    bool captureToBuffer();
    void releaseFrameBuffer();
    
    // Streaming
    bool startStream();
    void stopStream();
    uint8_t* getStreamFrame(size_t* length);
    
    // Estado
    CameraState getState() const { return state; }
    bool isInitialized() const { return initialized; }
    
    // Configuración
    void setQuality(int jpegQuality);
    void setFrameSize(int frameSize);
    void setBrightness(int brightness);
    void setContrast(int contrast);
    
private:
    #ifndef DISABLE_CAMERA
    bool initCamera();
    camera_config_t getCameraConfig();
    #endif
};

#endif // CAMERA_CONTROLLER_H