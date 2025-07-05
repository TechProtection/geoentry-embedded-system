#include "GeoEntryDevice.h"

GeoEntryDevice::GeoEntryDevice(const String& wifiSSID, const String& wifiPassword, 
                               const String& apiURL, const String& deviceID)
    : ssid(wifiSSID), password(wifiPassword), serverURL(apiURL), deviceId(deviceID),
      lastCheck(0), checkInterval(5000), lastEventId(""), userAtHome(false) {
    
    proximityLed = nullptr;
    statusLed = nullptr;
    wifiLed = nullptr;
}

GeoEntryDevice::~GeoEntryDevice() {
    delete proximityLed;
    delete statusLed;
    delete wifiLed;
}

void GeoEntryDevice::init() {
    Serial.begin(115200);
    Serial.println("Iniciando GeoEntry Device...");
    
    initializeLeds();
    
    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.print("Conectando a WiFi");
    
    statusLed->handle(LedCommands::TURN_ON);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        statusLed->handle(LedCommands::TOGGLE);
    }
    
    setWiFiStatus(true);
    on(GeoEntryEvents::WIFI_CONNECTED);
    
    Serial.println();
    Serial.println("WiFi conectado!");
    Serial.print("Dirección IP: ");
    Serial.println(WiFi.localIP());
    
    Serial.println("GeoEntry Device iniciado correctamente");
    Serial.println("Monitoreando eventos de proximidad...");
}

void GeoEntryDevice::initializeLeds() {
    proximityLed = new Led(2, false);
    statusLed = new Led(4, false);   
    wifiLed = new Led(5, false);
    
    proximityLed->turnOff(); 
    statusLed->turnOn();
    wifiLed->turnOff(); 

void GeoEntryDevice::loop() {
    if (WiFi.status() != WL_CONNECTED) {
        if (wifiLed->getState()) {
            setWiFiStatus(false);
            on(GeoEntryEvents::WIFI_DISCONNECTED);
        }
        reconnectWiFi();
        return;
    }
    
    if (millis() - lastCheck >= checkInterval) {
        handle(GeoEntryCommands::CHECK_PROXIMITY);
        lastCheck = millis();
    }
    
    updateSystemStatus();
    delay(100);
}

void GeoEntryDevice::on(Event event) {
    if (event == GeoEntryEvents::USER_ENTERED) {
        Serial.println("🏠 Usuario ENTRÓ a casa");
        setProximityStatus(true);
        userAtHome = true;
    } else if (event == GeoEntryEvents::USER_EXITED) {
        Serial.println("🚶 Usuario SALIÓ de casa");
        setProximityStatus(false);
        userAtHome = false;
    } else if (event == GeoEntryEvents::WIFI_CONNECTED) {
        Serial.println("📶 WiFi conectado");
        statusLed->handle(LedCommands::TURN_ON);
    } else if (event == GeoEntryEvents::WIFI_DISCONNECTED) {
        Serial.println("📶 WiFi desconectado");
        statusLed->handle(LedCommands::BLINK);
    } else if (event == GeoEntryEvents::API_REQUEST_SUCCESS) {
        statusLed->blink(1, 100); 
    } else if (event == GeoEntryEvents::API_REQUEST_FAILED) {
        statusLed->blink(3, 200);
    }
}

void GeoEntryDevice::handle(Command command) {
    if (command == GeoEntryCommands::CHECK_PROXIMITY) {
        checkProximityEvents();
    } else if (command == GeoEntryCommands::RECONNECT_WIFI) {
        reconnectWiFi();
    } else if (command == GeoEntryCommands::RESET_SYSTEM) {
        ESP.restart();
    } else if (command == GeoEntryCommands::UPDATE_STATUS) {
        updateSystemStatus();
    }
}

void GeoEntryDevice::checkProximityEvents() {
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }
    
    HTTPClient http;
    String url = serverURL + deviceId;
    
    http.begin(url.c_str());
    http.addHeader("Content-Type", "application/json");
    
    Serial.println("Consultando: " + url);
    
    int httpResponseCode = http.GET();
    
    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println("Respuesta del servidor:");
        Serial.println(response);
        
        on(GeoEntryEvents::API_REQUEST_SUCCESS);
        processProximityEvents(response);
    } else {
        Serial.printf("Error en petición HTTP: %d\n", httpResponseCode);
        on(GeoEntryEvents::API_REQUEST_FAILED);
    }
    
    http.end();
}

void GeoEntryDevice::processProximityEvents(String jsonResponse) {
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, jsonResponse);
    
    if (error) {
        Serial.print("Error parsing JSON: ");
        Serial.println(error.c_str());
        return;
    }
    
    if (doc.is<JsonArray>()) {
        JsonArray events = doc.as<JsonArray>();
        
        if (events.size() > 0) {
            JsonObject latestEvent = events[0];
            processEvent(latestEvent);
        } else {
            Serial.println("No hay eventos de proximidad");
        }
    } else if (doc.is<JsonObject>()) {
        // Si es un solo objeto
        JsonObject event = doc.as<JsonObject>();
        processEvent(event);
    } else if (doc["data"].is<JsonArray>()) {
        // Si viene encapsulado en un objeto con campo "data"
        JsonArray events = doc["data"];
        
        for (JsonObject event : events) {
            processEvent(event);
        }
    }
}

void GeoEntryDevice::processEvent(JsonObject event) {
    String eventId = event["event_id"].as<String>();
    if (eventId.isEmpty()) {
        eventId = event["id"].as<String>();
    }
    
    String eventType = event["event_type"].as<String>();
    
    String locationName = event["home_location_name"].as<String>();
    if (locationName.isEmpty()) {
        locationName = event["location_name"].as<String>();
    }
    
    float distance = event["distance"];
    String createdAt = event["created_at"].as<String>();
    
    if (eventId == lastEventId) {
        return;
    }
    
    lastEventId = eventId;
    
    Serial.println("=== NUEVO EVENTO DE PROXIMIDAD ===");
    Serial.println("ID del evento: " + eventId);
    Serial.println("Tipo de evento: " + eventType);
    Serial.println("Ubicación: " + locationName);
    if (distance > 0) {
        Serial.println("Distancia: " + String(distance) + " metros");
    }
    if (!createdAt.isEmpty()) {
        Serial.println("Fecha: " + createdAt);
    }
    Serial.println("================================");
    
    if (eventType == "enter") {
        on(GeoEntryEvents::USER_ENTERED);
        Serial.println("🏠 USUARIO ENTRÓ A " + locationName + " - LED ROJO ENCENDIDO");
    } else if (eventType == "exit") {
        on(GeoEntryEvents::USER_EXITED);
        Serial.println("🚪 USUARIO SALIÓ DE " + locationName + " - LED ROJO APAGADO");
    }
    
    Serial.println("Estado del LED de Proximidad: " + String(userAtHome ? "ENCENDIDO" : "APAGADO"));
    Serial.println();
}

void GeoEntryDevice::reconnectWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        return;
    }
    
    Serial.println("Reconectando WiFi...");
    WiFi.disconnect();
    WiFi.begin(ssid.c_str(), password.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        setWiFiStatus(true);
        on(GeoEntryEvents::WIFI_CONNECTED);
    }
}

void GeoEntryDevice::updateSystemStatus() {
    static unsigned long lastStatusBlink = 0;
    if (millis() - lastStatusBlink >= 2000) {
        statusLed->blink(1, 50);
        lastStatusBlink = millis();
    }
}

void GeoEntryDevice::setWiFiCredentials(const String& newSSID, const String& newPassword) {
    ssid = newSSID;
    password = newPassword;
}

void GeoEntryDevice::setAPIConfiguration(const String& url, const String& deviceID) {
    serverURL = url;
    deviceId = deviceID;
}

void GeoEntryDevice::setCheckInterval(unsigned long interval) {
    checkInterval = interval;
}

bool GeoEntryDevice::isUserAtHome() const {
    return userAtHome;
}

bool GeoEntryDevice::isWiFiConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

String GeoEntryDevice::getLastEventId() const {
    return lastEventId;
}

void GeoEntryDevice::setProximityStatus(bool atHome) {
    if (atHome) {
        proximityLed->handle(LedCommands::TURN_ON);
    } else {
        proximityLed->handle(LedCommands::TURN_OFF);
    }
}

void GeoEntryDevice::setSystemStatus(bool active) {
    if (active) {
        statusLed->handle(LedCommands::TURN_ON);
    } else {
        statusLed->handle(LedCommands::TURN_OFF);
    }
}

void GeoEntryDevice::setWiFiStatus(bool connected) {
    if (connected) {
        wifiLed->handle(LedCommands::TURN_ON);
    } else {
        wifiLed->handle(LedCommands::TURN_OFF);
    }
}
