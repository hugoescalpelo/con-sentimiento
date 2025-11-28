/*
Con-Sentimiento.
Fecha: 2025-11-23
By: Luma Escalpelo
Para: e-Cuerpo

Este programa es el ejemplo base para el manejo de la pantal SSD1306 con una resolución de 128x64 y 128x32.

Este programa esta escrito especifícamente para el ESP32 DevKitV1, encontrado en la lista de microtroladores de Espressif como DOIT ESP32.
La versión de core necesita es 3.3.4.

Es necesario que no haya instalada otras versiones de la biblioteca Async TCP y ESP Async Web Server, únicamente las de ESP32Async.
*/

// Proyecto: Con-Sentimiento COMPLETO con frases, scroll, captive portal y robusta aleatorización
#include <WiFi.h> // Biblioteca Wifi para el ESP32 DevKit V1 y similares
#include <DNSServer.h>
#include <AsyncTCP.h> // by ESP32Async, 3.4.9 o superior
#include <ESPAsyncWebServer.h> // by ESP32Async, 3.9.0 o superior
#include <Adafruit_GFX.h> // by Adafruit
#include <Adafruit_SSD1306.h> // by Adafruit

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char* ssid = "puer-tita";
const char* password = "llave-cita";
DNSServer dns;
AsyncWebServer server(80);

// Frases OLED
const char* frases_oled[] = {
  "Me llamo puer-tita, mi clave es llave-cita", "Feliz Navidad, mandame un mensaje.", "Sonrie, sera un buen dia."
};
const int total_frases_oled = sizeof(frases_oled)/sizeof(frases_oled[0]);

// Frases captive
const char* frases_captive[] = {
  "Deja un mensaje.", "Tu mensaje quedara en mi pantallita.", "Manda saludos."
};
const int total_frases_captive = sizeof(frases_captive)/sizeof(frases_captive[0]);

char mensaje[128];
bool mensaje_actualizado = true;
int16_t scrollX = 0;
uint16_t textoWidth = 0;
unsigned long lastScroll = 0;
const int scrollSpeed = 5;
int frase_captive_idx;

void calcularAnchoTexto(const char* text) {
  int16_t x1, y1; uint16_t w, h;
  display.setTextSize(2);
  display.setTextWrap(false);
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  textoWidth = w;
}

void mostrarMarquesina(const char* text) {
  if (strlen(text) == 0) {
    strcpy(mensaje, frases_oled[random(total_frases_oled)]);
    mensaje_actualizado = true;
    return;
  }
  display.clearDisplay();
  display.setRotation(0); // 0=0°, 1=90°, 2=180°, 3=270°
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setTextWrap(false);
  int y = (SCREEN_HEIGHT - 16) / 2;
  if (textoWidth <= SCREEN_WIDTH) {
    int x = (SCREEN_WIDTH - textoWidth) / 2;
    display.setCursor(x, y);
    display.print(text);
  } else {
    int offset = scrollX % (textoWidth + 20);
    display.setCursor(-offset, y);
    display.print(text);
    display.print("   ");
    display.print(text);
  }
  display.display();
}

void actualizarScroll() {
  if (textoWidth > SCREEN_WIDTH && millis() - lastScroll > scrollSpeed) {
    lastScroll = millis();
    scrollX++;
    if (scrollX > textoWidth + 20) scrollX = 0;
  }
}

void handleCaptive(AsyncWebServerRequest *request) {
  String html = "<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Puer-Tita</title><style>";
  html += "body{margin:0;font-family:sans-serif;background:linear-gradient(135deg,#f7c5cc,#b8dfe6);color:#333;display:flex;flex-direction:column;align-items:center;justify-content:center;height:100vh;}";
  html += "h1{font-size:2.5em;margin-bottom:0.3em;text-align:center;}p{font-size:1.2em;max-width:90%;text-align:center;margin-bottom:2em;}";
  html += "form{background:rgba(255,255,255,0.9);padding:20px;border-radius:12px;box-shadow:0 4px 15px rgba(0,0,0,0.2);display:flex;flex-direction:column;align-items:center;}";
  html += "textarea{width:250px;height:100px;margin-bottom:15px;font-size:1em;padding:10px;border-radius:8px;border:1px solid #ccc;}";
  html += "input[type='submit']{background:#d87ca0;color:white;border:none;padding:12px 20px;border-radius:8px;font-size:1.1em;cursor:pointer;transition:background 0.3s;}input[type='submit']:hover{background:#bd6e8f;}";
  html += "</style></head><body>";
  html += "<h1>Puer-Tita</h1><p>" + String(frases_captive[frase_captive_idx]) + "</p>";
  html += "<form action='/send' method='POST'><textarea name='mensaje' placeholder='Escribe un mensaje bonito'></textarea><input type='submit' value='Enviar mensaje'></form></body></html>";
  request->send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) while (true);
  randomSeed(esp_random());
  strcpy(mensaje, frases_oled[random(total_frases_oled)]);
  frase_captive_idx = random(total_frases_captive);
  calcularAnchoTexto(mensaje);
  mostrarMarquesina(mensaje);
  WiFi.softAP(ssid, password);
  delay(100);
  dns.start(53, "*", WiFi.softAPIP());
  server.on("/", HTTP_GET, handleCaptive);
  server.on("/generate_204", HTTP_GET, handleCaptive);
  server.on("/hotspot-detect.html", HTTP_GET, handleCaptive);
  server.on("/ncsi.txt", HTTP_GET, handleCaptive);
  server.on("/send", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("mensaje", true)) {
      String nuevo = request->getParam("mensaje", true)->value();
      nuevo.trim();
      nuevo.toCharArray(mensaje, sizeof(mensaje));
      mensaje_actualizado = true;
      frase_captive_idx = random(total_frases_captive);
    }
    request->redirect("/");
  });
  server.begin();
}

void loop() {
  dns.processNextRequest();
  if (mensaje_actualizado) {
    calcularAnchoTexto(mensaje);
    scrollX = 0;
    lastScroll = millis();
    mensaje_actualizado = false;
  }
  actualizarScroll();
  mostrarMarquesina(mensaje);
  yield();
}
