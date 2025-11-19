# 🐕 Comedero Automático ESP32

Sistema de alimentación automática para mascotas con control remoto, monitoreo ambiental y visión por cámara.

## 📋 Características

- ✅ **Alimentación Programada**: 4 raciones automáticas cada 4 horas
- ✅ **Carrusel Motorizado**: 5 compartimentos con motor stepper de alta precisión
- ✅ **Detección de Presencia**: Sensor PIR para confirmar que la mascota está presente
- ✅ **Alerta Sonora**: Speaker que avisa antes de dispensar comida
- ✅ **Monitoreo Ambiental**: Sensor DHT22 para temperatura y humedad del pienso
- ✅ **Cámara ESP32-S3**: Visualización en tiempo real y fotos bajo demanda
- ✅ **Control por Telegram**: Bot completo con comandos y notificaciones
- ✅ **Interfaz Web**: Panel de control accesible desde cualquier navegador
- ✅ **Configuración Persistente**: Todas las opciones se guardan en memoria flash

## 🔧 Hardware Necesario

### Componentes Principales
- **ESP32-S3-WROOM** con módulo de cámara (o ESP32 estándar sin cámara)
- **Motor Stepper NEMA 17** (200 pasos/revolución)
- **Driver A4988 o DRV8825** para el motor stepper
- **Sensor DHT22** (temperatura y humedad)
- **Sensor PIR HC-SR501** (detección de movimiento)
- **Buzzer activo 5V** o speaker pequeño
- **Fuente de alimentación 12V 2A** (para el motor)
- **Regulador LM7805** o equivalente (para alimentar el ESP32)

### Conexiones

```
ESP32-S3          →  Componente
GPIO 2            →  STEP (A4988)
GPIO 4            →  DIR (A4988)
GPIO 15           →  ENABLE (A4988)
GPIO 5            →  DHT22 Data
GPIO 18           →  PIR OUT
GPIO 19           →  Buzzer +
GND               →  GND común
3.3V              →  DHT22 VCC, PIR VCC
```

**⚠️ IMPORTANTE**: Alimenta el motor stepper con 12V desde una fuente externa. NO uses el pin VIN del ESP32.

## 📂 Estructura del Proyecto

```
ComederoAutomatico/
├── platformio.ini              # Configuración y dependencias
├── src/
│   ├── main.cpp                # Código principal
│   ├── config.h                # Configuración global
│   │
│   ├── hardware/               # Control de hardware
│   │   ├── StepperController.h/cpp
│   │   ├── SensorManager.h/cpp
│   │   └── CameraController.h/cpp
│   │
│   ├── feeding/                # Lógica de alimentación
│   │   ├── FeedingLogic.h/cpp
│   │   └── FeedingScheduler.h/cpp
│   │
│   ├── communication/          # Comunicaciones
│   │   ├── WebServer.h/cpp
│   │   └── TelegramBot.h/cpp
│   │
│   ├── storage/                # Almacenamiento
│   │   └── ConfigManager.h/cpp
│   │
│   └── utils/                  # Utilidades
│       └── Logger.h/cpp
│
└── data/web/                   # Interfaz web
    ├── index.html
    ├── style.css
    └── script.js
```

## 🚀 Instalación

### 1. Configurar PlatformIO

```bash
# Instalar PlatformIO Core
pip install platformio

# Clonar o crear el proyecto
mkdir ComederoAutomatico
cd ComederoAutomatico
```

### 2. Configurar WiFi y Telegram

Edita `src/config.h`:

```cpp
#define WIFI_SSID "TU_RED_WIFI"
#define WIFI_PASSWORD "TU_CONTRASEÑA"
#define BOT_TOKEN "TOKEN_DE_TU_BOT_TELEGRAM"
```

Para obtener un token de Telegram:
1. Habla con [@BotFather](https://t.me/botfather)
2. Crea un nuevo bot: `/newbot`
3. Copia el token que te proporciona

### 3. Compilar y Subir

```bash
# Compilar
pio run

# Subir al ESP32
pio run --target upload

# Ver monitor serial
pio device monitor
```

### 4. Subir el Sistema de Archivos (opcional)

```bash
pio run --target uploadfs
```

## 🎮 Uso

### Control por Telegram

Comandos disponibles:

- `/start` - Iniciar bot y mostrar ayuda
- `/estado` - Ver estado completo del sistema
- `/alimentar` - Dispensar comida inmediatamente
- `/foto` - Capturar y enviar foto actual
- `/sensores` - Ver temperatura y humedad
- `/horario` - Ver próxima alimentación programada
- `/activar` - Activar alimentación automática
- `/desactivar` - Desactivar alimentación automática
- `/rellenar` - Marcar que has rellenado el carrusel
- `/configurar` - Cambiar parámetros del sistema
- `/ayuda` - Mostrar todos los comandos

### Interfaz Web

Accede desde tu navegador a: `http://[IP_DEL_ESP32]`

La interfaz web te permite:
- Ver estado en tiempo real
- Dispensar comida manualmente
- Configurar horarios y parámetros
- Ver feed de la cámara
- Revisar historial de alimentaciones
- Ajustar alertas ambientales

## ⚙️ Configuración Avanzada

### Modificar Intervalos de Alimentación

En `config.h`:

```cpp
#define DEFAULT_FEEDING_INTERVAL_HOURS 4  // Cambiar a 6, 8, etc.
#define DEFAULT_PORTIONS_PER_DAY 4         // Raciones diarias
```

### Ajustar Sensibilidad del Motor

```cpp
#define STEPPER_MAX_SPEED 1000        // Velocidad máxima
#define STEPPER_ACCELERATION 500      // Aceleración
#define MICROSTEPS 16                 // Microstepping (8, 16, 32)
```

### Personalizar Alertas Ambientales

```cpp
#define TEMP_MIN_ALERT 5.0      // °C mínima
#define TEMP_MAX_ALERT 35.0     // °C máxima
#define HUMIDITY_MAX_ALERT 70.0 // % máxima humedad
```

### Desactivar Funciones

```cpp
// En FeedingLogic
feedingLogic.enableSound(false);           // Sin sonido
feedingLogic.requirePresence(false);        // Sin detección
```

## 🔍 Solución de Problemas

### El motor no se mueve
- Verifica la alimentación de 12V
- Comprueba que el pin ENABLE esté en LOW
- Ajusta el potenciómetro del driver A4988

### Sensor DHT22 devuelve NaN
- Espera 2 segundos entre lecturas
- Verifica las conexiones (VCC, GND, DATA)
- Prueba con otro sensor

### No conecta a WiFi
- Verifica credenciales en `config.h`
- Asegúrate de estar en red 2.4GHz (no 5GHz)
- Verifica que el ESP32 esté cerca del router

### Bot de Telegram no responde
- Verifica el token del bot
- Asegura que el bot está iniciado (`/start`)
- Comprueba conexión a internet del ESP32

### Cámara no funciona
- Usa un ESP32-S3 con módulo de cámara
- Verifica pines en `config.h`
- Reduce calidad de imagen si hay problemas

## 📊 Monitoreo y Logs

El sistema genera logs detallados en el Serial Monitor:

```
[INFO] Alimentación completada: Éxito
[WARNING] Alerta ambiental: Humedad alta (75%)
[DEBUG] Movimiento del motor completado
[ERROR] Error en alimentación: Timeout sin presencia
```

Niveles de log configurables en `config.h`:
- `0` = ERROR
- `1` = WARNING
- `2` = INFO (recomendado)
- `3` = DEBUG

## 🔒 Seguridad

### Restringir Acceso a Telegram

En `config.h`, establece tu Chat ID:

```cpp
#define ALLOWED_CHAT_ID 123456789  // Solo tú puedes controlar
```

Para obtener tu Chat ID:
1. Envía un mensaje a tu bot
2. Visita: `https://api.telegram.org/bot[TOKEN]/getUpdates`
3. Busca tu `chat.id`

### Autenticación Web (opcional)

Implementa autenticación básica en `WebServer.cpp`:

```cpp
server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    if(!request->authenticate("admin", "password"))
        return request->requestAuthentication();
    // ... resto del código
});
```

## 🛠️ Desarrollo y Extensión

### Añadir Nuevos Comandos de Telegram

En `TelegramBot.cpp`:

```cpp
void TelegramBotManager::handleCommand(String command, String chatId) {
    if (command == "/micomando") {
        // Tu código aquí
        sendMessage("Respuesta");
    }
}
```

### Agregar Endpoints a la API Web

En `WebServer.cpp`:

```cpp
server.on("/api/nueva-ruta", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{\"respuesta\":\"datos\"}";
    request->send(200, "application/json", json);
});
```

### Crear Nuevos Sensores

1. Crea archivo `NuevoSensor.h/cpp` en `hardware/`
2. Inicializa en `setup()`
3. Actualiza en `loop()`
4. Integra con `SensorManager`

## 📝 Licencia

Este proyecto es de código abierto. Siéntete libre de modificarlo y mejorarlo.

## 🤝 Contribuciones

¡Las contribuciones son bienvenidas! 

Ideas para mejoras:
- Soporte para múltiples mascotas
- Integración con Home Assistant
- App móvil nativa
- Machine learning para detectar la mascota
- Pesaje automático de raciones

## 📞 Soporte

Para problemas o preguntas:
- Revisa la sección de Solución de Problemas
- Consulta los logs del Serial Monitor
- Verifica las conexiones de hardware

---

**Hecho con ❤️ para nuestras mascotas**













