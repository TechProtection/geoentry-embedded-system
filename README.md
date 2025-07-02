# GeoEntry ESP32 IoT Device

## Descripción
Dispositivo ESP32 que actúa como cliente HTTP para el sistema GeoEntry, integrando funcionalidades de receptor y transmisor para gestión de eventos de proximidad.

## Arquitectura del Sistema
```
App Móvil → REST API → Edge API → ESP32 (este dispositivo)
    ↑                                ↓
    ← REST API ← Edge API ← (confirmación/estado)
```

## Funcionalidades Principales

### 🔄 Cliente HTTP (Receptor)
- **Polling periódico** a la Edge API cada 15 segundos
- Consulta endpoint `/api/v1/locations/proximity-check`
- Verifica si hay alertas de proximidad para el dispositivo
- Procesa respuestas JSON de la API

### 📤 Transmisor (Emisor)
- Envía confirmaciones cuando detecta proximidad
- Reporta estado del dispositivo a la Edge API
- Utiliza endpoint `/api/v1/proximity-events`
- Maneja autenticación con headers HTTP

### 💡 Control de Hardware
- **LED Principal (GPIO2)**: Indicador de proximidad detectada
- **LED Estado (GPIO4)**: Estado general del sistema
- **LED WiFi (GPIO5)**: Estado de conectividad WiFi
- **Botón Reset (GPIO0)**: Reinicio del sistema (3 seg presionado)

## Configuración

### Credenciales WiFi
```cpp
const char* WIFI_SSID = "Tu_Red_WiFi";
const char* WIFI_PASSWORD = "Tu_Contraseña";
```

### Configuración del Dispositivo
```cpp
const char* DEVICE_ID = "esp32-iot-001";        // ID único del dispositivo
const char* API_KEY = "test-api-key-123";       // Clave API
const char* PROFILE_ID = "test-profile";        // Perfil del dispositivo
```

### Ubicación del Dispositivo
```cpp
const float DEVICE_LATITUDE = -12.0464;   // Lima, Perú (ejemplo)
const float DEVICE_LONGITUDE = -77.0428;  // Ajustar según ubicación real
```

### Intervalos de Tiempo
```cpp
const unsigned long POLLING_INTERVAL = 15000;    // 15 segundos
const unsigned long HTTP_TIMEOUT = 8000;         // 8 segundos timeout HTTP
const unsigned long LED_ALERT_DURATION = 5000;   // 5 segundos LED encendido
```

## Estados del Sistema

### 🟢 Estados Normales
- **INITIALIZING**: Inicializando sistema
- **CONNECTING_WIFI**: Conectando a WiFi
- **POLLING_API**: Funcionamiento normal, consultando API
- **PROXIMITY_DETECTED**: Proximidad detectada, LEDs activos

### 🔴 Estados de Error
- **ERROR_STATE**: Error en sistema (3+ fallos consecutivos)
- **RECOVERING**: Intentando recuperar conexión

## Indicadores LED

### LED Principal (GPIO2) - Proximidad
- **OFF**: No hay proximidad detectada
- **ON (5 seg)**: Proximidad detectada y confirmada

### LED Estado (GPIO4) - Sistema
- **Parpadeo lento**: Inicializando
- **Parpadeo rápido**: Conectando WiFi o proximidad activa
- **ON continuo**: Sistema funcionando normalmente
- **Parpadeo muy rápido**: Estado de error

### LED WiFi (GPIO5) - Conectividad
- **ON**: WiFi conectado
- **OFF**: WiFi desconectado
- **Parpadeo**: Intentando conectar

## API Endpoints Utilizados

### 📍 Verificación de Proximidad
```http
POST /api/v1/locations/proximity-check
Headers:
  Content-Type: application/json
  X-Device-ID: esp32-iot-001
  X-API-Key: test-api-key-123

Body:
{
  "latitude": -12.0464,
  "longitude": -77.0428
}
```

### 📤 Confirmación de Eventos
```http
POST /api/v1/proximity-events
Headers:
  Content-Type: application/json
  X-Device-ID: esp32-iot-001
  X-API-Key: test-api-key-123

Body:
{
  "latitude": -12.0464,
  "longitude": -77.0428,
  "event_type": "ENTER",
  "location_id": "home-loc"
}
```

## Características Avanzadas

### 🛡️ Robustez
- **Watchdog Timer**: Reinicio automático si el sistema se cuelga
- **Manejo de errores HTTP**: Reintentos y recuperación automática
- **Verificación WiFi**: Reconexión automática si se pierde
- **Estados del sistema**: Seguimiento detallado del estado

### 📊 Monitoreo
- **Heartbeat**: Reporte de estado cada minuto
- **Estadísticas**: Conteo de requests exitosos vs fallidos
- **Memoria**: Monitoreo de heap libre
- **Uptime**: Tiempo de funcionamiento

### 🔧 Utilidades
- **Reset Manual**: Botón de reinicio por 3 segundos
- **Logs Serial**: Output detallado para debugging
- **Feedback Visual**: Patrones LED para diferentes estados

## Instalación y Uso

### 1. Hardware Requerido
- ESP32 DevKit o similar
- 3 LEDs (opcional, usa LED onboard si no tienes externos)
- Resistencias 220Ω para los LEDs
- Botón push (opcional, usa BOOT button)

### 2. Conexiones
```
GPIO2 (LED_PROXIMITY) → LED Rojo + Resistencia → GND
GPIO4 (LED_STATUS)    → LED Verde + Resistencia → GND  
GPIO5 (LED_WIFI)      → LED Azul + Resistencia → GND
GPIO0 (BUTTON_RESET)  → Botón → GND (pull-up interno)
```

### 3. Software
- Arduino IDE con soporte ESP32
- Librerías: WiFi, HTTPClient, ArduinoJson, esp_task_wdt
- Cargar sketch.ino al ESP32

### 4. Configuración
1. Modificar credenciales WiFi en el código
2. Ajustar DEVICE_ID único
3. Configurar ubicación (DEVICE_LATITUDE, DEVICE_LONGITUDE)
4. Compilar y cargar

## Solución de Problemas

### WiFi no conecta
- Verificar SSID y contraseña
- Comprobar que la red esté disponible
- Revisar señal WiFi (debe ser > -70 dBm)

### API no responde
- Verificar conectividad a internet
- Comprobar que Edge API esté funcionando
- Revisar headers de autenticación

### LEDs no funcionan
- Verificar conexiones de hardware
- Comprobar que los pines GPIO estén libres
- Revisar polaridad de los LEDs

### Sistema se reinicia constantemente
- Verificar alimentación adecuada (5V/3.3V)
- Comprobar que no haya cortocircuitos
- Revisar logs Serial para errores

## Logs y Debugging

### Monitor Serial (115200 baud)
El dispositivo envía información detallada por Serial:
- Estado de inicialización
- Conexión WiFi
- Requests HTTP y respuestas
- Estados de proximidad
- Errores y recuperación
- Heartbeat del sistema

### Ejemplos de Output
```
=================================
GeoEntry ESP32 IoT Device v1.0
=================================
Conectando a WiFi: Mi_Red_WiFi
✓ WiFi conectado!
IP Address: 192.168.1.100
✓ Edge API respondiendo correctamente

--- Polling Edge API ---
Enviando: {"latitude":-12.0464,"longitude":-77.0428}
✓ API Response: {"device_id":"esp32-iot-001",...}
🎯 ¡PROXIMIDAD DETECTADA!
🔴 Activando alerta LED de proximidad
📤 Enviando confirmación de proximidad...
✓ Confirmación enviada exitosamente

💓 HEARTBEAT - Estado del sistema:
├─ Estado: Proximidad Detectada
├─ WiFi: Conectado
├─ Proximidad: Activa
├─ Evento actual: ENTER
└─ Uptime: 125 segundos
```

## Versión y Compatibilidad
- **Versión**: 1.0
- **ESP32 Core**: 2.0.0+
- **Arduino IDE**: 1.8.0+
- **PlatformIO**: Compatible
- **Wokwi Simulator**: Compatible

## Licencia
Proyecto desarrollado para GeoEntry IoT System.

---
*Documentación actualizada: Julio 2025*
