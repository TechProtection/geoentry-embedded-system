# GeoEntry IoT Device - Sistema de Monitoreo de Proximidad y Sensores Inteligentes

Este proyecto implementa un sistema de monitoreo de eventos de proximidad y control de sensores inteligentes para GeoEntry usando el framework **ModestIoT** en un ESP32. El dispositivo consulta el Edge API de GeoEntry para detectar cuando un usuario entra o sale de casa y también monitorea el estado de dispositivos inteligentes del hogar, representando la información mediante patrones de LEDs.

## Características Principales

- **Framework ModestIoT**: Implementación orientada a objetos con patrones Command y Observer
- **Conectividad WiFi**: Conexión automática y reconexión inteligente
- **Monitoreo de Proximidad**: Consulta periódica del API de eventos de proximidad cada 5 segundos
- **Sensores Inteligentes**: Monitoreo del estado de dispositivos del hogar cada 10 segundos
- **Indicadores LED Avanzados**: Sistema visual con patrones de parpadeo para representar múltiples estados
- **Gestión de Eventos**: Procesamiento de eventos de entrada/salida y estados de sensores

## Componentes de Hardware

### Microcontrolador
- **ESP32 DevKit V1**: Controlador principal con WiFi integrado

### LEDs Indicadores

#### LED Rojo (Pin D2) - Proximidad
- 🔴 **ENCENDIDO**: Usuario está en casa
- ⚫ **APAGADO**: Usuario fuera de casa

#### LED Verde (Pin D4) - Sensores TV/Luz
- ⚫ **APAGADO**: TV❌ Luz❌ (ambos inactivos)
- 🟢 **SÓLIDO**: TV✅ Luz✅ (ambos activos)
- 🟢 **PARPADEO LENTO (1s)**: TV✅ Luz❌ (solo TV activa)
- 🟢 **PARPADEO RÁPIDO (0.3s)**: TV❌ Luz✅ (solo Luz activa)

#### LED Azul (Pin D5) - Sensores AC/Cafetera
- ⚫ **APAGADO**: AC❌ Cafetera❌ (ambos inactivos)
- 🔵 **SÓLIDO**: AC✅ Cafetera✅ (ambos activos)
- 🔵 **PARPADEO LENTO (1s)**: AC✅ Cafetera❌ (solo AC activo)
- 🔵 **PARPADEO RÁPIDO (0.3s)**: AC❌ Cafetera✅ (solo Cafetera activa)

### Resistencias
- **3x Resistencias de 220Ω**: Limitación de corriente para LEDs
- **Botón de Reset**: Reinicio manual del sistema

## Arquitectura del Software

### Framework ModestIoT

El proyecto utiliza una arquitectura basada en componentes con los siguientes elementos:

```
ModestIoT Framework
├── EventHandler (Patrón Observer)
├── CommandHandler (Patrón Command)
├── Device (Clase base abstracta)
├── Sensor (Manejo de eventos)
├── Actuator (Ejecución de comandos)
└── Led (Actuador específico)
```

### Clases Principales

#### `GeoEntryDevice`
- **Hereda de**: `Device`
- **Responsabilidades**:
  - Gestión de conectividad WiFi
  - Consultas periódicas al API
  - Procesamiento de eventos de proximidad
  - Control de LEDs indicadores
  - Manejo de estados del sistema

#### `Led`
- **Hereda de**: `Actuator`
- **Responsabilidades**:
  - Control individual de LEDs
  - Comandos: encender, apagar, alternar, parpadear
  - Soporte para lógica invertida

## Configuración del Proyecto

### Credenciales WiFi
```cpp
const char* WIFI_SSID = "Wokwi-GUEST";
### Configuración Básica
```cpp
const char* WIFI_SSID = "Tu_Red_WiFi";
const char* WIFI_PASSWORD = "Tu_Contraseña";
```

### Configuración del API
```cpp
const char* API_URL = "https://geoentry-edge-api.onrender.com/api/v1/proximity-events/device/";
const char* DEVICE_ID = "7b4cdbcd-2bf0-4047-9355-05e33babf2c9";
const char* USER_ID = "a8c6b41b-8c4d-4b8a-9e2f-1a3b5c7d9e0f";
```

### Configuración de Pines
```cpp
LED_PROXIMITY_PIN = 2  // D2 - LED Rojo (Proximidad)
LED_SMART1_PIN = 4     // D4 - LED Verde (TV/Luz)
LED_SMART2_PIN = 5     // D5 - LED Azul (AC/Cafetera)
```

### Configuración de Intervalos
```cpp
PROXIMITY_CHECK_INTERVAL = 5000   // 5 segundos
SENSOR_CHECK_INTERVAL = 10000     // 10 segundos
```

## Funcionamiento del Sistema

### Ciclo Principal
1. **Inicialización**: Configuración de LEDs y conexión WiFi
2. **Monitoreo de Proximidad**: Consulta del API cada 5 segundos
3. **Monitoreo de Sensores**: Consulta del estado de sensores cada 10 segundos
4. **Procesamiento de Datos**: Análisis de respuestas JSON de ambos APIs
5. **Actualización de Patrones**: Control de LEDs según estados
6. **Reporte de Estado**: Salida periódica por consola serial

### Tipos de Eventos de Proximidad
- **`enter`**: Usuario entra a casa → LED rojo se enciende
- **`exit`**: Usuario sale de casa → LED rojo se apaga

### Estados de Sensores Inteligentes
- **TV**: Sensor tipo `tv`
- **Luz**: Sensor tipo `luz`
- **Aire Acondicionado**: Sensor tipo `aire_acondicionado`
- **Cafetera**: Sensor tipo `cafetera`

### Lógica de Patrones LED
Cada LED inteligente representa 2 sensores usando un patrón basado en estados:
- **Ninguno activo** → LED apagado
- **Ambos activos** → LED sólido
- **Solo primer sensor** → Parpadeo lento (1 segundo)
- **Solo segundo sensor** → Parpadeo rápido (0.3 segundos)

### Gestión de Errores
- **WiFi desconectado**: Reconexión automática y LEDs apagados
- **Error en API**: Reintentos y patrón de error (3 parpadeos rápidos)
- **Timeout de red**: Manejo robusto de conexiones

## Instalación y Uso

### Requisitos
- Arduino IDE o PlatformIO
- Librerías requeridas (ver `libraries.txt`)
- ESP32 Board Package

### Pasos de Instalación
1. Clonar o descargar el proyecto
2. Abrir `sketch.ino` o `example_smart_sensors.ino` en Arduino IDE
3. Configurar credenciales WiFi, Device ID y User ID
4. Compilar y subir al ESP32
5. Abrir Monitor Serial (115200 baud) para ver logs

### Configuración de Usuario
Para que el dispositivo funcione correctamente, asegúrate de configurar:
- **USER_ID**: El ID del usuario en la base de datos de GeoEntry
- **DEVICE_ID**: El ID del dispositivo registrado en el sistema

### Monitoreo en Tiempo Real
```
=================================
  GeoEntry Smart Sensors Demo
=================================

📱 Configuración:
   - WiFi: Tu_Red_WiFi
   - Device ID: 7b4cdbcd-2bf0-4047-9355-05e33babf2c9
   - User ID: a8c6b41b-8c4d-4b8a-9e2f-1a3b5c7d9e0f
   - Proximidad: cada 5s
   - Sensores: cada 10s

🔍 Patrones de LEDs:
   LED Verde (TV/Luz):
     • Apagado: TV❌ Luz❌
     • Sólido: TV✅ Luz✅
     • Lento: TV✅ Luz❌
     • Rápido: TV❌ Luz✅

   LED Azul (AC/Cafetera):
     • Apagado: AC❌ Cafetera❌
     • Sólido: AC✅ Cafetera✅
     • Lento: AC✅ Cafetera❌
     • Rápido: AC❌ Cafetera✅

🚀 Sistema iniciado - monitoreando...
```

### Comandos del Monitor Serial
El ejemplo incluye comandos útiles para debugging:
- `status` - Mostrar estado del sistema
- `restart` - Reiniciar el dispositivo
- `check` - Forzar verificación de sensores
- `help` - Mostrar ayuda

## Archivos del Proyecto

```
geoentry-embedde-system/
├── sketch.ino                 # Archivo principal original de Arduino
├── example_smart_sensors.ino  # Ejemplo completo con sensores inteligentes
├── ModestIoT.h               # Header principal del framework
├── GeoEntryDevice.h/.cpp     # Clase principal del dispositivo (actualizada)
├── Device.h/.cpp             # Clase base del framework
├── Led.h/.cpp                # Actuador LED con patrones
├── Sensor.h/.cpp             # Clase base para sensores
├── Actuator.h/.cpp           # Clase base para actuadores
├── CommandHandler.h          # Interface para manejo de comandos
├── EventHandler.h            # Interface para manejo de eventos
├── libraries.txt             # Lista de librerías requeridas
├── wokwi-project.txt         # Configuración para simulador Wokwi
├── diagram.json              # Diagrama de conexiones
└── README.md                 # Esta documentación
```

## APIs Utilizadas

### Proximity Events API
```
GET https://geoentry-edge-api.onrender.com/api/v1/proximity-events/device/{deviceId}
```

### Sensors API  
```
GET https://geoentry-edge-api.onrender.com/api/v1/sensors/user/{userId}
```

## Integración con el Ecosistema GeoEntry

Este dispositivo IoT forma parte del ecosistema completo de GeoEntry:

1. **App Móvil**: Los usuarios pueden activar/desactivar sensores
2. **Web Dashboard**: Visualización y control desde el navegador
3. **REST API**: Backend principal con autenticación y gestión
4. **Edge API**: API optimizada para dispositivos IoT
5. **Dispositivo ESP32**: Este proyecto - representación física de estados

### Flujo de Datos
```
App/Web → REST API → Supabase → Edge API → ESP32 → LEDs
```

## Notas Técnicas

### Consideraciones de Red
- El dispositivo requiere conexión WiFi estable
- Timeout de red configurado en 10 segundos
- Reconexión automática en caso de fallo

### Optimizaciones de Energía
- Delays optimizados para reducir consumo
- Gestión eficiente de estados de LEDs
- Intervalos configurables de consulta
├── Led.h/.cpp              # Clase para control de LEDs
├── Device.h/.cpp           # Clase base abstracta para dispositivos
├── Actuator.h/.cpp         # Clase base para actuadores
├── Sensor.h/.cpp           # Clase base para sensores
├── EventHandler.h          # Interfaz para manejo de eventos
├── CommandHandler.h        # Interfaz para manejo de comandos
├── libraries.txt           # Lista de librerías requeridas
├── diagram.json            # Diagrama de circuito para Wokwi
└── wokwi-project.txt       # Configuración del proyecto Wokwi
```

## Simulación en Wokwi

El proyecto incluye configuración completa para simulación en [Wokwi](https://wokwi.com):
- Circuito predefinido con ESP32 y componentes
- Conexiones automáticas según `diagram.json`
- Librerías preconfiguradas

## Licencia

Creative Commons Attribution-NoDerivatives 4.0 International (CC BY-ND 4.0)

**Desarrollado por**: TechProtection - GeoEntry Development Team  
**Fecha**: Julio 4, 2025  
**Versión**: 1.0.0
