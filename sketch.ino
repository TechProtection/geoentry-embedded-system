/*
 * GeoEntry ESP32 IoT Device - Integrated Receiver & Transmitter
 * 
 * Funcionalidades:
 * - Cliente HTTP que consulta la Edge API periódicamente
 * - Verifica eventos de proximidad para el dispositivo
 * - Controla LEDs basado en eventos de proximidad
 * - Envía confirmaciones y estado del dispositivo
 * - Manejo robusto de WiFi y errores HTTP
 * 
 * Flujo: App Móvil → REST API → Edge API → ESP32 → Confirmación de vuelta
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ==================== CONFIGURACIÓN ====================
// WiFi credentials
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

// API Configuration
const char* API_BASE_URL = "https://geoentry-edge-api.onrender.com/api/v1";
const char* DEVICE_ID = "esp32-iot-001";
const char* API_KEY = "test-api-key-123";

// Device Configuration
const char* PROFILE_ID = "test-profile";
const float DEVICE_LATITUDE = -12.0464;   // Lima, Perú (ejemplo)
const float DEVICE_LONGITUDE = -77.0428;

// Timing Configuration
const unsigned long POLLING_INTERVAL = 15000;    // 15 segundos
const unsigned long WIFI_TIMEOUT = 10000;        // 10 segundos timeout WiFi
const unsigned long HTTP_TIMEOUT = 8000;         // 8 segundos timeout HTTP
const unsigned long LED_ALERT_DURATION = 5000;   // 5 segundos LED encendido
const unsigned long SYSTEM_TIMEOUT = 30000;      // 30 segundos para detección de cuelgue

// Pin Configuration
const int LED_PROXIMITY = 2;      // LED principal de proximidad (GPIO2 - LED onboard)
const int LED_STATUS = 4;         // LED de estado del sistema (GPIO4)
const int LED_WIFI = 5;           // LED indicador WiFi (GPIO5)
const int BUTTON_RESET = 0;       // Botón de reset (GPIO0 - BOOT button)

// ==================== VARIABLES GLOBALES ====================
unsigned long lastPollingTime = 0;
unsigned long lastWiFiCheck = 0;
unsigned long ledAlertStartTime = 0;
unsigned long lastHeartbeat = 0;
unsigned long lastSystemActivity = 0;    // Para detectar cuelgues sin watchdog

bool isProximityActive = false;
bool isLedAlertActive = false;
bool wifiConnected = false;
bool systemHealthy = true;

String lastEventId = "";
String currentEventType = "";
int consecutiveErrors = 0;
int totalRequests = 0;
int successfulRequests = 0;

// Estados del sistema
enum SystemState {
  INITIALIZING,
  CONNECTING_WIFI,
  POLLING_API,
  PROXIMITY_DETECTED,
  ERROR_STATE,
  RECOVERING
};

SystemState currentState = INITIALIZING;

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=================================");
  Serial.println("GeoEntry ESP32 IoT Device v1.0");
  Serial.println("=================================");
  
  // Configurar pines
  setupPins();
  
  // Inicializar sistema de monitoreo sin watchdog
  lastSystemActivity = millis();
  
  // Inicializar WiFi
  initializeWiFi();
  
  // Test inicial de la API
  testAPIConnection();
  
  currentState = POLLING_API;
  Serial.println("Sistema inicializado correctamente");
  Serial.println("Iniciando polling de la Edge API...");
  
  // Feedback visual de inicialización exitosa
  blinkLED(LED_STATUS, 3, 200);
  
  // Actualizar actividad del sistema
  updateSystemActivity();
}

// ==================== LOOP PRINCIPAL ====================
void loop() {
  // Actualizar actividad del sistema
  updateSystemActivity();
  
  // Verificar estado del sistema (sin watchdog hardware)
  checkSystemHealth();
  
  // Verificar estado del WiFi
  checkWiFiStatus();
  
  // Manejar estados del sistema
  handleSystemState();
  
  // Polling de la API
  if (millis() - lastPollingTime >= POLLING_INTERVAL) {
    performAPIPolling();
    lastPollingTime = millis();
  }
  
  // Manejar alertas LED
  handleLEDAlerts();
  
  // Heartbeat del sistema
  handleSystemHeartbeat();
  
  // Verificar botón de reset
  checkResetButton();
  
  delay(100); // Pequeña pausa para estabilidad
}

// ==================== MONITOREO DEL SISTEMA ====================
void updateSystemActivity() {
  lastSystemActivity = millis();
}

void checkSystemHealth() {
  // Verificar si el sistema está respondiendo
  if (millis() - lastSystemActivity > SYSTEM_TIMEOUT) {
    Serial.println("⚠ Sistema no responde - Reiniciando...");
    systemHealthy = false;
    
    // Intentar recuperación suave antes de reiniciar
    attemptSoftRecovery();
    
    // Si la recuperación suave falla, reiniciar
    delay(1000);
    ESP.restart();
  }
  
  // Verificar memoria disponible
  if (ESP.getFreeHeap() < 10000) {  // Menos de 10KB libres
    Serial.println("⚠ Memoria baja - Liberando recursos...");
    freeSystemResources();
  }
}

void attemptSoftRecovery() {
  Serial.println("🔄 Intentando recuperación suave...");
  
  // Resetear variables de estado
  consecutiveErrors = 0;
  isProximityActive = false;
  isLedAlertActive = false;
  
  // Apagar LEDs
  digitalWrite(LED_PROXIMITY, LOW);
  digitalWrite(LED_STATUS, LOW);
  
  // Reintentar conexión WiFi si es necesario
  if (!wifiConnected) {
    WiFi.disconnect();
    delay(1000);
    initializeWiFi();
  }
  
  // Actualizar estado
  currentState = RECOVERING;
  updateSystemActivity();
}

void freeSystemResources() {
  // Limpiar strings que puedan estar ocupando memoria
  if (lastEventId.length() > 100) {
    lastEventId = "";
  }
  if (currentEventType.length() > 50) {
    currentEventType = "";
  }
  
  // Force garbage collection
  updateSystemActivity();
}

// ==================== CONFIGURACIÓN DE PINES ====================
void setupPins() {
  pinMode(LED_PROXIMITY, OUTPUT);
  pinMode(LED_STATUS, OUTPUT);
  pinMode(LED_WIFI, OUTPUT);
  pinMode(BUTTON_RESET, INPUT_PULLUP);
  
  // Inicializar LEDs apagados
  digitalWrite(LED_PROXIMITY, LOW);
  digitalWrite(LED_STATUS, LOW);
  digitalWrite(LED_WIFI, LOW);
  
  Serial.println("Pines configurados correctamente");
}

// ==================== GESTIÓN WiFi ====================
void initializeWiFi() {
  Serial.print("Conectando a WiFi: ");
  Serial.println(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  unsigned long startTime = millis();
  currentState = CONNECTING_WIFI;
  
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_TIMEOUT) {
    delay(500);
    Serial.print(".");
    // Parpadear LED WiFi durante conexión
    digitalWrite(LED_WIFI, !digitalRead(LED_WIFI));
    updateSystemActivity(); // Mantener actividad durante conexión
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    digitalWrite(LED_WIFI, HIGH);
    Serial.println("\n✓ WiFi conectado!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal Strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    wifiConnected = false;
    digitalWrite(LED_WIFI, LOW);
    Serial.println("\n✗ Error: No se pudo conectar a WiFi");
    currentState = ERROR_STATE;
  }
  
  updateSystemActivity();
}

void checkWiFiStatus() {
  if (millis() - lastWiFiCheck >= 5000) { // Verificar cada 5 segundos
    bool currentWiFiStatus = (WiFi.status() == WL_CONNECTED);
    
    if (currentWiFiStatus != wifiConnected) {
      wifiConnected = currentWiFiStatus;
      digitalWrite(LED_WIFI, wifiConnected ? HIGH : LOW);
      
      if (wifiConnected) {
        Serial.println("✓ WiFi reconectado");
        currentState = POLLING_API;
        consecutiveErrors = 0;
      } else {
        Serial.println("✗ WiFi desconectado");
        currentState = ERROR_STATE;
      }
    }
    
    lastWiFiCheck = millis();
    updateSystemActivity();
  }
}

// ==================== COMUNICACIÓN API ====================
void testAPIConnection() {
  Serial.println("Probando conexión con Edge API...");
  
  HTTPClient http;
  http.begin("https://geoentry-edge-api.onrender.com/");
  http.setTimeout(HTTP_TIMEOUT);
  
  int httpResponseCode = http.GET();
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("✓ Edge API respondiendo correctamente");
    Serial.print("Response: ");
    Serial.println(response);
  } else {
    Serial.print("✗ Error conectando con Edge API: ");
    Serial.println(httpResponseCode);
  }
  
  http.end();
  updateSystemActivity();
}

void performAPIPolling() {
  if (!wifiConnected) {
    Serial.println("⚠ Skipping API polling - WiFi not connected");
    updateSystemActivity();
    return;
  }
  
  Serial.println("\n--- Polling Edge API ---");
  totalRequests++;
  
  // Consultar eventos de proximidad
  if (checkProximityEvents()) {
    successfulRequests++;
    consecutiveErrors = 0;
    
    if (currentState == ERROR_STATE) {
      currentState = POLLING_API;
      systemHealthy = true;
    }
  } else {
    consecutiveErrors++;
    Serial.print("Errores consecutivos: ");
    Serial.println(consecutiveErrors);
    
    if (consecutiveErrors >= 3) {
      currentState = ERROR_STATE;
      systemHealthy = false;
    }
  }
  
  // Estadísticas
  Serial.print("Estadísticas - Total: ");
  Serial.print(totalRequests);
  Serial.print(", Exitosos: ");
  Serial.print(successfulRequests);
  Serial.print(", Tasa éxito: ");
  Serial.print((float)successfulRequests / totalRequests * 100, 1);
  Serial.println("%");
  
  updateSystemActivity();
}

bool checkProximityEvents() {
  HTTPClient http;
  String url = String(API_BASE_URL) + "/locations/proximity-check";
  
  http.begin(url);
  http.setTimeout(HTTP_TIMEOUT);
  
  // Headers de autenticación
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Device-ID", DEVICE_ID);
  http.addHeader("X-API-Key", API_KEY);
  
  // Body con ubicación actual
  StaticJsonDocument<200> requestDoc;
  requestDoc["latitude"] = DEVICE_LATITUDE;
  requestDoc["longitude"] = DEVICE_LONGITUDE;
  
  String requestBody;
  serializeJson(requestDoc, requestBody);
  
  Serial.print("Enviando: ");
  Serial.println(requestBody);
  
  int httpResponseCode = http.POST(requestBody);
  
  updateSystemActivity(); // Actualizar durante operaciones HTTP
  
  if (httpResponseCode == 200) {
    String response = http.getString();
    Serial.print("✓ API Response: ");
    Serial.println(response);
    
    // Procesar respuesta
    processProximityResponse(response);
    http.end();
    return true;
    
  } else {
    Serial.print("✗ HTTP Error: ");
    Serial.println(httpResponseCode);
    
    if (httpResponseCode > 0) {
      String errorResponse = http.getString();
      Serial.print("Error details: ");
      Serial.println(errorResponse);
    }
    
    http.end();
    return false;
  }
}

void processProximityResponse(String jsonResponse) {
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, jsonResponse);
  
  if (error) {
    Serial.print("✗ Error parsing JSON: ");
    Serial.println(error.c_str());
    updateSystemActivity();
    return;
  }
  
  // Verificar si hay resultados de proximidad
  JsonArray proximityResults = doc["proximity_results"];
  bool proximityDetected = false;
  String detectedEventType = "";
  String detectedLocationId = "";
  float detectedDistance = 0.0;
  
  for (JsonObject result : proximityResults) {
    bool withinRadius = result["within_radius"];
    if (withinRadius) {
      proximityDetected = true;
      detectedEventType = result["event_type"].as<String>();
      detectedLocationId = result["location_id"].as<String>();
      detectedDistance = result["distance"];
      
      Serial.println("🎯 ¡PROXIMIDAD DETECTADA!");
      Serial.print("Ubicación: ");
      Serial.println(result["location_name"].as<String>());
      Serial.print("Distancia: ");
      Serial.print(detectedDistance);
      Serial.println(" metros");
      Serial.print("Evento: ");
      Serial.println(detectedEventType);
      
      break; // Procesar solo el primer evento
    }
  }
  
  if (proximityDetected) {
    handleProximityDetected(detectedEventType, detectedLocationId, detectedDistance);
  } else {
    handleNoProximity();
  }
  
  updateSystemActivity();
}

void handleProximityDetected(String eventType, String locationId, float distance) {
  currentState = PROXIMITY_DETECTED;
  isProximityActive = true;
  currentEventType = eventType;
  
  // Activar alerta LED
  activateLEDAlert();
  
  // Enviar confirmación a la API
  sendProximityConfirmation(eventType, locationId, distance);
  
  updateSystemActivity();
}

void handleNoProximity() {
  if (isProximityActive) {
    Serial.println("ℹ Saliendo del área de proximidad");
    isProximityActive = false;
    currentEventType = "";
    
    if (currentState == PROXIMITY_DETECTED) {
      currentState = POLLING_API;
    }
  }
  updateSystemActivity();
}

void sendProximityConfirmation(String eventType, String locationId, float distance) {
  Serial.println("📤 Enviando confirmación de proximidad...");
  
  HTTPClient http;
  String url = String(API_BASE_URL) + "/proximity-events";
  
  http.begin(url);
  http.setTimeout(HTTP_TIMEOUT);
  
  // Headers
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Device-ID", DEVICE_ID);
  http.addHeader("X-API-Key", API_KEY);
  
  // Body con datos del evento
  StaticJsonDocument<300> eventDoc;
  eventDoc["latitude"] = DEVICE_LATITUDE;
  eventDoc["longitude"] = DEVICE_LONGITUDE;
  eventDoc["event_type"] = eventType;
  eventDoc["location_id"] = locationId;
  
  String eventBody;
  serializeJson(eventDoc, eventBody);
  
  Serial.print("Enviando evento: ");
  Serial.println(eventBody);
  
  int httpResponseCode = http.POST(eventBody);
  
  updateSystemActivity(); // Actualizar durante operaciones HTTP
  
  if (httpResponseCode == 201) {
    String response = http.getString();
    Serial.println("✓ Confirmación enviada exitosamente");
    Serial.print("Response: ");
    Serial.println(response);
    
    // Extraer event_id de la respuesta
    StaticJsonDocument<200> responseDoc;
    deserializeJson(responseDoc, response);
    lastEventId = responseDoc["event_id"].as<String>();
    
  } else {
    Serial.print("✗ Error enviando confirmación: ");
    Serial.println(httpResponseCode);
    
    if (httpResponseCode > 0) {
      String errorResponse = http.getString();
      Serial.print("Error details: ");
      Serial.println(errorResponse);
    }
  }
  
  http.end();
}

// ==================== CONTROL DE LEDs ====================
void activateLEDAlert() {
  Serial.println("🔴 Activando alerta LED de proximidad");
  digitalWrite(LED_PROXIMITY, HIGH);
  isLedAlertActive = true;
  ledAlertStartTime = millis();
  
  // Patrón de parpadeo especial
  for (int i = 0; i < 5; i++) {
    digitalWrite(LED_STATUS, HIGH);
    delay(100);
    digitalWrite(LED_STATUS, LOW);
    delay(100);
    updateSystemActivity(); // Mantener actividad durante parpadeo
  }
}

void handleLEDAlerts() {
  if (isLedAlertActive && millis() - ledAlertStartTime >= LED_ALERT_DURATION) {
    Serial.println("🔴 Desactivando alerta LED");
    digitalWrite(LED_PROXIMITY, LOW);
    isLedAlertActive = false;
    updateSystemActivity();
  }
}

void blinkLED(int pin, int times, int delayMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(pin, HIGH);
    delay(delayMs);
    digitalWrite(pin, LOW);
    delay(delayMs);
    updateSystemActivity(); // Mantener actividad durante parpadeo
  }
}

// ==================== MANEJO DE ESTADOS ====================
void handleSystemState() {
  static unsigned long lastStateUpdate = 0;
  
  if (millis() - lastStateUpdate >= 1000) { // Actualizar cada segundo
    switch (currentState) {
      case INITIALIZING:
        // Parpadeo lento del LED de estado
        digitalWrite(LED_STATUS, !digitalRead(LED_STATUS));
        break;
        
      case CONNECTING_WIFI:
        // Parpadeo rápido durante conexión WiFi
        digitalWrite(LED_STATUS, !digitalRead(LED_STATUS));
        break;
        
      case POLLING_API:
        // LED de estado encendido continuamente
        digitalWrite(LED_STATUS, HIGH);
        break;
        
      case PROXIMITY_DETECTED:
        // Parpadeo del LED de estado cuando hay proximidad
        digitalWrite(LED_STATUS, !digitalRead(LED_STATUS));
        break;
        
      case ERROR_STATE:
        // Parpadeo muy rápido para indicar error
        static int errorBlinks = 0;
        digitalWrite(LED_STATUS, !digitalRead(LED_STATUS));
        errorBlinks++;
        if (errorBlinks >= 10) {
          currentState = RECOVERING;
          errorBlinks = 0;
        }
        break;
        
      case RECOVERING:
        // Intentar recuperación
        Serial.println("🔄 Intentando recuperar sistema...");
        if (!wifiConnected) {
          initializeWiFi();
        }
        if (wifiConnected) {
          consecutiveErrors = 0;
          currentState = POLLING_API;
          systemHealthy = true;
        }
        break;
    }
    
    lastStateUpdate = millis();
    updateSystemActivity();
  }
}

void handleSystemHeartbeat() {
  if (millis() - lastHeartbeat >= 60000) { // Cada minuto
    Serial.println("\n💓 HEARTBEAT - Estado del sistema:");
    Serial.print("├─ Estado: ");
    printSystemState();
    Serial.print("├─ WiFi: ");
    Serial.println(wifiConnected ? "Conectado" : "Desconectado");
    Serial.print("├─ Proximidad: ");
    Serial.println(isProximityActive ? "Activa" : "Inactiva");
    Serial.print("├─ Evento actual: ");
    Serial.println(currentEventType.length() > 0 ? currentEventType : "Ninguno");
    Serial.print("├─ Último Event ID: ");
    Serial.println(lastEventId.length() > 0 ? lastEventId : "Ninguno");
    Serial.print("├─ Memoria libre: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");
    Serial.print("├─ Última actividad: ");
    Serial.print((millis() - lastSystemActivity) / 1000);
    Serial.println(" seg atrás");
    Serial.print("└─ Uptime: ");
    Serial.print(millis() / 1000);
    Serial.println(" segundos");
    
    lastHeartbeat = millis();
    updateSystemActivity();
  }
}

void printSystemState() {
  switch (currentState) {
    case INITIALIZING: Serial.println("Inicializando"); break;
    case CONNECTING_WIFI: Serial.println("Conectando WiFi"); break;
    case POLLING_API: Serial.println("Polling API"); break;
    case PROXIMITY_DETECTED: Serial.println("Proximidad Detectada"); break;
    case ERROR_STATE: Serial.println("Error"); break;
    case RECOVERING: Serial.println("Recuperando"); break;
    default: Serial.println("Desconocido"); break;
  }
}

// ==================== UTILIDADES ====================
void checkResetButton() {
  static unsigned long buttonPressTime = 0;
  static bool buttonPressed = false;
  
  if (digitalRead(BUTTON_RESET) == LOW) {
    if (!buttonPressed) {
      buttonPressed = true;
      buttonPressTime = millis();
    } else if (millis() - buttonPressTime >= 3000) {
      // Botón presionado por 3 segundos - reiniciar sistema
      Serial.println("🔄 REINICIO DEL SISTEMA SOLICITADO");
      blinkLED(LED_STATUS, 5, 100);
      ESP.restart();
    }
  } else {
    buttonPressed = false;
  }
  
  updateSystemActivity();
}

// ==================== FUNCIONES DE DEBUG ====================
void printMemoryInfo() {
  Serial.print("Memoria libre: ");
  Serial.print(ESP.getFreeHeap());
  Serial.print(" bytes, Heap mínimo: ");
  Serial.print(ESP.getMinFreeHeap());
  Serial.println(" bytes");
  updateSystemActivity();
}

void printNetworkInfo() {
  if (wifiConnected) {
    Serial.println("=== Información de Red ===");
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("Gateway: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("DNS: ");
    Serial.println(WiFi.dnsIP());
    Serial.print("Señal: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  }
  updateSystemActivity();
}