#include "GeoEntryDevice.h"

GeoEntryDevice::GeoEntryDevice(const String& wifiSSID, const String& wifiPassword, 
                               const String& apiURL, const String& deviceID, const String& userID)
    : ssid(wifiSSID), password(wifiPassword), serverURL(apiURL), deviceId(deviceID), userId(userID),
      lastCheck(0), checkInterval(5000), lastSensorCheck(0), sensorCheckInterval(10000), 
      lastEventId(""), userAtHome(false),
      tvSensorActive(false), luzSensorActive(false), acSensorActive(false), cafeteraSensorActive(false),
      lastLed1Blink(0), lastLed2Blink(0), led1BlinkState(false), led2BlinkState(false),
      led1Pattern(0), led2Pattern(0) {
    
    proximityLed = nullptr;
    smartLed1 = nullptr;
    smartLed2 = nullptr;
}

GeoEntryDevice::~GeoEntryDevice() {
    delete proximityLed;
    delete smartLed1;
    delete smartLed2;
}

void GeoEntryDevice::init() {
    Serial.begin(115200);
    Serial.println("Iniciando GeoEntry Device...");
    
    initializeLeds();
    
    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.print("Conectando a WiFi");
    
    // Patrón de espera mientras conecta
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        smartLed1->handle(LedCommands::TOGGLE);
    }
    
    // WiFi conectado - patrón de éxito
    smartLed1->handle(LedCommands::TURN_ON);
    smartLed2->handle(LedCommands::TURN_ON);
    delay(1000);
    smartLed1->handle(LedCommands::TURN_OFF);
    smartLed2->handle(LedCommands::TURN_OFF);
    
    Serial.println();
    Serial.println("WiFi conectado!");
    Serial.print("Dirección IP: ");
    Serial.println(WiFi.localIP());
    
    on(GeoEntryEvents::WIFI_CONNECTED);
    
    Serial.println("GeoEntry Device iniciado correctamente");
    Serial.println("Monitoreando eventos de proximidad y sensores inteligentes...");
}

void GeoEntryDevice::initializeLeds() {
    proximityLed = new Led(2, false);  // LED rojo para proximidad
    smartLed1 = new Led(4, false);     // LED verde para TV/Luz
    smartLed2 = new Led(5, false);     // LED azul para AC/Cafetera
    
    proximityLed->turnOff(); 
    smartLed1->turnOff();
    smartLed2->turnOff();
} 

void GeoEntryDevice::loop() {
    if (WiFi.status() != WL_CONNECTED) {
        on(GeoEntryEvents::WIFI_DISCONNECTED);
        reconnectWiFi();
        return;
    }
    
    // Verificar eventos de proximidad
    if (millis() - lastCheck >= checkInterval) {
        handle(GeoEntryCommands::CHECK_PROXIMITY);
        lastCheck = millis();
    }
    
    // Verificar estados de sensores
    if (millis() - lastSensorCheck >= sensorCheckInterval) {
        handle(GeoEntryCommands::CHECK_SENSORS);
        lastSensorCheck = millis();
    }
    
    // Actualizar patrones de parpadeo de LEDs inteligentes
    updateSmartLedPatterns();
    
    delay(50); // Reducido para mejor respuesta de los patrones
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
        // Patrón de éxito en LEDs inteligentes
        smartLed1->blink(2, 200);
        smartLed2->blink(2, 200);
    } else if (event == GeoEntryEvents::WIFI_DISCONNECTED) {
        Serial.println("📶 WiFi desconectado");
        // Apagar LEDs inteligentes cuando no hay WiFi
        smartLed1->handle(LedCommands::TURN_OFF);
        smartLed2->handle(LedCommands::TURN_OFF);
    } else if (event == GeoEntryEvents::API_REQUEST_SUCCESS) {
        // Breve destello para indicar comunicación exitosa
    } else if (event == GeoEntryEvents::API_REQUEST_FAILED) {
        // Patrón de error
        smartLed1->blink(3, 100);
        smartLed2->blink(3, 100);
    }
}

void GeoEntryDevice::handle(Command command) {
    if (command == GeoEntryCommands::CHECK_PROXIMITY) {
        checkProximityEvents();
    } else if (command == GeoEntryCommands::CHECK_SENSORS) {
        checkSensorStates();
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
    // Función mantenida para compatibilidad pero ya no usa LED de estado
    // Los LEDs inteligentes muestran ahora el estado del sistema
}

void GeoEntryDevice::checkSensorStates() {
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }
    
    HTTPClient http;
    String url = "https://geoentry-edge-api.onrender.com/api/v1/sensors/user/" + userId;
    
    http.begin(url.c_str());
    http.addHeader("Content-Type", "application/json");
    
    Serial.println("Consultando sensores: " + url);
    
    int httpResponseCode = http.GET();
    
    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println("Respuesta sensores:");
        Serial.println(response);
        
        processSensorStates(response);
    } else {
        Serial.printf("Error en petición de sensores: %d\n", httpResponseCode);
    }
    
    http.end();
}

void GeoEntryDevice::processSensorStates(String jsonResponse) {
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, jsonResponse);
    
    if (error) {
        Serial.print("Error parsing sensors JSON: ");
        Serial.println(error.c_str());
        return;
    }
    
    // Resetear estados
    tvSensorActive = false;
    luzSensorActive = false;
    acSensorActive = false;
    cafeteraSensorActive = false;
    
    JsonArray sensors;
    if (doc.is<JsonArray>()) {
        sensors = doc.as<JsonArray>();
    } else if (doc["data"].is<JsonArray>()) {
        sensors = doc["data"];
    } else {
        Serial.println("Formato de respuesta de sensores inesperado");
        return;
    }
    
    Serial.println("=== ESTADOS DE SENSORES ===");
    for (JsonObject sensor : sensors) {
        String name = sensor["name"].as<String>();
        String type = sensor["sensor_type"].as<String>();
        bool isActive = sensor["isActive"];
        
        Serial.println("Sensor: " + name + " (" + type + ") - " + (isActive ? "ACTIVO" : "INACTIVO"));
        
        if (type == "tv") {
            tvSensorActive = isActive;
        } else if (type == "luz") {
            luzSensorActive = isActive;
        } else if (type == "aire_acondicionado") {
            acSensorActive = isActive;
        } else if (type == "cafetera") {
            cafeteraSensorActive = isActive;
        }
    }
    
    calculateLedPatterns();
    Serial.println("============================");
}

void GeoEntryDevice::calculateLedPatterns() {
    // LED Verde (Pin 4): TV y Luz
    led1Pattern = getLedPattern(tvSensorActive, luzSensorActive);
    
    // LED Azul (Pin 5): AC y Cafetera  
    led2Pattern = getLedPattern(acSensorActive, cafeteraSensorActive);
    
    Serial.println("Patrones calculados:");
    Serial.println("LED Verde (TV/Luz): " + String(led1Pattern));
    Serial.println("LED Azul (AC/Cafetera): " + String(led2Pattern));
}

int GeoEntryDevice::getLedPattern(bool sensor1, bool sensor2) {
    if (!sensor1 && !sensor2) return 0;  // Ambos apagados = LED apagado
    if (sensor1 && sensor2) return 1;     // Ambos encendidos = LED sólido
    if (sensor1 && !sensor2) return 2;    // Solo sensor1 = parpadeo lento
    if (!sensor1 && sensor2) return 3;    // Solo sensor2 = parpadeo rápido
    return 0;
}

void GeoEntryDevice::updateSmartLedPatterns() {
    unsigned long currentTime = millis();
    
    // Manejar LED Verde (TV/Luz)
    switch (led1Pattern) {
        case 0: // Apagado
            smartLed1->setState(false);
            break;
        case 1: // Sólido
            smartLed1->setState(true);
            break;
        case 2: // Parpadeo lento (1000ms)
            if (currentTime - lastLed1Blink >= 1000) {
                led1BlinkState = !led1BlinkState;
                smartLed1->setState(led1BlinkState);
                lastLed1Blink = currentTime;
            }
            break;
        case 3: // Parpadeo rápido (300ms)
            if (currentTime - lastLed1Blink >= 300) {
                led1BlinkState = !led1BlinkState;
                smartLed1->setState(led1BlinkState);
                lastLed1Blink = currentTime;
            }
            break;
    }
    
    // Manejar LED Azul (AC/Cafetera)
    switch (led2Pattern) {
        case 0: // Apagado
            smartLed2->setState(false);
            break;
        case 1: // Sólido
            smartLed2->setState(true);
            break;
        case 2: // Parpadeo lento (1000ms)
            if (currentTime - lastLed2Blink >= 1000) {
                led2BlinkState = !led2BlinkState;
                smartLed2->setState(led2BlinkState);
                lastLed2Blink = currentTime;
            }
            break;
        case 3: // Parpadeo rápido (300ms)
            if (currentTime - lastLed2Blink >= 300) {
                led2BlinkState = !led2BlinkState;
                smartLed2->setState(led2BlinkState);
                lastLed2Blink = currentTime;
            }
            break;
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

void GeoEntryDevice::setUserConfiguration(const String& userID) {
    userId = userID;
}

void GeoEntryDevice::setCheckInterval(unsigned long interval) {
    checkInterval = interval;
}

void GeoEntryDevice::setSensorCheckInterval(unsigned long interval) {
    sensorCheckInterval = interval;
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

void GeoEntryDevice::setSmartLed1Pattern(int pattern) {
    led1Pattern = pattern;
}

void GeoEntryDevice::setSmartLed2Pattern(int pattern) {
    led2Pattern = pattern;
}
