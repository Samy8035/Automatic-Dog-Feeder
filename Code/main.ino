/*
 * Dispensador Automático de Comida para Perro con ESP32-CAM
 * Características:
 * - 8 compartimentos giratorios
 * - Dispensado cada 4 horas (08:00, 12:00, 16:00, 20:00)
 * - Interfaz web para configuración
 * - Cámara en vivo
 * - Sensor PIR y buzzer opcionales
 * - Configuración persistente en LittleFS
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <time.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <AccelStepper.h>
#include "esp_camera.h"

// ========== CONFIGURACIÓN DE PINES ==========
// Motor paso a paso (compatible con A4988/DRV8825)
#define STEP_PIN 12
#define DIR_PIN 13
#define ENABLE_PIN 15

// Sensores y actuadores
#define PIR_PIN 14
#define BUZZER_PIN 2

// Configuración del ESP32-CAM (AI-Thinker)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// ========== CONFIGURACIÓN WIFI ==========
const char* ssid = "TU_WIFI";
const char* password = "TU_PASSWORD";

// ========== SERVIDOR WEB ==========
WebServer server(80);

// ========== MOTOR PASO A PASO ==========
// Configurar motor: AccelStepper(DRIVER, STEP_PIN, DIR_PIN)
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

// ========== CONFIGURACIÓN NTP ==========
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;  // Ajustar según zona horaria
const int daylightOffset_sec = 0;

// ========== VARIABLES GLOBALES ==========
struct Config {
  int dispenseHours[4] = {8, 12, 16, 20};
  bool useBuzzer = true;
  bool usePIR = false;
  int currentCompartment = 0;
  String lastDispense = "Nunca";
} config;

const int COMPARTMENTS = 8;
const int STEPS_PER_COMPARTMENT = 200; // Ajustar según tu motor (200 pasos = 1.8°, 400 para 0.9°)
bool systemInitialized = false;

// ========== FUNCIONES DE CONFIGURACIÓN ==========

void saveConfig() {
  File file = LittleFS.open("/config.json", "w");
  if (!file) {
    Serial.println("Error al abrir archivo de configuración");
    return;
  }
  
  StaticJsonDocument<512> doc;
  JsonArray hours = doc.createNestedArray("dispenseHours");
  for (int i = 0; i < 4; i++) {
    hours.add(config.dispenseHours[i]);
  }
  doc["useBuzzer"] = config.useBuzzer;
  doc["usePIR"] = config.usePIR;
  doc["currentCompartment"] = config.currentCompartment;
  doc["lastDispense"] = config.lastDispense;
  
  serializeJson(doc, file);
  file.close();
  Serial.println("Configuración guardada");
}

void loadConfig() {
  if (!LittleFS.exists("/config.json")) {
    Serial.println("Archivo de configuración no existe, usando valores predeterminados");
    saveConfig();
    return;
  }
  
  File file = LittleFS.open("/config.json", "r");
  if (!file) {
    Serial.println("Error al leer configuración");
    return;
  }
  
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  
  if (error) {
    Serial.println("Error al parsear JSON");
    return;
  }
  
  for (int i = 0; i < 4; i++) {
    config.dispenseHours[i] = doc["dispenseHours"][i];
  }
  config.useBuzzer = doc["useBuzzer"];
  config.usePIR = doc["usePIR"];
  config.currentCompartment = doc["currentCompartment"];
  config.lastDispense = doc["lastDispense"].as<String>();
  
  Serial.println("Configuración cargada");
}

// ========== FUNCIONES DE HARDWARE ==========

void initCamera() {
  camera_config_t camera_config;
  camera_config.ledc_channel = LEDC_CHANNEL_0;
  camera_config.ledc_timer = LEDC_TIMER_0;
  camera_config.pin_d0 = Y2_GPIO_NUM;
  camera_config.pin_d1 = Y3_GPIO_NUM;
  camera_config.pin_d2 = Y4_GPIO_NUM;
  camera_config.pin_d3 = Y5_GPIO_NUM;
  camera_config.pin_d4 = Y6_GPIO_NUM;
  camera_config.pin_d5 = Y7_GPIO_NUM;
  camera_config.pin_d6 = Y8_GPIO_NUM;
  camera_config.pin_d7 = Y9_GPIO_NUM;
  camera_config.pin_xclk = XCLK_GPIO_NUM;
  camera_config.pin_pclk = PCLK_GPIO_NUM;
  camera_config.pin_vsync = VSYNC_GPIO_NUM;
  camera_config.pin_href = HREF_GPIO_NUM;
  camera_config.pin_sscb_sda = SIOD_GPIO_NUM;
  camera_config.pin_sscb_scl = SIOC_GPIO_NUM;
  camera_config.pin_pwdn = PWDN_GPIO_NUM;
  camera_config.pin_reset = RESET_GPIO_NUM;
  camera_config.xclk_freq_hz = 20000000;
  camera_config.pixel_format = PIXFORMAT_JPEG;
  camera_config.frame_size = FRAMESIZE_VGA;
  camera_config.jpeg_quality = 12;
  camera_config.fb_count = 1;
  
  esp_err_t err = esp_camera_init(&camera_config);
  if (err != ESP_OK) {
    Serial.printf("Error al inicializar cámara: 0x%x\n", err);
    return;
  }
  Serial.println("Cámara inicializada");
}

void playBuzzer() {
  if (!config.useBuzzer) return;
  
  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
  }
}

bool checkPIR() {
  if (!config.usePIR) return true;
  
  Serial.println("Esperando detección de movimiento...");
  unsigned long startTime = millis();
  
  while (millis() - startTime < 30000) { // Esperar 30 segundos máximo
    if (digitalRead(PIR_PIN) == HIGH) {
      Serial.println("Movimiento detectado!");
      return true;
    }
    delay(100);
  }
  
  Serial.println("Timeout: no se detectó movimiento");
  return false;
}

void rotateMotor(int steps, bool clockwise) {
  digitalWrite(ENABLE_PIN, LOW); // Habilitar motor
  
  // Configurar dirección y mover
  int targetPos = stepper.currentPosition() + (clockwise ? steps : -steps);
  stepper.moveTo(targetPos);
  
  // Ejecutar movimiento de forma bloqueante
  while (stepper.distanceToGo() != 0) {
    stepper.run();
  }
  
  digitalWrite(ENABLE_PIN, HIGH); // Deshabilitar motor
}

void dispenseFood() {
  Serial.println("Iniciando dispensado...");
  
  // 1. Emitir sonido
  playBuzzer();
  delay(1000);
  
  // 2. Detectar movimiento (si está habilitado)
  if (config.usePIR && !checkPIR()) {
    Serial.println("Dispensado cancelado: no se detectó movimiento");
    return;
  }
  
  // 3. Girar al siguiente compartimento
  rotateMotor(STEPS_PER_COMPARTMENT, true);
  
  // Actualizar contador de compartimento
  config.currentCompartment = (config.currentCompartment + 1) % COMPARTMENTS;
  
  // Actualizar última dispensación
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    char buffer[30];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
    config.lastDispense = String(buffer);
  }
  
  saveConfig();
  Serial.println("Dispensado completado!");
}

// ========== FUNCIONES DE SERVIDOR WEB ==========

const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Dispensador Automático</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
      padding: 20px;
    }
    .container {
      max-width: 900px;
      margin: 0 auto;
      background: white;
      border-radius: 20px;
      box-shadow: 0 20px 60px rgba(0,0,0,0.3);
      overflow: hidden;
    }
    .header {
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: white;
      padding: 30px;
      text-align: center;
    }
    .header h1 {
      font-size: 2em;
      margin-bottom: 10px;
    }
    .content {
      padding: 30px;
    }
    .section {
      margin-bottom: 30px;
      padding: 20px;
      background: #f8f9fa;
      border-radius: 10px;
    }
    .section h2 {
      color: #667eea;
      margin-bottom: 15px;
      font-size: 1.5em;
    }
    .camera-feed {
      width: 100%;
      border-radius: 10px;
      box-shadow: 0 4px 6px rgba(0,0,0,0.1);
    }
    .form-group {
      margin-bottom: 20px;
    }
    label {
      display: block;
      margin-bottom: 8px;
      font-weight: 600;
      color: #333;
    }
    input[type="number"] {
      width: 100px;
      padding: 10px;
      border: 2px solid #ddd;
      border-radius: 5px;
      font-size: 16px;
    }
    input[type="checkbox"] {
      width: 20px;
      height: 20px;
      cursor: pointer;
    }
    .hours-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(120px, 1fr));
      gap: 15px;
    }
    .btn {
      padding: 12px 30px;
      border: none;
      border-radius: 8px;
      font-size: 16px;
      font-weight: 600;
      cursor: pointer;
      transition: all 0.3s;
      margin-right: 10px;
      margin-bottom: 10px;
    }
    .btn-primary {
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: white;
    }
    .btn-primary:hover {
      transform: translateY(-2px);
      box-shadow: 0 5px 15px rgba(102, 126, 234, 0.4);
    }
    .btn-success {
      background: #28a745;
      color: white;
    }
    .btn-success:hover {
      background: #218838;
      transform: translateY(-2px);
    }
    .status-info {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 15px;
    }
    .status-card {
      padding: 15px;
      background: white;
      border-radius: 8px;
      border-left: 4px solid #667eea;
    }
    .status-card h3 {
      font-size: 0.9em;
      color: #666;
      margin-bottom: 5px;
    }
    .status-card p {
      font-size: 1.2em;
      color: #333;
      font-weight: 600;
    }
    @media (max-width: 600px) {
      .status-info {
        grid-template-columns: 1fr;
      }
    }
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
      <h1>🐕 Dispensador Automático</h1>
      <p>Control inteligente de alimentación</p>
    </div>
    
    <div class="content">
      <!-- Cámara en vivo -->
      <div class="section">
        <h2>📹 Cámara en Vivo</h2>
        <img class="camera-feed" src="/stream" alt="Cámara">
      </div>
      
      <!-- Estado actual -->
      <div class="section">
        <h2>📊 Estado Actual</h2>
        <div class="status-info">
          <div class="status-card">
            <h3>Última Dispensación</h3>
            <p id="lastDispense">%LAST_DISPENSE%</p>
          </div>
          <div class="status-card">
            <h3>Próxima Dispensación</h3>
            <p id="nextDispense">%NEXT_DISPENSE%</p>
          </div>
          <div class="status-card">
            <h3>Compartimento Actual</h3>
            <p>%CURRENT_COMPARTMENT% / 8</p>
          </div>
          <div class="status-card">
            <h3>Hora del Sistema</h3>
            <p id="systemTime">--:--:--</p>
          </div>
        </div>
      </div>
      
      <!-- Configuración -->
      <div class="section">
        <h2>⚙️ Configuración</h2>
        <form action="/save" method="POST">
          <div class="form-group">
            <label>Horas de Dispensado:</label>
            <div class="hours-grid">
              <div>
                <label>Hora 1:</label>
                <input type="number" name="hour1" value="%HOUR1%" min="0" max="23">:00
              </div>
              <div>
                <label>Hora 2:</label>
                <input type="number" name="hour2" value="%HOUR2%" min="0" max="23">:00
              </div>
              <div>
                <label>Hora 3:</label>
                <input type="number" name="hour3" value="%HOUR3%" min="0" max="23">:00
              </div>
              <div>
                <label>Hora 4:</label>
                <input type="number" name="hour4" value="%HOUR4%" min="0" max="23">:00
              </div>
            </div>
          </div>
          
          <div class="form-group">
            <label>
              <input type="checkbox" name="useBuzzer" value="1" %BUZZER_CHECKED%>
              Usar sonido antes de dispensar
            </label>
          </div>
          
          <div class="form-group">
            <label>
              <input type="checkbox" name="usePIR" value="1" %PIR_CHECKED%>
              Usar detección de movimiento
            </label>
          </div>
          
          <button type="submit" class="btn btn-primary">💾 Guardar Configuración</button>
        </form>
      </div>
      
      <!-- Control manual -->
      <div class="section">
        <h2>🎮 Control Manual</h2>
        <button onclick="dispenseNow()" class="btn btn-success">🍖 Dispensar Ahora</button>
      </div>
    </div>
  </div>
  
  <script>
    function dispenseNow() {
      if (confirm('¿Deseas dispensar comida ahora?')) {
        fetch('/dispense').then(() => {
          alert('Dispensando comida...');
          setTimeout(() => location.reload(), 2000);
        });
      }
    }
    
    function updateTime() {
      fetch('/time').then(r => r.text()).then(time => {
        document.getElementById('systemTime').textContent = time;
      });
    }
    
    setInterval(updateTime, 1000);
    updateTime();
  </script>
</body>
</html>
)rawliteral";

String getNextDispenseTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "Error de hora";
  }
  
  int currentHour = timeinfo.tm_hour;
  int nextHour = -1;
  
  for (int i = 0; i < 4; i++) {
    if (config.dispenseHours[i] > currentHour) {
      nextHour = config.dispenseHours[i];
      break;
    }
  }
  
  if (nextHour == -1) {
    nextHour = config.dispenseHours[0];
  }
  
  char buffer[20];
  sprintf(buffer, "%02d:00:00", nextHour);
  return String(buffer);
}

String processor(const String& var) {
  if (var == "LAST_DISPENSE") return config.lastDispense;
  if (var == "NEXT_DISPENSE") return getNextDispenseTime();
  if (var == "CURRENT_COMPARTMENT") return String(config.currentCompartment + 1);
  if (var == "HOUR1") return String(config.dispenseHours[0]);
  if (var == "HOUR2") return String(config.dispenseHours[1]);
  if (var == "HOUR3") return String(config.dispenseHours[2]);
  if (var == "HOUR4") return String(config.dispenseHours[3]);
  if (var == "BUZZER_CHECKED") return config.useBuzzer ? "checked" : "";
  if (var == "PIR_CHECKED") return config.usePIR ? "checked" : "";
  return String();
}

void handleRoot() {
  String html = htmlPage;
  html.replace("%LAST_DISPENSE%", config.lastDispense);
  html.replace("%NEXT_DISPENSE%", getNextDispenseTime());
  html.replace("%CURRENT_COMPARTMENT%", String(config.currentCompartment + 1));
  html.replace("%HOUR1%", String(config.dispenseHours[0]));
  html.replace("%HOUR2%", String(config.dispenseHours[1]));
  html.replace("%HOUR3%", String(config.dispenseHours[2]));
  html.replace("%HOUR4%", String(config.dispenseHours[3]));
  html.replace("%BUZZER_CHECKED%", config.useBuzzer ? "checked" : "");
  html.replace("%PIR_CHECKED%", config.usePIR ? "checked" : "");
  
  server.send(200, "text/html", html);
}

void handleSave() {
  if (server.hasArg("hour1")) config.dispenseHours[0] = server.arg("hour1").toInt();
  if (server.hasArg("hour2")) config.dispenseHours[1] = server.arg("hour2").toInt();
  if (server.hasArg("hour3")) config.dispenseHours[2] = server.arg("hour3").toInt();
  if (server.hasArg("hour4")) config.dispenseHours[3] = server.arg("hour4").toInt();
  
  config.useBuzzer = server.hasArg("useBuzzer");
  config.usePIR = server.hasArg("usePIR");
  
  saveConfig();
  
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleDispense() {
  dispenseFood();
  server.send(200, "text/plain", "OK");
}

void handleTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    server.send(200, "text/plain", "Error");
    return;
  }
  
  char buffer[20];
  strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
  server.send(200, "text/plain", buffer);
}

void handleStream() {
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    server.send(500, "text/plain", "Error al capturar imagen");
    return;
  }
  
  server.sendHeader("Content-Type", "image/jpeg");
  server.sendHeader("Content-Length", String(fb->len));
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.send_P(200, "image/jpeg", (const char*)fb->buf, fb->len);
  
  esp_camera_fb_return(fb);
}

// ========== FUNCIONES DE CONTROL DE TIEMPO ==========

void checkSchedule() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return;
  }
  
  static int lastHour = -1;
  int currentHour = timeinfo.tm_hour;
  int currentMinute = timeinfo.tm_min;
  
  // Solo verificar en el minuto 0 de cada hora
  if (currentMinute == 0 && currentHour != lastHour) {
    for (int i = 0; i < 4; i++) {
      if (currentHour == config.dispenseHours[i]) {
        Serial.println("¡Hora de dispensar!");
        dispenseFood();
        break;
      }
    }
    lastHour = currentHour;
  }
  
  // Resetear el contador de hora al cambiar de minuto
  if (currentMinute != 0) {
    lastHour = -1;
  }
}

// ========== SETUP ==========

void setup() {
  Serial.begin(115200);
  Serial.println("\n\nIniciando Dispensador Automático...");
  
  // Inicializar pines
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  
  digitalWrite(ENABLE_PIN, HIGH); // Motor deshabilitado por defecto
  digitalWrite(BUZZER_PIN, LOW);
  
  // Configurar motor paso a paso con AccelStepper
  stepper.setMaxSpeed(1000);      // Pasos por segundo (ajustar según necesidad)
  stepper.setAcceleration(500);   // Pasos/s² (ajustar para suavidad)
  stepper.setCurrentPosition(0);
  
  // Inicializar LittleFS
  if (!LittleFS.begin(true)) {
    Serial.println("Error al montar LittleFS");
    return;
  }
  Serial.println("LittleFS montado");
  
  // Cargar configuración
  loadConfig();
  
  // Inicializar cámara
  initCamera();
  
  // Conectar WiFi
  Serial.print("Conectando a WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  
  // Configurar NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("Sincronizando hora...");
  
  // Esperar sincronización
  struct tm timeinfo;
  int attempts = 0;
  while (!getLocalTime(&timeinfo) && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println("\nHora sincronizada");
  
  // Configurar servidor web
  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/dispense", handleDispense);
  server.on("/time", handleTime);
  server.on("/stream", handleStream);
  
  server.begin();
  Serial.println("Servidor web iniciado");
  
  // Configurar mDNS
  if (MDNS.begin("feeder")) {
    Serial.println("mDNS iniciado: http://feeder.local");
  }
  
  systemInitialized = true;
  Serial.println("Sistema listo!");
}

// ========== LOOP ==========

void loop() {
  server.handleClient();
  
  if (systemInitialized) {
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck > 30000) { // Verificar cada 30 segundos
      checkSchedule();
      lastCheck = millis();
    }
  }
  
  delay(10);
}
