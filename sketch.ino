/*
 * GeoEntry ESP32 Proximity Event Monitor - ModestIoT Version
 * 
 * Este código utiliza la arquitectura ModestIoT para conectar un ESP32
 * al Edge API de GeoEntry y monitorear eventos de proximidad.
 * 
 * FUNCIONAMIENTO:
 * - Consulta el API cada 15 segundos usando la arquitectura ModestIoT
 * - Cuando event_type="enter": Enciende LED rojo
 * - Cuando event_type="exit": Apaga LED rojo
 * - Muestra el nombre de la ubicación en consola
 */

#include "ModestIoT.h"

const String WIFI_SSID = "Wokwi-GUEST";
const String WIFI_PASSWORD = "";

const String API_URL = "https://geoentry-edge-api.onrender.com/api/v1/proximity-events/device/";
const String DEVICE_ID = "7b4cdbcd-2bf0-4047-9355-05e33babf2c9";

GeoEntryDevice* geoDevice;

void setup() {
  Serial.begin(115200);
  Serial.println("=== GeoEntry ESP32 Monitor - ModestIoT ===");
  
  geoDevice = new GeoEntryDevice(
    WIFI_SSID,    
    WIFI_PASSWORD,
    API_URL,
    DEVICE_ID
  );
  
  geoDevice->setCheckInterval(15000);
  
  geoDevice->init();
  
  Serial.println("LEDs configurados:");
  Serial.println("- Rojo (D2): Usuario en casa / fuera de casa");
  Serial.println("- Verde (D4): Estado del sistema y actividad");
  Serial.println("- Azul (D5): Estado de conexión WiFi");
  Serial.println("Sistema iniciado correctamente");
}

void loop() {
  geoDevice->loop();
}