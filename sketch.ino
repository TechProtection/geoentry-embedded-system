/*
 * GeoEntry ESP32 Proximity Event Monitor
 * 
 * Este código conecta un ESP32 al Edge API de GeoEntry para monitorear
 * eventos de proximidad y controlar LEDs basado en cuando un usuario
 * entra o sale de casa.
 * 
 * CONFIGURACIÓN REQUERIDA:
 * 1. Cambiar 'YOUR_WIFI_SSID' por el nombre de tu red WiFi
 * 2. Cambiar 'YOUR_WIFI_PASSWORD' por la contraseña de tu WiFi
 * 3. Cambiar '192.168.1.100' por la IP donde corre el Edge API
 * 4. Verificar que el deviceId corresponda al dispositivo móvil
 * 
 * CONEXIONES:
 * - LED Rojo (Pin D2): Indica si usuario está en casa (ON) o fuera (OFF)
 * - LED Verde (Pin D4): Estado del sistema y consultas al API
 * - LED Azul (Pin D5): Estado de conexión WiFi
 * 
 * FUNCIONAMIENTO:
 * - Consulta el API cada 10 segundos
 * - Cuando event_type="enter": Enciende LED rojo
 * - Cuando event_type="exit": Apaga LED rojo
 * - Muestra el nombre de la ubicación en consola
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Configuración WiFi
const char* ssid = "Wokwi-GUEST";        // Cambia por tu red WiFi
const char* password = ""; // Cambia por tu contraseña WiFi

// Configuración del API
const char* serverURL = "https://geoentry-edge-api.onrender.com/api/v1/proximity-events/device/"; // Cambia por tu IP local
const char* deviceId = "7b4cdbcd-2bf0-4047-9355-05e33babf2c9"; // ID del dispositivo móvil

// Configuración de LEDs
const int LED_PROXIMITY_PIN = 2; // Pin D2 - LED rojo para proximidad
const int LED_STATUS_PIN = 4;    // Pin D4 - LED verde para estado del sistema
const int LED_WIFI_PIN = 5;      // Pin D5 - LED azul para WiFi
bool proximityLedState = false;

// Variables de control
unsigned long lastCheck = 0;
const unsigned long checkInterval = 5000; // Verificar cada 10 segundos
String lastEventId = "";

void setup() {
  Serial.begin(115200);
  
  // Configurar LEDs
  pinMode(LED_PROXIMITY_PIN, OUTPUT);
  pinMode(LED_STATUS_PIN, OUTPUT);
  pinMode(LED_WIFI_PIN, OUTPUT);
  
  // Estado inicial de LEDs
  digitalWrite(LED_PROXIMITY_PIN, LOW); // Rojo apagado (no hay usuario en casa)
  digitalWrite(LED_STATUS_PIN, HIGH);   // Verde encendido (sistema iniciando)
  digitalWrite(LED_WIFI_PIN, LOW);      // Azul apagado (WiFi desconectado)
  
  // Conectar a WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    // Parpadear LED azul mientras conecta
    digitalWrite(LED_WIFI_PIN, !digitalRead(LED_WIFI_PIN));
  }
  
  // WiFi conectado - LED azul encendido
  digitalWrite(LED_WIFI_PIN, HIGH);
  
  Serial.println();
  Serial.println("WiFi conectado!");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());
  
  Serial.println("ESP32 Proximity Event Monitor iniciado");
  Serial.println("Monitoreando eventos de proximidad...");
  Serial.println("LEDs:");
  Serial.println("- Rojo (D2): Usuario en casa / fuera de casa");
  Serial.println("- Verde (D4): Estado del sistema");
  Serial.println("- Azul (D5): Estado WiFi");
}

void loop() {
  // Verificar eventos cada 10 segundos
  if (millis() - lastCheck >= checkInterval) {
    checkProximityEvents();
    lastCheck = millis();
  }
  
  delay(100);
}

void checkProximityEvents() {
  if (WiFi.status() == WL_CONNECTED) {
    // LED verde parpadea durante la consulta
    digitalWrite(LED_STATUS_PIN, LOW);
    
    HTTPClient http;
    
    // Construir URL completa
    String url = String(serverURL) + String(deviceId);
    
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    
    Serial.println("Consultando: " + url);
    
    int httpResponseCode = http.GET();
    
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Respuesta del servidor:");
      Serial.println(response);
      
      processProximityEvents(response);
      
      // LED verde encendido - consulta exitosa
      digitalWrite(LED_STATUS_PIN, HIGH);
    } else {
      Serial.print("Error en la petición HTTP: ");
      Serial.println(httpResponseCode);
      
      // LED verde parpadea rápido - error en consulta
      for (int i = 0; i < 3; i++) {
        digitalWrite(LED_STATUS_PIN, HIGH);
        delay(100);
        digitalWrite(LED_STATUS_PIN, LOW);
        delay(100);
      }
      digitalWrite(LED_STATUS_PIN, HIGH);
    }
    
    http.end();
  } else {
    Serial.println("WiFi desconectado");
    digitalWrite(LED_WIFI_PIN, LOW);
    reconnectWiFi();
  }
}

void processProximityEvents(String jsonResponse) {
  // Crear documento JSON
  DynamicJsonDocument doc(4096);
  
  // Parsear JSON
  DeserializationError error = deserializeJson(doc, jsonResponse);
  
  if (error) {
    Serial.print("Error parseando JSON: ");
    Serial.println(error.c_str());
    return;
  }
  
  // Verificar si es un array
  if (doc.is<JsonArray>()) {
    JsonArray events = doc.as<JsonArray>();
    
    if (events.size() > 0) {
      // Procesar el evento más reciente (primer elemento del array)
      JsonObject latestEvent = events[0];
      processEvent(latestEvent);
    } else {
      Serial.println("No hay eventos de proximidad");
    }
  } else if (doc.is<JsonObject>()) {
    // Si es un solo objeto
    JsonObject event = doc.as<JsonObject>();
    processEvent(event);
  }
}

void processEvent(JsonObject event) {
  String eventId = event["event_id"];
  String eventType = event["event_type"];
  String homeLocationName = event["home_location_name"];
  float distance = event["distance"];
  String createdAt = event["created_at"];
  
  // Verificar si es un evento nuevo
  if (eventId != lastEventId) {
    lastEventId = eventId;
    
    Serial.println("=== NUEVO EVENTO DE PROXIMIDAD ===");
    Serial.println("ID del evento: " + eventId);
    Serial.println("Tipo de evento: " + eventType);
    Serial.println("Ubicación: " + homeLocationName);
    Serial.println("Distancia: " + String(distance) + " metros");
    Serial.println("Fecha: " + createdAt);
    Serial.println("================================");
    
    // Controlar LED basado en el tipo de evento
    if (eventType == "enter") {
      // Usuario entró a casa - Encender LED rojo
      digitalWrite(LED_PROXIMITY_PIN, HIGH);
      proximityLedState = true;
      Serial.println("🏠 USUARIO ENTRÓ A " + homeLocationName + " - LED ROJO ENCENDIDO");
    } else if (eventType == "exit") {
      // Usuario salió de casa - Apagar LED rojo
      digitalWrite(LED_PROXIMITY_PIN, LOW);
      proximityLedState = false;
      Serial.println("🚪 USUARIO SALIÓ DE " + homeLocationName + " - LED ROJO APAGADO");
    }
    
    // Mostrar estado actual del LED
    Serial.println("Estado del LED de Proximidad: " + String(proximityLedState ? "ENCENDIDO" : "APAGADO"));
    Serial.println();
  }
}

void reconnectWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Reintentando conexión WiFi...");
    digitalWrite(LED_WIFI_PIN, LOW);
    
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      // Parpadear LED azul durante reconexión
      digitalWrite(LED_WIFI_PIN, !digitalRead(LED_WIFI_PIN));
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      digitalWrite(LED_WIFI_PIN, HIGH);
      Serial.println("\nWiFi reconectado!");
    } else {
      digitalWrite(LED_WIFI_PIN, LOW);
      Serial.println("\nFallo en reconexión WiFi");
    }
  }
}