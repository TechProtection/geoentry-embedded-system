# GeoEntry IoT Device - Sistema de Monitoreo de Proximidad

Este proyecto implementa un sistema de monitoreo de eventos de proximidad para GeoEntry usando el framework **ModestIoT** en un ESP32. El dispositivo consulta el Edge API de GeoEntry para detectar cuando un usuario entra o sale de casa y controla LEDs indicadores basado en estos eventos.

## Características Principales

- **Framework ModestIoT**: Implementación orientada a objetos con patrones Command y Observer
- **Conectividad WiFi**: Conexión automática y reconexión inteligente
- **Monitoreo en Tiempo Real**: Consulta periódica del API de GeoEntry cada 5 segundos
- **Indicadores LED**: Sistema visual de estado con 3 LEDs de colores
- **Gestión de Eventos**: Procesamiento de eventos de entrada y salida del hogar

## Componentes de Hardware

### Microcontrolador
- **ESP32 DevKit V1**: Controlador principal con WiFi integrado

### LEDs Indicadores
- **LED Rojo (Pin D2)**: Indica presencia en casa
  - 🔴 **ENCENDIDO**: Usuario está en casa
  - ⚫ **APAGADO**: Usuario fuera de casa

- **LED Verde (Pin D4)**: Estado del sistema
  - 🟢 **ENCENDIDO**: Sistema funcionando correctamente
  - 🟢 **PARPADEO LENTO**: Actividad del sistema
  - 🟢 **PARPADEO RÁPIDO**: Error en consulta API

- **LED Azul (Pin D5)**: Estado de conectividad
  - 🔵 **ENCENDIDO**: WiFi conectado
  - ⚫ **APAGADO**: WiFi desconectado
  - 🔵 **PARPADEO**: Intentando conectar/reconectar

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
const char* WIFI_PASSWORD = "";
```

### Configuración de Pines
```cpp
LED_PROXIMITY_PIN = 2  // D2 - LED Rojo
LED_STATUS_PIN = 4     // D4 - LED Verde  
LED_WIFI_PIN = 5       // D5 - LED Azul
```

## Funcionamiento del Sistema

### Ciclo Principal
1. **Inicialización**: Configuración de LEDs y conexión WiFi
2. **Monitoreo Continuo**: Consulta del API cada 5 segundos
3. **Procesamiento de Eventos**: Análisis de respuestas JSON
4. **Actualización de Estado**: Control de LEDs según eventos
5. **Reporte de Estado**: Salida periódica por consola serial

### Tipos de Eventos
- **`enter`**: Usuario entra a casa → LED rojo se enciende
- **`exit`**: Usuario sale de casa → LED rojo se apaga

### Gestión de Errores
- **WiFi desconectado**: Reconexión automática
- **Error en API**: Reintentos y indicación visual
- **Timeout de red**: Manejo robusto de conexiones

## Instalación y Uso

### Requisitos
- Arduino IDE o PlatformIO
- Librerías requeridas (ver `libraries.txt`)
- ESP32 Board Package

### Pasos de Instalación
1. Clonar o descargar el proyecto
2. Abrir `sketch.ino` en Arduino IDE
3. Configurar credenciales WiFi y parámetros del API
4. Compilar y subir al ESP32
5. Abrir Monitor Serial (115200 baud) para ver logs

### Monitoreo en Tiempo Real
```
Sistema GeoEntry listo!
Monitoreando eventos de proximidad...
=========================================
LEDs:
- Rojo (D2): Usuario en casa / fuera de casa
- Verde (D4): Estado del sistema y actividad
- Azul (D5): Estado de conexión WiFi
=========================================
```

## Archivos del Proyecto

```
geoentry-embedde-system/
├── sketch.ino              # Archivo principal de Arduino
├── ModestIoT.h             # Header principal del framework
├── GeoEntryDevice.h/.cpp   # Clase principal del dispositivo
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
