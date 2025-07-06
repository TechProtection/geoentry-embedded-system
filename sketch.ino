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

// Configuración de red y API
const String WIFI_SSID = "Wokwi-GUEST";
const String WIFI_PASSWORD = "";
const String API_URL = "https://geoentry-edge-api.onrender.com/api/v1/";

// IDs únicos para cada dispositivo
const String DEVICE_ID = "7b4cdbcd-2bf0-4047-9355-05e33babf2c9";
const String USER_ID = "dd380cd7-852b-4855-9c68-c45f71b62521";

// Instancia del dispositivo
GeoEntryDevice* device;

void setup() {
    Serial.begin(115200);
    Serial.println("=================================");
    Serial.println("  GeoEntry Smart Home");
    Serial.println("=================================");
    
    // Crear e inicializar el dispositivo
    device = new GeoEntryDevice(WIFI_SSID, WIFI_PASSWORD, API_URL, DEVICE_ID, USER_ID);
    device->init();
    
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
    
    Serial.println("\n🚀 Sistema iniciado - monitoreando...");
}

void loop() {
    // Ejecutar el bucle principal del dispositivo
    device->loop();
}