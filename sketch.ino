/*
 * Ejemplo de uso del GeoEntry Device con Sensores Inteligentes
 * 
 * Este ejemplo muestra cómo configurar y usar el dispositivo GeoEntry
 * con la nueva funcionalidad de sensores inteligentes que automatiza
 * el encendido/apagado según la presencia del usuario en casa.
 * 
 * LÓGICA DE AUTOMATIZACIÓN:
 * 
 * 🏠 USUARIO ENTRA A CASA:
 * - LED Rojo se enciende (proximidad)
 * - TODOS los sensores se encienden automáticamente
 * - LEDs inteligentes muestran patrones según sensores activos
 * - Usuario puede controlar sensores desde app/web
 * 
 * 🚪 USUARIO SALE DE CASA:
 * - LED Rojo se apaga (proximidad)
 * - TODOS los sensores se apagan automáticamente (seguridad)
 * - LEDs inteligentes se apagan
 * - App/Web bloquea controles hasta que regrese
 * 
 * LEDs utilizados:
 * - LED Rojo (Pin 2): Proximidad (encendido cuando el usuario está en casa)
 * - LED Verde (Pin 4): Estados de TV y Luz (patrones de parpadeo)
 * - LED Azul (Pin 5): Estados de AC y Cafetera (patrones de parpadeo)
 * 
 * Patrones de LEDs Inteligentes (solo cuando usuario está en casa):
 * - Apagado: Ambos sensores inactivos
 * - Sólido: Ambos sensores activos  
 * - Parpadeo lento (1s): Solo primer sensor activo (TV o AC)
 * - Parpadeo rápido (0.3s): Solo segundo sensor activo (Luz o Cafetera)
 */

#include "GeoEntryDevice.h"

// Configuración del dispositivo
const String WIFI_SSID = "Wokwi-GUEST";
const String WIFI_PASSWORD = "";
const String API_URL = "https://geoentry-edge-api.onrender.com/api/v1/proximity-events/device/";
const String DEVICE_ID = "7b4cdbcd-2bf0-4047-9355-05e33babf2c9";
const String USER_ID = "dd380cd7-852b-4855-9c68-c45f71b62521";

// Crear instancia del dispositivo
GeoEntryDevice* geoDevice;

void setup() {
    Serial.begin(115200);
    Serial.println("=================================");
    Serial.println("  GeoEntry Smart Home");
    Serial.println("=================================");
    
    // Crear y configurar el dispositivo
    geoDevice = new GeoEntryDevice(WIFI_SSID, WIFI_PASSWORD, API_URL, DEVICE_ID, USER_ID);
    
    // Configurar intervalos de consulta
    geoDevice->setCheckInterval(5000);        // Proximidad cada 5 segundos
    geoDevice->setSensorCheckInterval(10000); // Sensores cada 10 segundos
    
    // Inicializar el dispositivo
    geoDevice->init();
    
    Serial.println("\n📱 Configuración:");
    Serial.println("   - WiFi: " + WIFI_SSID);
    Serial.println("   - Device ID: " + DEVICE_ID);
    Serial.println("   - User ID: " + USER_ID);
    Serial.println("   - Proximidad: cada 5s");
    Serial.println("   - Sensores: cada 10s");
    
    Serial.println("\n🔍 Automatización Inteligente:");
    Serial.println("   Cuando ENTRA a casa:");
    Serial.println("     🔴 LED rojo se enciende");
    Serial.println("     ✅ TODOS los sensores se encienden automáticamente");
    Serial.println("     🎮 Control manual disponible en app/web");
    
    Serial.println("   Cuando SALE de casa:");
    Serial.println("     ⚫ LED rojo se apaga");
    Serial.println("     ❌ TODOS los sensores se apagan automáticamente");
    Serial.println("     🚫 Control manual bloqueado por seguridad");
    
    Serial.println("\n🔍 Patrones de LEDs (solo cuando está en casa):");
    Serial.println("   LED Verde (TV/Luz):");
    Serial.println("     • Apagado: TV❌ Luz❌");
    Serial.println("     • Sólido: TV✅ Luz✅");
    Serial.println("     • Lento: TV✅ Luz❌");
    Serial.println("     • Rápido: TV❌ Luz✅");
    
    Serial.println("   LED Azul (AC/Cafetera):");
    Serial.println("     • Apagado: AC❌ Cafetera❌");
    Serial.println("     • Sólido: AC✅ Cafetera✅");
    Serial.println("     • Lento: AC✅ Cafetera❌");
    Serial.println("     • Rápido: AC❌ Cafetera✅");
    
    Serial.println("\n🚀 Sistema iniciado - monitoreando...\n");
}

void loop() {
    // El loop principal del dispositivo maneja todo automáticamente
    geoDevice->loop();
    
    // Opcional: Mostrar información de estado cada 30 segundos
    static unsigned long lastStatus = 0;
    if (millis() - lastStatus >= 30000) {
        printSystemStatus();
        lastStatus = millis();
    }
}

void printSystemStatus() {
    Serial.println("\n📊 Estado del Sistema:");
    Serial.println("   • WiFi: " + String(geoDevice->isWiFiConnected() ? "Conectado" : "Desconectado"));
    Serial.println("   • Usuario en casa: " + String(geoDevice->isUserAtHome() ? "Sí" : "No"));
    Serial.println("   • Último evento: " + geoDevice->getLastEventId());
    Serial.println("   • Memoria libre: " + String(ESP.getFreeHeap()) + " bytes");
    Serial.println("   • Uptime: " + String(millis() / 1000) + " segundos");
    Serial.println();
}

// Función para manejar comandos desde el Serial Monitor (opcional)
void serialEvent() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        if (command == "status") {
            printSystemStatus();
        } else if (command == "restart") {
            Serial.println("🔄 Reiniciando sistema...");
            ESP.restart();
        } else if (command == "check") {
            Serial.println("🔍 Forzando verificación...");
            geoDevice->handle(GeoEntryCommands::CHECK_PROXIMITY);
            geoDevice->handle(GeoEntryCommands::CHECK_SENSORS);
        } else if (command == "help") {
            Serial.println("\n📋 Comandos disponibles:");
            Serial.println("   • status  - Mostrar estado del sistema");
            Serial.println("   • restart - Reiniciar el dispositivo");
            Serial.println("   • check   - Forzar verificación");
            Serial.println("   • help    - Mostrar esta ayuda");
            Serial.println();
        } else {
            Serial.println("❓ Comando desconocido. Escribe 'help' para ver los comandos disponibles.");
        }
    }
}
