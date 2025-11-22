// ESP32 Dog Feeder Controller - Carrusel con 5 compartimentos
// Con AccelStepper + AsyncWebServer

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AccelStepper.h>

const char *ssid = "Livebox6-9F30";
const char *password = "wifiabajo1";

AsyncWebServer server(80);

// Pines del motor
#define motorStep 14
#define motorDir 13
#define motorEnable 4
#define motorMs 15

AccelStepper stepper(AccelStepper::DRIVER, motorStep, motorDir);

// Variables para el carrusel de 5 compartimentos
const int NUM_COMPARTMENTS = 5;
volatile long stepsPerCompartment = 400; // Steps entre cada compartimento (configurable)
volatile int currentCompartment = 0; // Compartimento actual (0-4)
volatile long lastSteps = 0;
volatile long lastSpeed = 800;

void notFound(AsyncWebServerRequest *request){
  request->send(404, "text/plain", "Not found");
}

// Calcular posición de un compartimento específico
long getCompartmentPosition(int compartment) {
  return compartment * stepsPerCompartment;
}

// Calcular qué compartimento está más cerca de la posición actual
int getCurrentCompartmentFromPosition() {
  long pos = stepper.currentPosition();
  int nearest = (int)((pos + stepsPerCompartment/2) / stepsPerCompartment);
  if (nearest < 0) nearest = 0;
  if (nearest >= NUM_COMPARTMENTS) nearest = NUM_COMPARTMENTS - 1;
  return nearest;
}

void setup(){
  Serial.begin(115200);
  pinMode(motorEnable, OUTPUT);
  digitalWrite(motorEnable, LOW);

  stepper.setMaxSpeed(lastSpeed);
  stepper.setAcceleration(200.0);
  stepper.setCurrentPosition(0); // Iniciar en compartimento 0

  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Conectado. IP: ");
  Serial.println(WiFi.localIP());

  // Página principal
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String page = R"rawliteral(
<!doctype html>
<html lang="es">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Dog Feeder - 5 Compartimentos</title>
<style>
:root{--bg:#f3f4f6;--card:#fff;--accent:#1f73ff;--text:#111;--success:#10b981;--warning:#f59e0b}
html,body{height:100%;margin:0;font-family:Inter,system-ui,Segoe UI,Roboto,"Helvetica Neue",Arial;}
body{background:var(--bg);color:var(--text);padding:20px;}
.container{max-width:900px;margin:0 auto;}
.card{background:var(--card);border-radius:12px;box-shadow:0 6px 18px rgba(15,23,42,0.08);padding:22px;margin-bottom:16px}
.h1{font-size:24px;margin:0 0 8px 0;font-weight:700}
.h2{font-size:18px;margin:0 0 12px 0;font-weight:600}
.subtitle{font-size:14px;color:#64748b;margin-bottom:16px}
.compartments-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(140px,1fr));gap:12px;margin-bottom:20px}
.compartment-btn{padding:16px;border-radius:10px;border:2px solid #e6e9ef;background:#fff;cursor:pointer;transition:all 0.2s;text-align:center}
.compartment-btn:hover{border-color:var(--accent);transform:translateY(-2px);box-shadow:0 4px 12px rgba(31,115,255,0.15)}
.compartment-btn.active{border-color:var(--success);background:#f0fdf4;font-weight:600}
.comp-number{font-size:28px;font-weight:700;color:var(--accent);margin-bottom:4px}
.comp-label{font-size:13px;color:#64748b}
.comp-position{font-size:12px;color:#94a3b8;margin-top:4px}
.config-section{background:#f8fafc;border-radius:8px;padding:16px;margin-bottom:16px}
.form-group{display:flex;flex-direction:column;gap:6px;margin-bottom:12px}
.input{padding:10px;border-radius:8px;border:1px solid #e6e9ef;font-size:14px;width:100%}
.button{padding:10px 16px;border-radius:8px;border:0;background:var(--accent);color:#fff;cursor:pointer;font-weight:600;transition:all 0.2s}
.button:hover{background:#1557cc;transform:translateY(-1px)}
.btn-success{background:var(--success)}
.btn-success:hover{background:#059669}
.btn-warning{background:var(--warning)}
.btn-warning:hover{background:#d97706}
.btn-ghost{background:transparent;border:1px solid #e6e9ef;color:var(--text)}
.btn-ghost:hover{background:#f8fafc}
.small{padding:8px 12px;font-size:13px}
.status-bar{display:flex;justify-content:space-between;align-items:center;gap:12px;flex-wrap:wrap}
.status-badge{background:#f1f5f9;padding:8px 12px;border-radius:8px;border:1px solid #e6e9ef;font-size:13px}
.kv{font-weight:600;color:var(--accent)}
.slider{width:100%;height:8px;border-radius:4px;background:#e6e9ef;outline:none}
.controls-grid{display:grid;grid-template-columns:1fr 1fr;gap:12px}
.btn-row{display:flex;gap:8px;flex-wrap:wrap}
@media(max-width:700px){.controls-grid{grid-template-columns:1fr}}
</style>
</head>
<body>
<div class="container">
  
  <div class="card">
    <div class="h1">🐕 Dog Feeder Controller</div>
    <div class="subtitle">Carrusel con 5 compartimentos - Control paso a paso</div>
    <div class="status-bar">
      <div class="status-badge">IP: <span class="kv" id="ipaddr">...</span></div>
      <div class="status-badge">Posición: <span class="kv" id="pos">0</span> steps</div>
      <div class="status-badge">Compartimento: <span class="kv" id="currentComp">0</span></div>
    </div>
  </div>

  <div class="card">
    <div class="h2">Compartimentos</div>
    <div class="compartments-grid" id="compartmentsGrid"></div>
  </div>

  <div class="card">
    <div class="h2">Configuración del Carrusel</div>
    <div class="config-section">
      <div class="form-group">
        <label><strong>Steps por compartimento</strong></label>
        <input id="stepsPerComp" class="input" type="number" step="1" min="1" />
        <small style="color:#64748b">Define cuántos steps de separación hay entre cada compartimento</small>
      </div>
      <button class="button btn-success" onclick="saveStepsPerComp()">💾 Guardar Configuración</button>
    </div>

    <div class="form-group">
      <label><strong>Velocidad máxima</strong></label>
      <input id="speed" class="slider" type="range" min="10" max="3000" step="10" />
      <div style="display:flex;justify-content:space-between;margin-top:4px">
        <small>10</small><div>Velocidad: <span id="speedVal">800</span></div><small>3000</small>
      </div>
    </div>
  </div>

  <div class="card">
    <div class="h2">Control Manual</div>
    <div class="controls-grid">
      <div>
        <div class="form-group">
          <label>Pasos manuales</label>
          <input id="steps" class="input" type="number" step="1" />
        </div>
        <button class="button" onclick="sendMove()">Mover</button>
      </div>
      <div class="btn-row">
        <button class="button small" onclick="sendMoveBy(100)">+100</button>
        <button class="button small" onclick="sendMoveBy(500)">+500</button>
        <button class="button small" onclick="sendMoveBy(-100)">-100</button>
        <button class="button small" onclick="sendMoveBy(-500)">-500</button>
      </div>
    </div>
    <div style="height:12px"></div>
    <div class="btn-row">
      <button class="btn-ghost button small" onclick="goHome()">🏠 Home (Comp. 0)</button>
      <button class="btn-ghost button small btn-warning" onclick="stopMotor()">⏹ Stop</button>
      <button class="btn-ghost button small" onclick="resetPosition()">🔄 Reset Posición</button>
    </div>
  </div>

</div>

<script>
const NUM_COMPS = 5;
let stepsPerComp = 400;
let currentSpeed = 800;

// Elementos del DOM
const ipSpan = document.getElementById('ipaddr');
const posEl = document.getElementById('pos');
const currentCompEl = document.getElementById('currentComp');
const stepsPerCompEl = document.getElementById('stepsPerComp');
const speedEl = document.getElementById('speed');
const speedVal = document.getElementById('speedVal');
const stepsEl = document.getElementById('steps');

// Cargar estado inicial
async function fetchState(){
  try {
    const r = await fetch('/api/state');
    const j = await r.json();
    stepsPerComp = j.stepsPerCompartment;
    currentSpeed = j.lastSpeed;
    stepsPerCompEl.value = stepsPerComp;
    speedEl.value = currentSpeed;
    speedVal.textContent = currentSpeed;
    ipSpan.textContent = window.location.hostname;
    renderCompartments();
    updatePosition();
  } catch(e){ console.warn('Error cargando estado', e); }
}

// Renderizar botones de compartimentos
function renderCompartments(){
  const grid = document.getElementById('compartmentsGrid');
  grid.innerHTML = '';
  for(let i = 0; i < NUM_COMPS; i++){
    const btn = document.createElement('button');
    btn.className = 'compartment-btn';
    btn.onclick = () => goToCompartment(i);
    const pos = i * stepsPerComp;
    btn.innerHTML = `
      <div class="comp-number">${i}</div>
      <div class="comp-label">Compartimento ${i}</div>
      <div class="comp-position">${pos} steps</div>
    `;
    btn.id = 'comp-' + i;
    grid.appendChild(btn);
  }
}

// Ir a un compartimento específico
async function goToCompartment(comp){
  const speed = parseInt(speedEl.value || currentSpeed);
  await fetch(`/api/goto-compartment?comp=${comp}&speed=${speed}`);
  setTimeout(updatePosition, 100);
}

// Guardar steps por compartimento
async function saveStepsPerComp(){
  const steps = parseInt(stepsPerCompEl.value || 400);
  await fetch(`/api/set-steps-per-comp?steps=${steps}`);
  stepsPerComp = steps;
  alert('✅ Configuración guardada: ' + steps + ' steps por compartimento');
  renderCompartments();
}

// Mover relativo
async function sendMove(){
  const steps = parseInt(stepsEl.value || 0);
  const speed = parseInt(speedEl.value || currentSpeed);
  await fetch(`/api/move?steps=${steps}&speed=${speed}`);
}

async function sendMoveBy(amount){
  const speed = parseInt(speedEl.value || currentSpeed);
  await fetch(`/api/move?steps=${amount}&speed=${speed}`);
}

// Home
async function goHome(){
  const speed = parseInt(speedEl.value || currentSpeed);
  await fetch(`/api/home?speed=${speed}`);
  setTimeout(updatePosition, 100);
}

// Stop
async function stopMotor(){
  await fetch('/api/stop');
}

// Reset posición a 0
async function resetPosition(){
  if(confirm('¿Resetear la posición actual a 0 (Compartimento 0)?')){
    await fetch('/api/reset-position');
    updatePosition();
  }
}

// Actualizar posición y compartimento actual
async function updatePosition(){
  try{
    const r = await fetch('/api/position');
    const j = await r.json();
    posEl.textContent = j.pos;
    currentCompEl.textContent = j.compartment;
    
    // Marcar compartimento activo
    document.querySelectorAll('.compartment-btn').forEach(btn => btn.classList.remove('active'));
    const activeBtn = document.getElementById('comp-' + j.compartment);
    if(activeBtn) activeBtn.classList.add('active');
  }catch(e){}
}

// Event listeners
speedEl.addEventListener('input', ()=>{ speedVal.textContent = speedEl.value; });

// Polling de posición
setInterval(updatePosition, 300);

// Inicializar
fetchState();
</script>
</body>
</html>
)rawliteral";

    request->send(200, "text/html", page);
  });

  // Estado general
  server.on("/api/state", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{\"stepsPerCompartment\":" + String(stepsPerCompartment)
                + ",\"lastSpeed\":" + String(lastSpeed)
                + ",\"currentCompartment\":" + String(currentCompartment) + "}";
    request->send(200, "application/json", json);
  });

  // Posición actual + compartimento
  server.on("/api/position", HTTP_GET, [](AsyncWebServerRequest *request){
    long pos = stepper.currentPosition();
    int comp = getCurrentCompartmentFromPosition();
    currentCompartment = comp;
    String json = "{\"pos\":" + String(pos) + ",\"compartment\":" + String(comp) + "}";
    request->send(200, "application/json", json);
  });

  // Ir a compartimento específico
  server.on("/api/goto-compartment", HTTP_GET, [](AsyncWebServerRequest *request){
    if (!request->hasParam("comp")) {
      request->send(400, "application/json", "{\"ok\":false}");
      return;
    }
    int comp = request->getParam("comp")->value().toInt();
    if (comp < 0 || comp >= NUM_COMPARTMENTS) {
      request->send(400, "application/json", "{\"ok\":false,\"error\":\"invalid compartment\"}");
      return;
    }
    
    long targetPos = getCompartmentPosition(comp);
    if (request->hasParam("speed")) {
      lastSpeed = request->getParam("speed")->value().toInt();
    }
    
    Serial.printf("IR A COMPARTIMENTO %d -> posición %ld (speed: %ld)\n", comp, targetPos, lastSpeed);
    stepper.setMaxSpeed((float)lastSpeed);
    stepper.moveTo(targetPos);
    currentCompartment = comp;
    
    request->send(200, "application/json", "{\"ok\":true,\"compartment\":" + String(comp) + "}");
  });

  // Configurar steps por compartimento
  server.on("/api/set-steps-per-comp", HTTP_GET, [](AsyncWebServerRequest *request){
    if (!request->hasParam("steps")) {
      request->send(400, "application/json", "{\"ok\":false}");
      return;
    }
    long steps = request->getParam("steps")->value().toInt();
    stepsPerCompartment = steps;
    Serial.printf("CONFIGURADO: %ld steps por compartimento\n", steps);
    request->send(200, "application/json", "{\"ok\":true,\"stepsPerCompartment\":" + String(steps) + "}");
  });

  // Mover relativo
  server.on("/api/move", HTTP_GET, [](AsyncWebServerRequest *request){
    if (!request->hasParam("steps")) {
      request->send(400, "application/json", "{\"ok\":false}");
      return;
    }
    long steps = request->getParam("steps")->value().toInt();
    if (request->hasParam("speed")) {
      lastSpeed = request->getParam("speed")->value().toInt();
    }
    Serial.printf("MOVE relativo: %ld steps (speed: %ld)\n", steps, lastSpeed);
    stepper.setMaxSpeed((float)lastSpeed);
    stepper.move(steps);
    request->send(200, "application/json", "{\"ok\":true}");
  });

  // Home (ir a compartimento 0)
  server.on("/api/home", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("speed")) {
      lastSpeed = request->getParam("speed")->value().toInt();
    }
    Serial.println("HOME -> Compartimento 0");
    stepper.setMaxSpeed((float)lastSpeed);
    stepper.moveTo(0);
    currentCompartment = 0;
    request->send(200, "application/json", "{\"ok\":true}");
  });

  // Stop
  server.on("/api/stop", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("STOP");
    stepper.stop();
    request->send(200, "application/json", "{\"ok\":true}");
  });

  // Reset posición (establece la posición actual como 0)
  server.on("/api/reset-position", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("RESET POSICIÓN -> actual = 0");
    stepper.setCurrentPosition(0);
    currentCompartment = 0;
    request->send(200, "application/json", "{\"ok\":true}");
  });

  server.onNotFound(notFound);
  server.begin();
  Serial.println("Servidor web iniciado");
}

void loop(){
  stepper.run();
}