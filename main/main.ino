#ifndef BOARD_HAS_PSRAM
#error "Please enable PSRAM, Arduino IDE -> tools -> PSRAM -> OPI !!!"
#endif

#include <Arduino.h>
#include "epd_driver.h"
#include "utilities.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "actual_creds.h"

// Fonts
#include "opensans24b.h"

GFXfont currentFont = OpenSans24B;
uint8_t *framebuffer = NULL;

// Global JSON doc to prevent dealloc, kinda stupid but rolling with it for now
DynamicJsonDocument doc(64 * 1024);

uint8_t startWifi() {
  Serial.println("\r\nConnecting to: " + String(ssid));
  IPAddress dns(8, 8, 8, 8);  // Use Google DNS
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);  // switch off AP
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("STA: Failed!\n");
    WiFi.disconnect(false);
    delay(500);
    WiFi.begin(ssid, password);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected at: " + WiFi.localIP().toString());
  } else Serial.println("WiFi connection failed");
  return WiFi.status();
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  startWifi();
  delay(1000);
  framebuffer = (uint8_t *)ps_calloc(sizeof(uint8_t), EPD_WIDTH * EPD_HEIGHT / 2);
  if (!framebuffer) {
    Serial.println("alloc memory failed !!!");
    while (1)
      ;
  }
  memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);

  epd_init();

  epd_poweron();
  epd_clear();
  epd_poweroff();

  epd_poweron();


  String text = "Home";

  int cursor_x = 20;
  int cursor_y = 50;

  draw_string(cursor_x, cursor_y, text);

  //get_text_bounds(&currentFont, data, &xx, &yy, &x1, &y1, &w, &h, NULL);

  WiFiClient client;

  JsonObject json = fetchJson(client, api_ip);

  const char *temperature = json["weather"]["temperature"];

  draw_string(cursor_x, cursor_y + 50, "Outside Temperature: " + String(temperature));

  JsonArray insideTemps = json["temperatures"];

  for (int i = 0; i < insideTemps.size(); i++) {
    JsonObject temp = insideTemps[i];
    String name = temp["name"];
    String value = temp["value"];
    draw_string(cursor_x, cursor_y + (50 * (i + 2)), name + ": " + value);
  }

  epd_update();
}

JsonObject fetchJson(WiFiClient &client, String ip) {
  // Terminate other request?
  client.stop();

  HTTPClient http;

  // Assume HTTP for now
  http.begin(client, ip, 3000);

  int httpCode = http.GET();

  if (httpCode != HTTP_CODE_OK) {
    // Fail?
    Serial.println("HTTP request failed, returned" + String(httpCode));
  }

  String jsonString = http.getString();

  Serial.println(jsonString);

  doc.clear();
  DeserializationError error = deserializeJson(doc, jsonString);
  if (error) {
    // Fail?
    Serial.println(error.c_str());
  }

  client.stop();
  http.end();

  return doc.as<JsonObject>();
}

void draw_string(int x, int y, String text) {
  char *data = const_cast<char *>(text.c_str());
  write_string(&currentFont, data, &x, &y, framebuffer);
}

void epd_update() {
  epd_draw_grayscale_image(epd_full_screen(), framebuffer);
}

void loop() {
  // no-op
}