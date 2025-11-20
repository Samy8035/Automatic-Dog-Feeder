// Variables globales
let updateInterval;
const UPDATE_RATE = 2000; // 2 segundos

// Inicialización
document.addEventListener('DOMContentLoaded', () => {
    initializeEventListeners();
    startAutoUpdate();
    loadConfiguration();
});

// Event Listeners
function initializeEventListeners() {
    document.getElementById('btnFeedNow').addEventListener('click', feedNow);
    document.getElementById('btnCancel').addEventListener('click', cancelFeeding);
    document.getElementById('btnCapture').addEventListener('click', capturePhoto);
    document.getElementById('btnRefresh').addEventListener('click', refreshCamera);
    document.getElementById('btnSaveSchedule').addEventListener('click', saveSchedule);
    document.getElementById('btnSaveConfig').addEventListener('click', saveConfig);
    document.getElementById('btnResetDaily').addEventListener('click', resetDaily);
    document.getElementById('btnReboot').addEventListener('click', rebootSystem);
}

// Actualización automática
function startAutoUpdate() {
    updateStatus();
    updateInterval = setInterval(updateStatus, UPDATE_RATE);
}

async function updateStatus() {
    try {
        const response = await fetch('/api/status');
        const data = await response.json();
        
        if (data.success) {
            updateUI(data);
            setConnectionStatus(true);
        }
    } catch (error) {
        console.error('Error updating status:', error);
        setConnectionStatus(false);
    }
}

function updateUI(data) {
    // Estado de alimentación
    document.getElementById('feedingState').textContent = data.feeding.state;
    document.getElementById('currentCompartment').textContent = data.feeding.compartment;
    
    const progress = data.feeding.progress;
    document.getElementById('feedingProgress').style.width = progress + '%';
    document.getElementById('progressText').textContent = progress.toFixed(1) + '%';
    
    const isFeeding = data.feeding.inProgress;
    document.getElementById('btnFeedNow').disabled = isFeeding;
    document.getElementById('btnCancel').disabled = !isFeeding;
    
    // Sensores
    if (data.sensors.valid) {
        document.getElementById('temperature').textContent = 
            data.sensors.temperature.toFixed(1) + '°C';
        document.getElementById('humidity').textContent = 
            data.sensors.humidity.toFixed(1) + '%';
    }
    
    document.getElementById('presence').textContent = 
        data.sensors.presence ? 'Detectada' : 'No detectada';
    
    // Programación
    document.getElementById('nextFeeding').textContent = data.schedule.nextFeeding;
    document.getElementById('feedingsToday').textContent = 
        data.schedule.todayCount + '/' + data.schedule.maxPerDay;
    
    // Sistema
    document.getElementById('wifiStatus').textContent = data.system.wifi;
    document.getElementById('freeHeap').textContent = 
        (data.system.freeHeap / 1024).toFixed(1) + ' KB';
    document.getElementById('uptime').textContent = formatUptime(data.system.uptime);
    
    // Timestamp
    document.getElementById('lastUpdate').textContent = new Date().toLocaleTimeString();
}

// Acciones de alimentación
async function feedNow() {
    if (!confirm('¿Iniciar alimentación manual ahora?')) return;
    
    try {
        const response = await fetch('/api/feed/now', { method: 'POST' });
        const data = await response.json();
        
        if (data.success) {
            showToast('Alimentación iniciada', 'success');
        } else {
            showToast('Error: ' + data.message, 'error');
        }
    } catch (error) {
        showToast('Error de conexión', 'error');
    }
}

async function cancelFeeding() {
    if (!confirm('¿Cancelar alimentación en curso?')) return;
    
    try {
        const response = await fetch('/api/feed/cancel', { method: 'POST' });
        const data = await response.json();
        
        if (data.success) {
            showToast('Alimentación cancelada', 'success');
        }
    } catch (error) {
        showToast('Error de conexión', 'error');
    }
}

// Cámara
function refreshCamera() {
    const img = document.getElementById('cameraStream');
    const timestamp = new Date().getTime();
    
    // ✅ Manejar errores de carga
    img.onerror = function() {
        showToast('Error al cargar cámara', 'error');
        // Mostrar placeholder
        img.src = 'data:image/svg+xml,<svg xmlns="http://www.w3.org/2000/svg" width="640" height="480"><rect width="640" height="480" fill="%23f0f0f0"/><text x="50%" y="50%" text-anchor="middle" fill="%23999" font-size="20">Cámara no disponible</text></svg>';
    };
    
    img.onload = function() {
        // Quitar handler de error si carga bien
        img.onerror = null;
    };
    
    img.src = '/camera/stream?' + timestamp;
}

async function capturePhoto() {
    showToast('Capturando foto...', 'success');
    
    try {
        const img = document.getElementById('cameraStream');
        const timestamp = new Date().getTime();
        
        // ✅ Verificar que la captura funcionó
        const response = await fetch('/camera/capture?' + timestamp);
        if (!response.ok) {
            throw new Error('Error al capturar foto');
        }
        
        const blob = await response.blob();
        img.src = URL.createObjectURL(blob);
        showToast('Foto capturada', 'success');
    } catch (error) {
        showToast('Error al capturar foto', 'error');
        console.error('Capture error:', error);
    }
}

// Configuración
async function loadConfiguration() {
    try {
        const response = await fetch('/api/config');
        const data = await response.json();
        
        if (data.success) {
            document.getElementById('autoEnabled').checked = data.config.autoEnabled;
            document.getElementById('feedingInterval').value = data.config.feedingInterval;
            document.getElementById('portionsPerDay').value = data.config.portionsPerDay;
            document.getElementById('requirePresence').checked = data.config.requirePresence;
            document.getElementById('playSound').checked = data.config.playSound;
            document.getElementById('tempAlerts').checked = data.config.tempAlerts;
            document.getElementById('humidityAlerts').checked = data.config.humidityAlerts;
        }
    } catch (error) {
        console.error('Error loading config:', error);
    }
}

async function saveSchedule() {
    const config = {
        autoEnabled: document.getElementById('autoEnabled').checked,
        feedingInterval: parseInt(document.getElementById('feedingInterval').value),
        portionsPerDay: parseInt(document.getElementById('portionsPerDay').value)
    };
    
    try {
        const response = await fetch('/api/config/schedule', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(config)
        });
        
        const data = await response.json();
        
        if (data.success) {
            showToast('Configuración guardada', 'success');
        } else {
            showToast('Error al guardar', 'error');
        }
    } catch (error) {
        showToast('Error de conexión', 'error');
    }
}

async function saveConfig() {
    const config = {
        requirePresence: document.getElementById('requirePresence').checked,
        playSound: document.getElementById('playSound').checked,
        tempAlerts: document.getElementById('tempAlerts').checked,
        humidityAlerts: document.getElementById('humidityAlerts').checked
    };
    
    try {
        const response = await fetch('/api/config/advanced', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(config)
        });
        
        const data = await response.json();
        
        if (data.success) {
            showToast('Configuración guardada', 'success');
        } else {
            showToast('Error al guardar', 'error');
        }
    } catch (error) {
        showToast('Error de conexión', 'error');
    }
}

// Sistema
async function resetDaily() {
    if (!confirm('¿Reiniciar contador de alimentaciones diarias?')) return;
    
    try {
        const response = await fetch('/api/system/reset-daily', { method: 'POST' });
        const data = await response.json();
        
        if (data.success) {
            showToast('Contador reiniciado', 'success');
            updateStatus();
        }
    } catch (error) {
        showToast('Error de conexión', 'error');
    }
}

async function rebootSystem() {
    if (!confirm('¿Reiniciar el sistema? Esta acción tomará 10-20 segundos.')) return;
    
    try {
        await fetch('/api/system/reboot', { method: 'POST' });
        showToast('Reiniciando sistema...', 'success');
        
        setTimeout(() => {
            setConnectionStatus(false);
        }, 2000);
        
        setTimeout(() => {
            location.reload();
        }, 15000);
    } catch (error) {
        showToast('Error de conexión', 'error');
    }
}

// Utilidades
function setConnectionStatus(connected) {
    const indicator = document.getElementById('connectionStatus');
    if (connected) {
        indicator.classList.add('connected');
        indicator.querySelector('span:last-child').textContent = 'Conectado';
    } else {
        indicator.classList.remove('connected');
        indicator.querySelector('span:last-child').textContent = 'Desconectado';
    }
}

function showToast(message, type = 'success') {
    const toast = document.getElementById('toast');
    toast.textContent = message;
    toast.className = 'toast show ' + type;
    
    setTimeout(() => {
        toast.classList.remove('show');
    }, 3000);
}

function formatUptime(milliseconds) {
    const seconds = Math.floor(milliseconds / 1000);
    const minutes = Math.floor(seconds / 60);
    const hours = Math.floor(minutes / 60);
    const days = Math.floor(hours / 24);
    
    if (days > 0) return days + 'd ' + (hours % 24) + 'h';
    if (hours > 0) return hours + 'h ' + (minutes % 60) + 'm';
    if (minutes > 0) return minutes + 'm ' + (seconds % 60) + 's';
    return seconds + 's';
}