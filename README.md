# Minimalist E-Paper Weather Station

Estación meteorológica de escritorio de bajo consumo basada en **ESP32-C3 SuperMini** y una pantalla de tinta electrónica **WeAct Studio 1.54" (200×200 px)**. Cuenta con una interfaz minimalista inspirada en los widgets de Apple, portal cautivo de configuración Wi-Fi, consulta a la API abierta de Open-Meteo y una carcasa diseñada en OpenSCAD alimentada por una celda 18650.

![Platform](https://img.shields.io/badge/Platform-ESP32--C3-red?style=flat-square)
![Framework](https://img.shields.io/badge/Framework-Arduino-blue?style=flat-square)
![IDE](https://img.shields.io/badge/IDE-PlatformIO-orange?style=flat-square)
![License](https://img.shields.io/badge/License-MIT-green?style=flat-square)

---

## Características

* **Diseño minimalista:** Tipografías vectoriales `FreeSans` e iconos meteorológicos vectoriales limpios (SF Symbols style).
* **API abierta (sin registros):** Consulta directa a [Open-Meteo](https://open-meteo.com/) (temperatura actual, humedad, condición del día y máximas/mínimas) sin necesidad de API keys.
* **Conectividad híbrida + Portal Cautivo:** Intenta conectarse a la red predefinida en el código. Si no está disponible, levanta un punto de acceso Wi-Fi (`Meteo-Config-AP`) con interfaz web para guardar credenciales en flash vía `WiFiManager`.
* **Bajo consumo y autonomía:** Optimizado para pantalla E-Paper (retención de imagen sin consumo) y alimentación mediante batería de litio 18650 recargable.
* **Carcasa 3D paramétrica:** Modelo en OpenSCAD optimizado para impresión sin soportes y acabado en base texturizada.

---

## Lista de Materiales (BOM)

| Componente | Descripción |
| :--- | :--- |
| **ESP32-C3 SuperMini** | Microcontrolador RISC-V con conectividad Wi-Fi/BLE |
| **WeAct E-Paper 1.54"** | Pantalla de tinta electrónica SPI 200×200 px (controlador SSD1681) |
| **Módulo TP4056 USB-C** | Módulo de carga y protección para batería de litio |
| **Batería 18650** | Celda Li-Ion 3.7V (2500–3200 mAh) |
| **Tornillería** | 4 tornillos autorroscantes M2 × 6 mm |
| **Filamento** | PLA mate o PETG para la carcasa |

---

## Conexiones y Esquema Eléctrico

### Pantalla E-Paper a ESP32-C3 SuperMini

| Pin WeAct 1.54" | Pin ESP32-C3 | Función |
| :--- | :--- | :--- |
| **VCC** (Pin 8) | **3.3V** | Alimentación lógica |
| **GND** (Pin 7) | **GND** | Masa común |
| **SDA** (Pin 6) | **GPIO 6** | SPI MOSI |
| **SCL** (Pin 5) | **GPIO 4** | SPI SCK |
| **CS** (Pin 4) | **GPIO 7** | Chip Select |
| **D/C** (Pin 3) | **GPIO 3** | Data / Command |
| **RES** (Pin 2) | **GPIO 2** | Reset del panel |
| **BUSY** (Pin 1) | **GPIO 5** | Estado del bus |

### Alimentación (Batería y Cargador)

* **Batería 18650** $\rightarrow$ Terminales `B+` y `B-` del módulo **TP4056**.
* **Salida TP4056** $\rightarrow$ `OUT+` al pin **5V** del ESP32-C3; `OUT-` a **GND**.

---

## Configuración del Software

El proyecto está preparado para compilar directamente con **PlatformIO** (extensión de VS Code).

### 1. Estructura del Proyecto

```text
├── 3d_case/
│   └── meteo_case_18650.scad   # Archivo paramétrico de la carcasa
├── src/
│   └── main.cpp                # Código principal
├── platformio.ini              # Dependencias y configuración de la placa
└── README.md
```

### 2. Configuración de PlatformIO (platformio.ini)

```text
[env:esp32-c3-supermini]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino
monitor_speed = 115200
build_flags = 
    -D ARDUINO_USB_MODE=1
    -D ARDUINO_USB_CDC_ON_BOOT=1
lib_deps = 
    zinggjm/GxEPD2 @ ^1.5.8
    adafruit/Adafruit GFX Library @ ^1.11.9
    bblanchon/ArduinoJson @ ^7.0.4
    [https://github.com/tzapu/WiFiManager.git](https://github.com/tzapu/WiFiManager.git)
```

### 3. Ajuste de Parámetros de Usuario

Edita las primeras líneas de src/main.cpp con tus datos locales:
```c++

// Credenciales por defecto (opcional)
const char* DEFAULT_SSID = "MiRedWiFi";
const char* DEFAULT_PASS = "MiPassword";

// Nombre en cabecera y coordenadas GPS para Open-Meteo
const char* LOCATION_LABEL = "BARCELONA";
const float LATITUDE       = 41.3887;
const float LONGITUDE      = 2.1589;
```

### Primer Arranque y Funcionamiento
1. Conecta el ESP32-C3 por USB y haz Upload desde PlatformIO.
2. Si las credenciales por defecto no conectan, la pantalla mostrará una pantalla con instrucciones para acceder a la red Meteo-Config-AP.
3. Conéctate a dicha red desde el móvil o portátil y accede a http://192.168.4.1.
4. Selecciona tu red doméstica, ingresa la clave y guarda. El ESP32 almacenará los datos en la memoria flash NVS y descargará la predicción meteorológica.
5. El panel entrará en hibernación eléctrica tras actualizar los datos para conservar la batería.