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
    Led* statusLed;     // LED verde - estado del sistema
    Led* wifiLed;       // LED azul - estado WiFi
    
    String ssid;
    String password;
    
    String serverURL;
    String deviceId;
    
    unsigned long lastCheck;
    unsigned long checkInterval;
    String lastEventId;
    bool userAtHome;
    
    void initializeLeds();
    void checkProximityEvents();
    void processProximityEvents(String jsonResponse);
    void processEvent(JsonObject event);
    void reconnectWiFi();
    void updateSystemStatus();

public:
    GeoEntryDevice(
        const String& wifiSSID = "Wokwi-GUEST",
        const String& wifiPassword = "",
        const String& apiURL = "https://geoentry-edge-api.onrender.com/api/v1/proximity-events/device/",
        const String& deviceID = "7b4cdbcd-2bf0-4047-9355-05e33babf2c9"
    );
    
    ~GeoEntryDevice();
    
    void on(Event event) override;
    void handle(Command command) override;
    
    void init();
    void loop();
    
    void setWiFiCredentials(const String& ssid, const String& password);
    void setAPIConfiguration(const String& url, const String& deviceId);
    void setCheckInterval(unsigned long interval);
    
    bool isUserAtHome() const;
    bool isWiFiConnected() const;
    String getLastEventId() const;
    
    void setProximityStatus(bool atHome);
    void setSystemStatus(bool active);
    void setWiFiStatus(bool connected);
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
    const Command RECONNECT_WIFI(2);
    const Command RESET_SYSTEM(3);
    const Command UPDATE_STATUS(4);
}

#endif
