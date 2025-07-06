#ifndef GEOENTRY_DEVICE_H
#define GEOENTRY_DEVICE_H

#include "Device.h"
#include "Led.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

class GeoEntryDevice : public Device {
private:

    Led* proximityLed;  // LED rojo - indica presencia en casa
    Led* smartLed1;     // LED verde - sensores TV/Luz
    Led* smartLed2;     // LED azul - sensores AC/Cafetera
    
    String ssid;
    String password;
    
    String serverURL;
    String deviceId;
    String userId;  // ID del usuario para consultar sensores
    
    unsigned long lastCheck;
    unsigned long checkInterval;
    unsigned long lastSensorCheck;
    unsigned long sensorCheckInterval;
    String lastEventId;
    bool userAtHome;
    
    // Estados de sensores virtuales
    bool tvSensorActive;
    bool luzSensorActive;
    bool acSensorActive;
    bool cafeteraSensorActive;
    
    // Variables para control de patrones de parpadeo
    unsigned long lastLed1Blink;
    unsigned long lastLed2Blink;
    bool led1BlinkState;
    bool led2BlinkState;
    int led1Pattern;  // 0=off, 1=solid, 2=slow, 3=fast
    int led2Pattern;  // 0=off, 1=solid, 2=slow, 3=fast
    
    void initializeLeds();
    void checkProximityEvents();
    void checkSensorStates();
    void processProximityEvents(String jsonResponse);
    void processSensorStates(String jsonResponse);
    void processEvent(JsonObject event);
    void reconnectWiFi();
    void updateSystemStatus();
    void updateSmartLedPatterns();
    void calculateLedPatterns();
    int getLedPattern(bool sensor1, bool sensor2);
    void turnOnAllSensorsOnEnter();
    void turnOffAllSensorsOnExit();
    void turnOnSensor(String sensorId, String sensorType);
    void turnOffSensor(String sensorId, String sensorType);
    String getPatternDescription(int pattern, String sensor1, String sensor2);

public:
    GeoEntryDevice(
        const String& wifiSSID = "Wokwi-GUEST",
        const String& wifiPassword = "",
        const String& apiURL = "https://geoentry-edge-api.onrender.com/api/v1/",
        const String& deviceID = "7b4cdbcd-2bf0-4047-9355-05e33babf2c9",
        const String& userID = "dd380cd7-852b-4855-9c68-c45f71b62521"
    );
    
    ~GeoEntryDevice();
    
    void on(Event event) override;
    void handle(Command command) override;
    
    void init();
    void loop();
    
    void setWiFiCredentials(const String& ssid, const String& password);
    void setAPIConfiguration(const String& url, const String& deviceId);
    void setUserConfiguration(const String& userID);
    void setCheckInterval(unsigned long interval);
    void setSensorCheckInterval(unsigned long interval);
    
    bool isUserAtHome() const;
    bool isWiFiConnected() const;
    String getLastEventId() const;
    
    void setProximityStatus(bool atHome);
    void setSmartLed1Pattern(int pattern);
    void setSmartLed2Pattern(int pattern);
};

namespace GeoEntryEvents {
    const Event USER_ENTERED(1);
    const Event USER_EXITED(2);
    const Event WIFI_CONNECTED(3);
    const Event WIFI_DISCONNECTED(4);
    const Event API_REQUEST_SUCCESS(5);
    const Event API_REQUEST_FAILED(6);
}

namespace GeoEntryCommands {
    const Command CHECK_PROXIMITY(1);
    const Command CHECK_SENSORS(2);
    const Command RECONNECT_WIFI(3);
    const Command RESET_SYSTEM(4);
    const Command UPDATE_STATUS(5);
}

#endif
