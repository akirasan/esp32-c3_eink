#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <GxEPD2_BW.h>

// Tipografías vectoriales estilo San Francisco / Helvetica
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>

// ==========================================
// 1. CREDENCIALES POR DEFECTO Y UBICACIÓN
// ==========================================
#include "config.h"
// Usar las macros definidas en config.h:
const char* DEFAULT_SSID   = WIFI_SSID;
const char* DEFAULT_PASS   = WIFI_PASS;
const char* LOCATION_LABEL = LOCATION_NAME;
const float LATITUDE       = LOCATION_LAT;
const float LONGITUDE      = LOCATION_LON;

const unsigned long UPDATE_INTERVAL_MIN = 30; // Frecuencia de refresco

// ==========================================
// CONFIGURACIÓN DE PINES
// ==========================================
#define EPD_CS    7
#define EPD_DC    3
#define EPD_RST   2
#define EPD_BUSY  5

GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(
    GxEPD2_154_D67(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY)
);

// ==========================================
// FUNCIONES GRÁFICAS Y TIPOGRÁFICAS
// ==========================================
void printCentered(const String &text, int y, const GFXfont *font) {
    display.setFont(font);
    display.setTextColor(GxEPD_BLACK);
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
    display.setCursor((200 - w) / 2, y);
    display.print(text);
}

void drawHeroTemperature(int temp, int y) {
    display.setFont(&FreeSansBold24pt7b);
    display.setTextColor(GxEPD_BLACK);
    String tStr = String(temp);
    
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(tStr, 0, y, &x1, &y1, &w, &h);
    
    int totalW = w + 14;
    int startX = (200 - totalW) / 2;
    
    display.setCursor(startX, y);
    display.print(tStr);
    
    int degX = startX + w + 7;
    int degY = y - h + 6;
    display.drawCircle(degX, degY, 4, GxEPD_BLACK);
    display.drawCircle(degX, degY, 3, GxEPD_BLACK);
}

// Iconografía estilo Apple / SF Symbols
void drawAppleSun(int cx, int cy) {
    display.fillCircle(cx, cy, 9, GxEPD_BLACK);
    for (int i = 0; i < 8; i++) {
        float angle = i * (PI / 4.0);
        int x1 = cx + cos(angle) * 13;
        int y1 = cy + sin(angle) * 13;
        int x2 = cx + cos(angle) * 17;
        int y2 = cy + sin(angle) * 17;
        display.drawLine(x1, y1, x2, y2, GxEPD_BLACK);
        display.drawLine(x1 + (i % 2 == 0 ? 0 : 1), y1, x2 + (i % 2 == 0 ? 0 : 1), y2, GxEPD_BLACK);
    }
}

void drawAppleCloud(int cx, int cy) {
    display.fillRoundRect(cx - 18, cy + 2, 36, 12, 6, GxEPD_BLACK);
    display.fillCircle(cx - 7, cy + 2, 9, GxEPD_BLACK);
    display.fillCircle(cx + 6, cy - 1, 12, GxEPD_BLACK);
}

void drawAppleSunCloud(int cx, int cy) {
    display.fillCircle(cx + 9, cy - 8, 8, GxEPD_BLACK);
    display.drawLine(cx + 9, cy - 19, cx + 9, cy - 16, GxEPD_BLACK);
    display.drawLine(cx + 19, cy - 8, cx + 16, cy - 8, GxEPD_BLACK);
    display.drawLine(cx + 16, cy - 15, cx + 14, cy - 13, GxEPD_BLACK);

    display.fillRoundRect(cx - 20, cy, 40, 16, 8, GxEPD_WHITE);
    display.fillCircle(cx - 7, cy + 2, 11, GxEPD_WHITE);
    display.fillCircle(cx + 6, cy - 1, 14, GxEPD_WHITE);

    drawAppleCloud(cx, cy);
}

void drawAppleRain(int cx, int cy) {
    drawAppleCloud(cx, cy - 6);
    for (int dx = -10; dx <= 10; dx += 10) {
        display.drawLine(cx + dx, cy + 12, cx + dx - 2, cy + 18, GxEPD_BLACK);
        display.drawLine(cx + dx + 1, cy + 12, cx + dx - 1, cy + 18, GxEPD_BLACK);
    }
}

void drawAppleThunder(int cx, int cy) {
    drawAppleCloud(cx, cy - 6);
    display.drawLine(cx, cy + 10, cx - 4, cy + 17, GxEPD_BLACK);
    display.drawLine(cx - 4, cy + 17, cx + 2, cy + 17, GxEPD_BLACK);
    display.drawLine(cx + 2, cy + 17, cx - 2, cy + 24, GxEPD_BLACK);
}

void drawWeatherIcon(int code, int cx, int cy) {
    if (code == 0) {
        drawAppleSun(cx, cy);
    } else if (code == 1 || code == 2) {
        drawAppleSunCloud(cx, cy);
    } else if (code == 3 || code == 45 || code == 48) {
        drawAppleCloud(cx, cy);
    } else if ((code >= 51 && code <= 67) || (code >= 80 && code <= 82)) {
        drawAppleRain(cx, cy);
    } else if (code >= 95) {
        drawAppleThunder(cx, cy);
    } else {
        drawAppleCloud(cx, cy);
    }
}

String getWeatherDesc(int code) {
    if (code == 0) return "Despejado";
    if (code == 1) return "Mayormente claro";
    if (code == 2) return "Nubosidad parcial";
    if (code == 3) return "Nublado";
    if (code == 45 || code == 48) return "Niebla";
    if (code >= 51 && code <= 55) return "Llovizna";
    if (code >= 61 && code <= 67) return "Lluvia";
    if (code >= 71 && code <= 77) return "Nieve";
    if (code >= 80 && code <= 82) return "Chubascos";
    if (code >= 95) return "Tormenta";
    return "Nublado";
}

// ==========================================
// RENDERIZADO DE PANTALLAS
// ==========================================
void showConfigPortalScreen(WiFiManager *wm) {
    display.setRotation(3);
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        // Icono WiFi minimalista
        display.fillCircle(100, 32, 3, GxEPD_BLACK);
        display.drawCircle(100, 32, 9, GxEPD_BLACK);
        display.drawCircle(100, 32, 16, GxEPD_BLACK);
        display.fillRect(70, 33, 60, 20, GxEPD_WHITE);

        printCentered("Configurar Wi-Fi", 62, &FreeSansBold9pt7b);

        display.drawRoundRect(14, 76, 172, 70, 8, GxEPD_BLACK);
        display.setFont(&FreeSans9pt7b);
        display.setCursor(24, 96);
        display.print("Conectar a red:");
        
        display.setFont(&FreeSansBold9pt7b);
        display.setCursor(24, 116);
        display.print("Meteo-Config-AP");

        display.setFont(&FreeSans9pt7b);
        display.setCursor(24, 134);
        display.print("IP: 192.168.4.1");

        printCentered("Esperando red...", 176, &FreeSans9pt7b);
    } while (display.nextPage());
    display.hibernate();
}

void renderAppleMeteoScreen(float temp, int hum, int code, float tMax, float tMin) {
    display.setRotation(3);
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        // 1. Cabecera
        printCentered(LOCATION_LABEL, 24, &FreeSansBold9pt7b);

        // 2. Icono central
        drawWeatherIcon(code, 100, 52);

        // 3. Temperatura destacada
        drawHeroTemperature((int)round(temp), 108);

        // 4. Condición
        printCentered(getWeatherDesc(code), 128, &FreeSans9pt7b);

        // 5. Línea sutil
        display.drawFastHLine(36, 145, 128, GxEPD_BLACK);

        // 6. Rango diario
        String minMaxStr = "H: " + String((int)round(tMax)) + "   L: " + String((int)round(tMin));
        printCentered(minMaxStr, 166, &FreeSansBold9pt7b);

        // 7. Humedad
        String humStr = "Humedad  " + String(hum) + "%";
        printCentered(humStr, 185, &FreeSans9pt7b);

    } while (display.nextPage());

    display.hibernate();
}

// ==========================================
// API OPEN-METEO
// ==========================================
bool updateWeather() {
    if (WiFi.status() != WL_CONNECTED) return false;

    HTTPClient http;
    String url = "http://api.open-meteo.com/v1/forecast?latitude=" + String(LATITUDE, 4) +
                 "&longitude=" + String(LONGITUDE, 4) +
                 "&current=temperature_2m,relative_humidity_2m,weather_code" +
                 "&daily=temperature_2m_max,temperature_2m_min&timezone=auto";

    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
            float currentTemp = doc["current"]["temperature_2m"];
            int currentHum   = doc["current"]["relative_humidity_2m"];
            int weatherCode  = doc["current"]["weather_code"];
            float maxTemp    = doc["daily"]["temperature_2m_max"][0];
            float minTemp    = doc["daily"]["temperature_2m_min"][0];

            renderAppleMeteoScreen(currentTemp, currentHum, weatherCode, maxTemp, minTemp);
            http.end();
            return true;
        }
    }
    http.end();
    return false;
}

// ==========================================
// CONEXIÓN HÍBRIDA
// ==========================================
void connectWiFiHybrid() {
    WiFi.mode(WIFI_STA);
    
    // Intenta primero con credenciales en código si no hay una conectada
    Serial.println("Intentando conexion con la red predefinida...");
    WiFi.begin(DEFAULT_SSID, DEFAULT_PASS);

    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 25) { // Espera ~12.5 seg
        delay(500);
        Serial.print(".");
        timeout++;
    }
    Serial.println();

    // Si no conectó, levanta el portal cautivo
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("No se pudo conectar. Iniciando Portal Cautivo...");
        WiFiManager wm;
        wm.setAPCallback(showConfigPortalScreen);
        wm.setConfigPortalTimeout(180); // 3 minutos activo

        if (!wm.startConfigPortal("Meteo-Config-AP")) {
            Serial.println("Timeout en el portal. Reiniciando...");
            delay(2000);
            ESP.restart();
        }
    }

    Serial.println("Conectado con exito! IP: " + WiFi.localIP().toString());
}

// ==========================================
// SETUP & LOOP
// ==========================================
unsigned long lastUpdateMillis = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);

    display.init(115200);

    // Conexión híbrida
    connectWiFiHybrid();

    // Descargar datos y pintar pantalla
    updateWeather();
    lastUpdateMillis = millis();
}

void loop() {
    if (millis() - lastUpdateMillis >= (UPDATE_INTERVAL_MIN * 60 * 1000UL)) {
        lastUpdateMillis = millis();
        Serial.println("Actualizando meteorologia...");
        updateWeather();
    }
    delay(1000);
}