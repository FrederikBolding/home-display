#ifndef BOARD_HAS_PSRAM
#error "Please enable PSRAM, Arduino IDE -> tools -> PSRAM -> OPI !!!"
#endif

#include <Arduino.h>
#include "epd_driver.h"
#include "utilities.h"
#include <WiFi.h>
#include <HTTPClient.h>

#include "actual_creds.h"

uint8_t *framebuffer = NULL;

uint8_t startWifi() {
  Serial.println("\r\nConnecting to: " + String(ssid));
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);  // switch off AP
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
}

bool fetch_image(WiFiClient &client, String ip) {
  // Terminate other request?
  client.stop();

  HTTPClient http;

  // Assume HTTP for now
  http.begin(client, ip, 3000, "/image");

  int httpCode = http.GET();

  if (httpCode != HTTP_CODE_OK) {
    // Fail?
    Serial.println("HTTP request failed, returned" + String(httpCode));
    return false;
  }

  WiFiClient *stream = http.getStreamPtr();

  size_t size = EPD_WIDTH * EPD_HEIGHT / 2;
  size_t index = 0;

  while (http.connected() && index < size) {
    if (stream->available()) {
      framebuffer[index++] = stream->read();
    } else {
      delay(1);
    }
  }

  client.stop();
  http.end();

  return true;
}

void epd_update() {
  epd_draw_grayscale_image(epd_full_screen(), framebuffer);
}

void loop() {
  WiFiClient client;

  bool result = fetch_image(client, api_ip);

  if (result) {
    epd_update();
  }

  delay(10000);
}