#include "CameraController.h"

CameraController::CameraController()
    : state(CAMERA_UNINITIALIZED),
      initialized(false),
      quality(10) {
    #ifndef DISABLE_CAMERA
    lastFrameBuffer = nullptr;
    #endif
}

CameraController::~CameraController() {
    releaseFrameBuffer();
}

bool CameraController::begin() {
    return begin(quality);
}

bool CameraController::begin(int jpegQuality) {
    #ifdef DISABLE_CAMERA
    state = CAMERA_ERROR;
    return false;
    #else
    quality = jpegQuality;
    
    if (initCamera()) {
        state = CAMERA_READY;
        initialized = true;
        return true;
    }
    
    state = CAMERA_ERROR;
    return false;
    #endif
}

#ifndef DISABLE_CAMERA
bool CameraController::initCamera() {
    camera_config_t config = getCameraConfig();
    
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Error al inicializar cámara: 0x%x\n", err);
        return false;
    }
    
    sensor_t* s = esp_camera_sensor_get();
    if (s) {
        s->set_framesize(s, FRAMESIZE_VGA);
        s->set_quality(s, quality);
    }
    
    return true;
}

camera_config_t CameraController::getCameraConfig() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    
    if (psramFound()) {
        config.frame_size = FRAMESIZE_UXGA;
        config.jpeg_quality = quality;
        config.fb_count = 2;
    } else {
        config.frame_size = FRAMESIZE_SVGA;
        config.jpeg_quality = 12;
        config.fb_count = 1;
    }
    
    return config;
}
#endif

uint8_t* CameraController::capturePhoto(size_t* length) {
    #ifdef DISABLE_CAMERA
    *length = 0;
    return nullptr;
    #else
    
    if (state != CAMERA_READY) {
        *length = 0;
        return nullptr;
    }
    
    releaseFrameBuffer();
    
    state = CAMERA_CAPTURING;
    lastFrameBuffer = esp_camera_fb_get();
    state = CAMERA_READY;
    
    if (!lastFrameBuffer) {
        *length = 0;
        return nullptr;
    }
    
    *length = lastFrameBuffer->len;
    return lastFrameBuffer->buf;
    #endif
}

bool CameraController::captureToBuffer() {
    #ifdef DISABLE_CAMERA
    return false;
    #else
    size_t len;
    return capturePhoto(&len) != nullptr;
    #endif
}

void CameraController::releaseFrameBuffer() {
    #ifndef DISABLE_CAMERA
    if (lastFrameBuffer) {
        esp_camera_fb_return(lastFrameBuffer);
        lastFrameBuffer = nullptr;
    }
    #endif
}

bool CameraController::startStream() {
    return state == CAMERA_READY;
}

void CameraController::stopStream() {
    releaseFrameBuffer();
}

uint8_t* CameraController::getStreamFrame(size_t* length) {
    return capturePhoto(length);
}

void CameraController::setQuality(int jpegQuality) {
    #ifndef DISABLE_CAMERA
    quality = jpegQuality;
    sensor_t* s = esp_camera_sensor_get();
    if (s) {
        s->set_quality(s, quality);
    }
    #endif
}

void CameraController::setFrameSize(int frameSize) {
    #ifndef DISABLE_CAMERA
    sensor_t* s = esp_camera_sensor_get();
    if (s) {
        s->set_framesize(s, (framesize_t)frameSize);
    }
    #endif
}

void CameraController::setBrightness(int brightness) {
    #ifndef DISABLE_CAMERA
    sensor_t* s = esp_camera_sensor_get();
    if (s) {
        s->set_brightness(s, brightness);
    }
    #endif
}

void CameraController::setContrast(int contrast) {
    #ifndef DISABLE_CAMERA
    sensor_t* s = esp_camera_sensor_get();
    if (s) {
        s->set_contrast(s, contrast);
    }
    #endif
}