#include "CameraController.h"

CameraController::CameraController()
    : state(CAMERA_UNINITIALIZED),
      initialized(false),
      quality(12) {  // Calidad por defecto mejorada
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
    
    // Inicializar cámara
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Error al inicializar cámara: 0x%x\n", err);
        return false;
    }
    
    // Obtener sensor y aplicar configuración adicional
    sensor_t* s = esp_camera_sensor_get();
    if (s) {
        // Configuración inicial como en el código que funciona
        s->set_vflip(s, 1);        // Voltear verticalmente
        s->set_brightness(s, 1);   // Subir brillo un poco
        s->set_saturation(s, 0);   // Bajar saturación
        
        // Aplicar calidad configurada
        s->set_quality(s, quality);
    }
    
    Serial.println("Cámara inicializada correctamente");
    return true;
}

camera_config_t CameraController::getCameraConfig() {
    camera_config_t config;
    
    // Configuración de canales LEDC
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    
    // Pines de datos (D0-D7)
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    
    // Pines de control
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    
    // Pines I2C para comunicación con el sensor
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    
    // Pines de power y reset
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    
    // Frecuencia del reloj - CRÍTICO: 10MHz como en código funcional
    config.xclk_freq_hz = 10000000;
    
    // Formato de píxel para streaming
    config.pixel_format = PIXFORMAT_JPEG;
    
    // Configuración según disponibilidad de PSRAM
    if (psramFound()) {
        Serial.println("PSRAM encontrado - usando configuración optimizada");
        config.frame_size = FRAMESIZE_SVGA;    // 800x600 como en código funcional
        config.jpeg_quality = 10;               // Calidad alta
        config.fb_count = 2;                    // Doble buffer
        config.grab_mode = CAMERA_GRAB_LATEST;  // Modo de captura más reciente
        config.fb_location = CAMERA_FB_IN_PSRAM; // Usar PSRAM
    } else {
        Serial.println("PSRAM no encontrado - usando configuración básica");
        config.frame_size = FRAMESIZE_SVGA;     // Mantener resolución
        config.jpeg_quality = 12;               // Calidad media
        config.fb_count = 1;                    // Un solo buffer
        config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
        config.fb_location = CAMERA_FB_IN_DRAM; // Usar DRAM
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
        Serial.println("Cámara no está lista para capturar");
        *length = 0;
        return nullptr;
    }
    
    // Liberar buffer anterior si existe
    releaseFrameBuffer();
    
    // Capturar nuevo frame
    state = CAMERA_CAPTURING;
    lastFrameBuffer = esp_camera_fb_get();
    state = CAMERA_READY;
    
    if (!lastFrameBuffer) {
        Serial.println("Error al capturar foto");
        *length = 0;
        return nullptr;
    }
    
    *length = lastFrameBuffer->len;
    Serial.printf("Foto capturada: %d bytes\n", *length);
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
        Serial.printf("Calidad JPEG actualizada a: %d\n", quality);
    }
    #endif
}

void CameraController::setFrameSize(int frameSize) {
    #ifndef DISABLE_CAMERA
    sensor_t* s = esp_camera_sensor_get();
    if (s) {
        s->set_framesize(s, (framesize_t)frameSize);
        Serial.printf("Tamaño de frame actualizado a: %d\n", frameSize);
    }
    #endif
}

void CameraController::setBrightness(int brightness) {
    #ifndef DISABLE_CAMERA
    sensor_t* s = esp_camera_sensor_get();
    if (s) {
        s->set_brightness(s, brightness);
        Serial.printf("Brillo actualizado a: %d\n", brightness);
    }
    #endif
}

void CameraController::setContrast(int contrast) {
    #ifndef DISABLE_CAMERA
    sensor_t* s = esp_camera_sensor_get();
    if (s) {
        s->set_contrast(s, contrast);
        Serial.printf("Contraste actualizado a: %d\n", contrast);
    }
    #endif
}