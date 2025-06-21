#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ============================================
// CONFIGURACIÓN DE PINES
// ============================================
#define LED_MAIN_PIN     2   // LED rojo - Luces principales
#define LED_STATUS_PIN   4   // LED verde - Estado del sistema
#define LED_WIFI_PIN     5   // LED azul - Estado WiFi
#define BUZZER_PIN      19   // Buzzer para confirmaciones
#define BUTTON_PIN      13   // Botón manual
#define TEST_BTN_PIN    12   // Botón de test proximidad

// ============================================
// CONFIGURACIÓN DE RED
// ============================================
const char* ssid = "Wokwi-GUEST";
const char* password = "";
const int HTTP_PORT = 80;

// ============================================
// CONFIGURACIÓN DE API EXTERNA
// ============================================
const char* API_BASE_URL = "https://geoentry-edge-api.onrender.com/api/v1";
const char* PROXIMITY_CHECK_ENDPOINT = "/locations/proximity-check";
const char* PROXIMITY_EVENTS_ENDPOINT = "/proximity-events";
const int API_TIMEOUT = 10000; // 10 segundos timeout
const int API_RETRY_ATTEMPTS = 3;

// ============================================
// ESTRUCTURA DE ESTADO DEL SISTEMA
// ============================================
struct SystemState {
  bool lightsOn = false;
  bool userNearby = false;
  float lastDistance = 0;
  String lastUserId = "";
  unsigned long lastUpdate = 0;
  int totalCommands = 0;
  bool systemActive = true;
  
  // Información adicional para API
  String deviceId = "esp32-hub-001";
  String locationId = "home-location-001";
  float deviceLatitude = 40.7128;  // Coordenadas de ejemplo - actualizar según ubicación real
  float deviceLongitude = -74.0060;
  bool apiConnected = false;
  unsigned long lastAPICall = 0;
  int failedAPICallsCount = 0;
}; // Fixed: Added semicolon

// ============================================
// VARIABLES GLOBALES
// ============================================
WebServer server(HTTP_PORT);
SystemState state; // Fixed: Moved here

// ============================================
// DECLARACIONES DE FUNCIONES
// ============================================
void printWelcomeBanner();
void initializePins();
void connectToWiFi();
void setupWebServer();
void handleCORS();
void handleProximityCommand();
void handleGetStatus();
void handleManualControl();
void handleSystemReset();
void handleHealthCheck();
void handleTestAPI();
void handleDashboard();
void handleNotFound();
void activateWelcomeSequence();
void activateGoodbyeSequence();
void setMainLights(bool on);
void setStatusLED(bool on);
void updateWiFiStatus();
void playStartupSound();
void playWelcomeSound();
void playGoodbyeSound();
void checkManualButton();
void checkTestButton();
void simulateProximityTest();
void simulateProximityEvent();
void simulateArrival();
void simulateDeparture();
void sendErrorResponse(int code, String message);
String generateResponseMessage(String action);
String generateDashboardHTML();

// ============================================
// FUNCIONES DE API EXTERNA
// ============================================
bool sendProximityCheck(String userId, String locationId, float latitude, float longitude);
bool sendProximityEvent(String userId, String locationId, bool isNear, float distance);
String makeHTTPRequest(String endpoint, String jsonData, String method = "POST");
bool isValidAPIResponse(String response);
void handleAPIError(String endpoint, String error);
void testAPIConnection();

// ============================================
// FUNCIÓN DE TEST AUTOMÁTICO
// ============================================
void simulateProximityTest() {
  // Simular llegada del usuario
  Serial.println("🎯 === SIMULATION: User arriving home ===");
  state.userNearby = true;
  state.lastDistance = 4.5;
  state.lastUserId = "test-user-001";
  state.lastUpdate = millis();
  state.totalCommands++;
  
  activateWelcomeSequence();
  
  Serial.println("✅ Simulation complete - lights should be ON");
  Serial.printf("💡 Current lights status: %s\n", state.lightsOn ? "ON" : "OFF");
  Serial.printf("👤 User nearby: %s\n", state.userNearby ? "YES" : "NO");
  Serial.printf("📊 Total commands processed: %d\n", state.totalCommands);
}

// ============================================
// CONFIGURACIÓN INICIAL
// ============================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  printWelcomeBanner();
  initializePins();
  connectToWiFi();
  setupWebServer();
  
  // Test inicial de conexión con API externa
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("🔍 Testing initial API connection...");
    delay(2000); // Dar tiempo para que la conexión se estabilice
    testAPIConnection();
  } else {
    Serial.println("⚠️ Skipping API test - WiFi not connected");
  }
  
  Serial.println("✅ GeoEntry ESP32 Hub READY!");
  Serial.printf("📡 Waiting for commands from mobile app...\n");
  Serial.printf("🌐 External API: %s\n", API_BASE_URL);
  Serial.printf("📍 Device Location: %.6f, %.6f\n", state.deviceLatitude, state.deviceLongitude);
  
  // Señal de inicialización completa
  playStartupSound();
  setStatusLED(true);
}

// ============================================
// LOOP PRINCIPAL
// ============================================
void loop() {
  server.handleClient();
  
  // Verificar botón manual
  static unsigned long lastButtonCheck = 0;
  if (millis() - lastButtonCheck > 100) {
    checkManualButton();
    checkTestButton();
    lastButtonCheck = millis();
  }
  
  // Actualizar LED WiFi según conexión
  static unsigned long lastWiFiCheck = 0;
  if (millis() - lastWiFiCheck > 5000) {
    updateWiFiStatus();
    lastWiFiCheck = millis();
  }
  
  // Test de conectividad API cada 5 minutos
  static unsigned long lastAPIHealthCheck = 0;
  if (millis() - lastAPIHealthCheck > 300000) { // 5 minutos
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("🔍 Performing periodic API health check...");
      testAPIConnection();
    }
    lastAPIHealthCheck = millis();
  }
  
  // Test automático cada 30 segundos (opcional - para desarrollo)
  static unsigned long lastAutoTest = 0;
  if (millis() - lastAutoTest > 30000) {
    Serial.println("🧪 Auto-testing proximity simulation...");
    simulateProximityTest();
    lastAutoTest = millis();
  }
  
  delay(10);
}

// ============================================
// INICIALIZACIÓN
// ============================================
void printWelcomeBanner() {
  Serial.println("========================================");
  Serial.println("🏠 GeoEntry ESP32 Smart Hub v1.0");
  Serial.println("   Developed by TechProtection");
  Serial.println("========================================");
}

void initializePins() {
  Serial.println("🔧 Initializing GPIO pins...");
  
  pinMode(LED_MAIN_PIN, OUTPUT);
  pinMode(LED_STATUS_PIN, OUTPUT);
  pinMode(LED_WIFI_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(TEST_BTN_PIN, INPUT_PULLUP);
  
  // Estado inicial - todo apagado
  digitalWrite(LED_MAIN_PIN, LOW);
  digitalWrite(LED_STATUS_PIN, LOW);
  digitalWrite(LED_WIFI_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  
  Serial.println("✅ GPIO pins configured");
}

void connectToWiFi() {
  Serial.printf("📶 Connecting to WiFi: %s\n", ssid);
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
    
    // Parpadeo durante conexión
    digitalWrite(LED_WIFI_PIN, !digitalRead(LED_WIFI_PIN));
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_WIFI_PIN, HIGH);
    Serial.println("\n✅ WiFi Connected Successfully!");
    Serial.printf("📍 IP Address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("🌐 Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
    Serial.printf("📡 RSSI: %d dBm\n", WiFi.RSSI());
  } else {
    digitalWrite(LED_WIFI_PIN, LOW);
    Serial.println("\n❌ WiFi Connection Failed!");
    Serial.println("🔄 System will continue in offline mode");
  }
}

// ============================================
// CONFIGURACIÓN DEL SERVIDOR WEB
// ============================================
void setupWebServer() {
  Serial.println("🌐 Setting up web server...");
  
  // Endpoint principal para comandos de proximidad
  server.on("/api/proximity", HTTP_POST, handleProximityCommand);
  server.on("/api/proximity", HTTP_OPTIONS, handleCORS);
  
  // Endpoints de estado y control
  server.on("/api/status", HTTP_GET, handleGetStatus);
  server.on("/api/manual", HTTP_POST, handleManualControl);
  server.on("/api/reset", HTTP_POST, handleSystemReset);
  
  // Health check
  server.on("/api/health", HTTP_GET, handleHealthCheck);
  
  // Test de conexión con API externa
  server.on("/api/test-api", HTTP_POST, handleTestAPI);
  
  // Dashboard web simple
  server.on("/", HTTP_GET, handleDashboard);
  
  // Manejo de rutas no encontradas
  server.onNotFound(handleNotFound);
  
  server.begin();
  Serial.printf("✅ Web server started on port %d\n", HTTP_PORT);
  Serial.println("📋 Available endpoints:");
  Serial.println("   POST /api/proximity - Receive proximity commands");
  Serial.println("   GET  /api/status    - Get system status");
  Serial.println("   POST /api/manual    - Manual control");
  Serial.println("   POST /api/test-api  - Test external API connection");
  Serial.println("   GET  /api/health    - Health check");
  Serial.println("   GET  /            - Web dashboard");
}

// ============================================
// HANDLERS DE ENDPOINTS
// ============================================
void handleCORS() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.send(200, "text/plain", "");
}

void handleProximityCommand() {
  Serial.println("📡 === PROXIMITY COMMAND RECEIVED ===");
  
  // Verificar que hay datos
  if (!server.hasArg("plain")) {
    Serial.println("❌ No JSON data received");
    sendErrorResponse(400, "Missing JSON data");
    return;
  }
  
  String requestBody = server.arg("plain");
  Serial.printf("📥 Raw data: %s\n", requestBody.c_str());
  
  // Parsear JSON
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, requestBody);
  
  if (error) {
    Serial.printf("❌ JSON parse error: %s\n", error.c_str());
    sendErrorResponse(400, "Invalid JSON format");
    return;
  }
  
  // Extraer datos del comando
  String userId = doc["userId"] | "unknown";
  String locationId = doc["locationId"] | state.locationId;
  bool isNear = doc["isNear"] | false;
  float distance = doc["distance"] | 0.0;
  float latitude = doc["latitude"] | state.deviceLatitude;
  float longitude = doc["longitude"] | state.deviceLongitude;
  unsigned long timestamp = doc["timestamp"] | millis();
  
  // Log de información recibida
  Serial.printf("👤 User ID: %s\n", userId.c_str());
  Serial.printf("📍 Location: %s\n", locationId.c_str());
  Serial.printf("🎯 Is Near: %s\n", isNear ? "YES" : "NO");
  Serial.printf("📏 Distance: %.1f meters\n", distance);
  Serial.printf("🌍 Coordinates: %.6f, %.6f\n", latitude, longitude);
  
  // Enviar datos a la API externa
  bool proximityCheckSent = false;
  bool proximityEventSent = false;
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("🌐 Sending data to external API...");
    
    // Enviar proximity check
    proximityCheckSent = sendProximityCheck(userId, locationId, latitude, longitude);
    
    // Enviar proximity event
    proximityEventSent = sendProximityEvent(userId, locationId, isNear, distance);
    
    state.apiConnected = proximityCheckSent && proximityEventSent;
    state.lastAPICall = millis();
    
    if (!state.apiConnected) {
      state.failedAPICallsCount++;
    } else {
      state.failedAPICallsCount = 0; // Reset counter on success
    }
  } else {
    Serial.println("⚠️ WiFi not connected - skipping API calls");
    state.apiConnected = false;
    state.failedAPICallsCount++;
  }
  
  // Actualizar estado del sistema
  bool previousState = state.userNearby;
  state.userNearby = isNear;
  state.lastDistance = distance;
  state.lastUserId = userId;
  state.lastUpdate = millis();
  state.totalCommands++;
  
  // Determinar acción a realizar
  String action = "";
  if (isNear && !previousState) {
    // Usuario LLEGÓ - Encender luces
    action = "WELCOME_HOME";
    activateWelcomeSequence();
  } else if (!isNear && previousState) {
    // Usuario SE FUE - Apagar luces
    action = "GOODBYE";
    activateGoodbyeSequence();
  } else if (isNear) {
    // Usuario sigue cerca - mantener estado
    action = "STAY_HOME";
    Serial.println("🏠 User still nearby - maintaining current state");
  } else {
    // Usuario sigue lejos - no hacer nada
    action = "STAY_AWAY";
    Serial.println("🚗 User still away - no action needed");
  }
  
  // Respuesta de confirmación al mobile app
  DynamicJsonDocument response(512);
  response["status"] = "success";
  response["action"] = action;
  response["lightsOn"] = state.lightsOn;
  response["timestamp"] = millis();
  response["deviceId"] = state.deviceId;
  response["message"] = generateResponseMessage(action);
  response["apiConnected"] = state.apiConnected;
  response["proximityCheckSent"] = proximityCheckSent;
  response["proximityEventSent"] = proximityEventSent;
  
  String responseJson;
  serializeJson(response, responseJson);
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", responseJson);
  
  Serial.printf("✅ Response sent: %s\n", action.c_str());
  Serial.printf("🌐 API Status - Check: %s, Event: %s\n", 
                proximityCheckSent ? "✅" : "❌", 
                proximityEventSent ? "✅" : "❌");
  Serial.println("================================");
}

void handleGetStatus() {
  Serial.println("📊 Status request received");
  
  DynamicJsonDocument status(1024);
  status["deviceId"] = state.deviceId;
  status["locationId"] = state.locationId;
  status["lightsOn"] = state.lightsOn;
  status["userNearby"] = state.userNearby;
  status["systemActive"] = state.systemActive;
  status["lastDistance"] = state.lastDistance;
  status["lastUserId"] = state.lastUserId;
  status["lastUpdate"] = state.lastUpdate;
  status["totalCommands"] = state.totalCommands;
  status["uptime"] = millis();
  status["wifiConnected"] = WiFi.status() == WL_CONNECTED;
  status["wifiRSSI"] = WiFi.RSSI();
  status["freeHeap"] = ESP.getFreeHeap();
  
  // Información adicional de API
  status["apiConnected"] = state.apiConnected;
  status["lastAPICall"] = state.lastAPICall;
  status["failedAPICallsCount"] = state.failedAPICallsCount;
  status["deviceLatitude"] = state.deviceLatitude;
  status["deviceLongitude"] = state.deviceLongitude;
  status["apiBaseURL"] = API_BASE_URL;
  
  String statusJson;
  serializeJson(status, statusJson);
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", statusJson);
}

void handleManualControl() {
  Serial.println("🎮 Manual control command received");
  
  if (!server.hasArg("plain")) {
    sendErrorResponse(400, "Missing command data");
    return;
  }
  
  String requestBody = server.arg("plain");
  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, requestBody);
  
  if (error) {
    sendErrorResponse(400, "Invalid JSON");
    return;
  }
  
  String command = doc["command"] | "";
  
  if (command == "LIGHTS_ON") {
    setMainLights(true);
    Serial.println("💡 Manual: Lights ON");
  } else if (command == "LIGHTS_OFF") {
    setMainLights(false);
    Serial.println("💡 Manual: Lights OFF");
  } else if (command == "TOGGLE_LIGHTS") {
    setMainLights(!state.lightsOn);
    Serial.printf("💡 Manual: Lights %s\n", state.lightsOn ? "ON" : "OFF");
  } else {
    sendErrorResponse(400, "Unknown command");
    return;
  }
  
  // Respuesta
  DynamicJsonDocument response(256);
  response["status"] = "success";
  response["command"] = command;
  response["lightsOn"] = state.lightsOn;
  
  String responseJson;
  serializeJson(response, responseJson);
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", responseJson);
}

void handleSystemReset() {
  Serial.println("🔄 System reset requested");
  
  // Reset estado
  state.lightsOn = false;
  state.userNearby = false;
  state.lastDistance = 0;
  state.lastUserId = "";
  state.totalCommands = 0;
  state.systemActive = true;
  
  // Reset hardware
  setMainLights(false);
  setStatusLED(true);
  
  playStartupSound();
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", "{\"status\":\"reset_complete\"}");
  
  Serial.println("✅ System reset completed");
}

void handleHealthCheck() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", "{\"status\":\"healthy\",\"timestamp\":" + String(millis()) + "}");
}

void handleDashboard() {
  String html = generateDashboardHTML();
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/html", html);
}

void handleNotFound() {
  Serial.printf("⚠️  404 - Not found: %s\n", server.uri().c_str());
  sendErrorResponse(404, "Endpoint not found");
}

// ============================================
// CONTROL DE DISPOSITIVOS
// ============================================
void activateWelcomeSequence() {
  Serial.println("🎉 === WELCOME HOME SEQUENCE ===");
  
  // Encender luces principales
  setMainLights(true);
  
  // Secuencia de bienvenida con LEDs
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_STATUS_PIN, HIGH);
    delay(200);
    digitalWrite(LED_STATUS_PIN, LOW);
    delay(200);
  }
  digitalWrite(LED_STATUS_PIN, HIGH);
  
  // Sonido de bienvenida
  playWelcomeSound();
  
  Serial.println("✅ Welcome sequence completed");
}

void activateGoodbyeSequence() {
  Serial.println("👋 === GOODBYE SEQUENCE ===");
  
  // Apagar luces principales después de un delay
  delay(2000); // Esperar 2 segundos
  
  if (!state.userNearby) { // Verificar que sigue lejos
    setMainLights(false);
    
    // Sonido de despedida
    playGoodbyeSound();
    
    Serial.println("✅ Goodbye sequence completed");
  }
}

void setMainLights(bool on) {
  digitalWrite(LED_MAIN_PIN, on ? HIGH : LOW);
  state.lightsOn = on;
  
  Serial.printf("💡 Main lights: %s\n", on ? "ON" : "OFF");
}

void setStatusLED(bool on) {
  digitalWrite(LED_STATUS_PIN, on ? HIGH : LOW);
}

void updateWiFiStatus() {
  bool connected = WiFi.status() == WL_CONNECTED;
  digitalWrite(LED_WIFI_PIN, connected ? HIGH : LOW);
  
  if (!connected) {
    Serial.println("⚠️  WiFi connection lost, attempting reconnect...");
    WiFi.reconnect();
  }
}

// ============================================
// SONIDOS Y FEEDBACK
// ============================================
void playStartupSound() {
  int melody[] = {262, 294, 330, 349, 392}; // C, D, E, F, G
  for (int i = 0; i < 5; i++) {
    tone(BUZZER_PIN, melody[i], 150);
    delay(200);
    noTone(BUZZER_PIN);
  }
}

void playWelcomeSound() {
  // Melodía ascendente de bienvenida
  int melody[] = {523, 587, 659, 698, 784}; // C5, D5, E5, F5, G5
  for (int i = 0; i < 5; i++) {
    tone(BUZZER_PIN, melody[i], 200);
    delay(250);
    noTone(BUZZER_PIN);
  }
}

void playGoodbyeSound() {
  // Melodía descendente de despedida
  int melody[] = {784, 698, 659, 587, 523}; // G5, F5, E5, D5, C5
  for (int i = 0; i < 5; i++) {
    tone(BUZZER_PIN, melody[i], 200);
    delay(250);
    noTone(BUZZER_PIN);
  }
}

void checkManualButton() {
  static bool lastButtonState = HIGH;
  static unsigned long lastDebounceTime = 0;
  
  bool buttonState = digitalRead(BUTTON_PIN);
  
  if (buttonState != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > 50) {
    if (buttonState == LOW) { // Botón presionado
      Serial.println("🔘 Manual button pressed - toggling lights");
      setMainLights(!state.lightsOn);
      
      // Feedback sonoro corto
      tone(BUZZER_PIN, 1000, 100);
      delay(150);
      noTone(BUZZER_PIN);
    }
  }
  
  lastButtonState = buttonState;
}

void checkTestButton() {
  static bool lastTestButtonState = HIGH;
  static unsigned long lastDebounceTime = 0;
  
  bool buttonState = digitalRead(TEST_BTN_PIN);
  
  if (buttonState != lastTestButtonState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > 50) {
    if (buttonState == LOW) { // Botón presionado
      Serial.println("🧪 === TEST BUTTON PRESSED ===");
      simulateProximityEvent();
      
      // Feedback sonoro
      tone(BUZZER_PIN, 1500, 200);
      delay(250);
      noTone(BUZZER_PIN);
    }
  }
  
  lastTestButtonState = buttonState;
}

void simulateProximityEvent() {
  static bool eventType = true; // true = arrival, false = departure
  
  if (eventType) {
    Serial.println("🏠 Manual Test: Simulating USER ARRIVAL");
    simulateArrival();
  } else {
    Serial.println("🚗 Manual Test: Simulating USER DEPARTURE");
    simulateDeparture(); 
  }
  
  eventType = !eventType;
}

void simulateArrival() {
  bool previousState = state.userNearby;
  state.userNearby = true;
  state.lastDistance = 3.5;
  state.lastUserId = "test-user-001";
  state.lastUpdate = millis();
  state.totalCommands++;
  
  Serial.printf("👤 User ID: %s\n", state.lastUserId.c_str());
  Serial.printf("📍 Location: %s\n", state.locationId.c_str());
  Serial.printf("🎯 Is Near: YES\n");
  Serial.printf("📏 Distance: %.1f meters\n", state.lastDistance);
  
  // Enviar datos a la API externa
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("🌐 Sending simulation data to API...");
    sendProximityCheck(state.lastUserId, state.locationId, state.deviceLatitude, state.deviceLongitude);
    delay(500); // Pequeña pausa entre llamadas
    sendProximityEvent(state.lastUserId, state.locationId, true, state.lastDistance);
  }
  
  if (!previousState) {
    activateWelcomeSequence();
    Serial.println("✅ Welcome sequence activated!");
  } else {
    Serial.println("🏠 User already home - no action needed");
  }
}

void simulateDeparture() {
  bool previousState = state.userNearby;
  state.userNearby = false;
  state.lastDistance = 25.0;
  state.lastUserId = "test-user-001";
  state.lastUpdate = millis();
  state.totalCommands++;
  
  Serial.printf("👤 User ID: %s\n", state.lastUserId.c_str());
  Serial.printf("📍 Location: %s\n", state.locationId.c_str());
  Serial.printf("🎯 Is Near: NO\n");
  Serial.printf("📏 Distance: %.1f meters\n", state.lastDistance);
  
  // Enviar datos a la API externa
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("🌐 Sending simulation data to API...");
    sendProximityCheck(state.lastUserId, state.locationId, state.deviceLatitude, state.deviceLongitude);
    delay(500); // Pequeña pausa entre llamadas
    sendProximityEvent(state.lastUserId, state.locationId, false, state.lastDistance);
  }
  
  if (previousState) {
    activateGoodbyeSequence();
    Serial.println("✅ Goodbye sequence activated!");
  } else {
    Serial.println("🚗 User already away - no action needed");
  }
}

// ============================================
// FUNCIONES DE API EXTERNA - GEOENTRY
// ============================================

bool sendProximityCheck(String userId, String locationId, float latitude, float longitude) {
  Serial.println("🌐 Sending proximity check to API...");
  
  DynamicJsonDocument requestDoc(512);
  requestDoc["userId"] = userId;
  requestDoc["locationId"] = locationId;
  requestDoc["deviceId"] = state.deviceId;
  requestDoc["latitude"] = latitude;
  requestDoc["longitude"] = longitude;
  requestDoc["timestamp"] = millis();
  requestDoc["deviceType"] = "ESP32_HUB";
  
  String jsonData;
  serializeJson(requestDoc, jsonData);
  
  Serial.printf("📤 Proximity Check Data: %s\n", jsonData.c_str());
  
  String response = makeHTTPRequest(PROXIMITY_CHECK_ENDPOINT, jsonData, "POST");
  bool success = isValidAPIResponse(response);
  
  if (success) {
    Serial.println("✅ Proximity check sent successfully");
  } else {
    Serial.println("❌ Failed to send proximity check");
    handleAPIError(PROXIMITY_CHECK_ENDPOINT, response);
  }
  
  return success;
}

bool sendProximityEvent(String userId, String locationId, bool isNear, float distance) {
  Serial.println("🌐 Sending proximity event to API...");
  
  DynamicJsonDocument requestDoc(512);
  requestDoc["userId"] = userId;
  requestDoc["locationId"] = locationId;
  requestDoc["deviceId"] = state.deviceId;
  requestDoc["isNear"] = isNear;
  requestDoc["distance"] = distance;
  requestDoc["timestamp"] = millis();
  requestDoc["eventType"] = isNear ? "ARRIVAL" : "DEPARTURE";
  requestDoc["deviceType"] = "ESP32_HUB";
  requestDoc["lightStatus"] = state.lightsOn;
  
  String jsonData;
  serializeJson(requestDoc, jsonData);
  
  Serial.printf("📤 Proximity Event Data: %s\n", jsonData.c_str());
  
  String response = makeHTTPRequest(PROXIMITY_EVENTS_ENDPOINT, jsonData, "POST");
  bool success = isValidAPIResponse(response);
  
  if (success) {
    Serial.println("✅ Proximity event sent successfully");
  } else {
    Serial.println("❌ Failed to send proximity event");
    handleAPIError(PROXIMITY_EVENTS_ENDPOINT, response);
  }
  
  return success;
}

String makeHTTPRequest(String endpoint, String jsonData, String method) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi not connected for API call");
    return "ERROR: NO_WIFI";
  }
  
  HTTPClient http;
  http.setTimeout(API_TIMEOUT);
  
  String fullURL = String(API_BASE_URL) + endpoint;
  Serial.printf("🌍 Making %s request to: %s\n", method.c_str(), fullURL.c_str());
  
  http.begin(fullURL);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("User-Agent", "ESP32-GeoEntry-Hub/1.0");
  
  // Agregar headers adicionales si son necesarios
  // http.addHeader("Authorization", "Bearer YOUR_API_KEY"); // Descomentar si se requiere autenticación
  
  int httpResponseCode = -1;
  String response = "";
  
  // Realizar la petición HTTP
  if (method == "POST") {
    httpResponseCode = http.POST(jsonData);
  } else if (method == "GET") {
    httpResponseCode = http.GET();
  } else if (method == "PUT") {
    httpResponseCode = http.PUT(jsonData);
  }
  
  if (httpResponseCode > 0) {
    response = http.getString();
    Serial.printf("📥 HTTP Response Code: %d\n", httpResponseCode);
    Serial.printf("📥 Response: %s\n", response.c_str());
    
    if (httpResponseCode >= 200 && httpResponseCode < 300) {
      Serial.println("✅ API call successful");
    } else {
      Serial.printf("⚠️ API returned error code: %d\n", httpResponseCode);
    }
  } else {
    Serial.printf("❌ HTTP Request failed, error: %s\n", http.errorToString(httpResponseCode).c_str());
    response = "ERROR: HTTP_" + String(httpResponseCode);
  }
  
  http.end();
  return response;
}

bool isValidAPIResponse(String response) {
  if (response.startsWith("ERROR:")) {
    return false;
  }
  
  // Intentar parsear como JSON para validar
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, response);
  
  if (error) {
    Serial.printf("⚠️ API response is not valid JSON: %s\n", error.c_str());
    return !response.isEmpty(); // Si no es JSON pero hay respuesta, considerarlo válido
  }
  
  // Verificar si hay un campo de estado o error en la respuesta
  if (doc.containsKey("error")) {
    Serial.printf("❌ API returned error: %s\n", doc["error"].as<String>().c_str());
    return false;
  }
  
  if (doc.containsKey("status")) {
    String status = doc["status"];
    if (status == "error" || status == "failed") {
      Serial.printf("❌ API returned error status: %s\n", status.c_str());
      return false;
    }
  }
  
  return true;
}

void handleAPIError(String endpoint, String error) {
  Serial.printf("🚨 API Error on endpoint %s: %s\n", endpoint.c_str(), error.c_str());
  
  // Incrementar contador de errores
  state.failedAPICallsCount++;
  
  // Si hay muchos errores consecutivos, podrías implementar alguna lógica
  if (state.failedAPICallsCount > 10) {
    Serial.println("⚠️ Too many API failures - consider checking network or API status");
    
    // Opcional: parpadear LED de estado para indicar problema
    for (int i = 0; i < 3; i++) {
      digitalWrite(LED_STATUS_PIN, LOW);
      delay(200);
      digitalWrite(LED_STATUS_PIN, HIGH);
      delay(200);
    }
  }
  
  // Log del error para debugging
  Serial.printf("📊 Total API failures: %d\n", state.failedAPICallsCount);
}

// ============================================
// FUNCIÓN DE TEST CON API EXTERNA
// ============================================
void testAPIConnection() {
  Serial.println("🧪 === TESTING API CONNECTION ===");
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ Cannot test API - WiFi not connected");
    return;
  }
  
  // Test con datos de ejemplo
  String testUserId = "test-user-esp32";
  String testLocationId = state.locationId;
  float testLatitude = state.deviceLatitude;
  float testLongitude = state.deviceLongitude;
  
  Serial.println("🔄 Testing proximity check endpoint...");
  bool proximityCheckTest = sendProximityCheck(testUserId, testLocationId, testLatitude, testLongitude);
  
  delay(1000); // Esperar entre llamadas
  
  Serial.println("🔄 Testing proximity event endpoint...");
  bool proximityEventTest = sendProximityEvent(testUserId, testLocationId, true, 5.0);
  
  Serial.printf("📊 API Test Results:\n");
  Serial.printf("   Proximity Check: %s\n", proximityCheckTest ? "✅ PASS" : "❌ FAIL");
  Serial.printf("   Proximity Event: %s\n", proximityEventTest ? "✅ PASS" : "❌ FAIL");
  
  if (proximityCheckTest && proximityEventTest) {
    Serial.println("🎉 All API tests passed!");
    state.apiConnected = true;
    
    // Sonido de éxito
    tone(BUZZER_PIN, 1000, 200);
    delay(250);
    tone(BUZZER_PIN, 1200, 200);
    delay(250);
    noTone(BUZZER_PIN);
  } else {
    Serial.println("❌ Some API tests failed");
    state.apiConnected = false;
    
    // Sonido de error
    tone(BUZZER_PIN, 400, 500);
    delay(550);
    noTone(BUZZER_PIN);
  }
}

// ============================================
// UTILIDADES
// ============================================
String generateResponseMessage(String action) {
  if (action == "WELCOME_HOME") {
    return "Welcome home! Lights activated automatically.";
  } else if (action == "GOODBYE") {
    return "Goodbye! Lights turned off to save energy.";
  } else if (action == "STAY_HOME") {
    return "You're still home. System maintaining current state.";
  } else {
    return "Command processed successfully.";
  }
}

void sendErrorResponse(int code, String message) {
  DynamicJsonDocument error(256);
  error["status"] = "error";
  error["code"] = code;
  error["message"] = message;
  error["timestamp"] = millis();
  
  String errorJson;
  serializeJson(error, errorJson);
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(code, "application/json", errorJson);
  
  Serial.printf("❌ Error response sent: %d - %s\n", code, message.c_str());
}

String generateDashboardHTML() {
  return R"HTML(
<!DOCTYPE html>
<html>
<head>
    <title>GeoEntry ESP32 Hub</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body { 
            font-family: Arial; 
            margin: 20px; 
            background: #f0f0f0;
            color: #333;
        }
        .container { 
            max-width: 600px; 
            margin: 0 auto; 
            background: white; 
            padding: 20px; 
            border-radius: 10px; 
            box-shadow: 0 4px 15px rgba(0,0,0,0.1);
        }
        .header { 
            text-align: center; 
            color: #2c3e50; 
            margin-bottom: 30px;
        }
        .status-card { 
            background: #ecf0f1; 
            padding: 15px; 
            margin: 10px 0; 
            border-radius: 8px; 
            border-left: 4px solid #3498db;
        }
        .btn { 
            padding: 12px 25px; 
            margin: 8px; 
            border: none; 
            border-radius: 6px; 
            font-size: 14px; 
            cursor: pointer; 
            color: white;
        }
        .btn-primary { background: #3498db; }
        .btn-success { background: #27ae60; }
        .btn-danger { background: #e74c3c; }
        .status-indicator {
            width: 12px;
            height: 12px;
            border-radius: 50%;
            display: inline-block;
            margin-right: 8px;
        }
        .online { background: #27ae60; }
        .offline { background: #e74c3c; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>GeoEntry ESP32 Hub</h1>
            <p>Smart Home Automation Controller</p>
        </div>
        
        <div class="status-card">
            <h3>System Status</h3>
            <p><span class="status-indicator online"></span><strong>Device:</strong> Online</p>
            <p><strong>Lights:</strong> <span id="lightsStatus">Loading...</span></p>
            <p><strong>User:</strong> <span id="userStatus">Loading...</span></p>
            <p><strong>WiFi:</strong> <span id="wifiStatus">Loading...</span></p>
            <p><strong>External API:</strong> <span id="apiStatus">Loading...</span></p>
            <p><strong>Uptime:</strong> <span id="uptime">Loading...</span></p>
        </div>
        
        <div class="status-card">
            <h3>Manual Controls</h3>
            <button class="btn btn-success" onclick="sendCommand('LIGHTS_ON')">Turn Lights ON</button>
            <button class="btn btn-danger" onclick="sendCommand('LIGHTS_OFF')">Turn Lights OFF</button>
            <button class="btn btn-primary" onclick="sendCommand('TOGGLE_LIGHTS')">Toggle Lights</button>
            <button class="btn btn-primary" onclick="testAPI()">Test API Connection</button>
        </div>
        
        <div class="status-card">
            <h3>Statistics</h3>
            <p><strong>Last Distance:</strong> <span id="lastDistance">-</span> meters</p>
            <p><strong>Last User:</strong> <span id="lastUser">-</span></p>
            <p><strong>Total Commands:</strong> <span id="totalCommands">-</span></p>
            <p><strong>API Failures:</strong> <span id="apiFails">-</span></p>
            <p><strong>Location:</strong> <span id="locationInfo">-</span></p>
        </div>
    </div>

    <script>
        function updateStatus() {
            fetch('/api/status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('lightsStatus').textContent = data.lightsOn ? 'ON' : 'OFF';
                    document.getElementById('userStatus').textContent = data.userNearby ? 'Home' : 'Away';
                    document.getElementById('wifiStatus').textContent = data.wifiConnected ? 'Connected' : 'Disconnected';
                    document.getElementById('apiStatus').textContent = data.apiConnected ? 'Connected' : 'Disconnected';
                    document.getElementById('uptime').textContent = Math.floor(data.uptime / 1000) + ' seconds';
                    document.getElementById('lastDistance').textContent = data.lastDistance.toFixed(1);
                    document.getElementById('lastUser').textContent = data.lastUserId || 'None';
                    document.getElementById('totalCommands').textContent = data.totalCommands;
                    document.getElementById('apiFails').textContent = data.failedAPICallsCount || 0;
                    document.getElementById('locationInfo').textContent = data.locationId + ' (' + data.deviceLatitude.toFixed(4) + ', ' + data.deviceLongitude.toFixed(4) + ')';
                })
                .catch(error => {
                    console.error('Error fetching status:', error);
                });
        }
        
        function sendCommand(command) {
            fetch('/api/manual', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({command: command})
            })
            .then(response => response.json())
            .then(data => {
                console.log('Command sent:', data);
                updateStatus();
            })
            .catch(error => {
                console.error('Error sending command:', error);
                alert('Error sending command');
            });
        }
        
        function testAPI() {
            fetch('/api/test-api', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'}
            })
            .then(response => response.json())
            .then(data => {
                console.log('API test result:', data);
                alert('API Test: ' + data.message);
                updateStatus();
            })
            .catch(error => {
                console.error('Error testing API:', error);
                alert('Error testing API connection');
            });
        }
        
        setInterval(updateStatus, 2000);
        updateStatus();
    </script>
</body>
</html>
)HTML";
}