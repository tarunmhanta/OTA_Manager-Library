# OTA_Manager — Reusable ESP32 OTA Library

**Exactly** as specified in your requirements.

## Quick Start (copy-paste)

```cpp
#include <WiFi.h>
#include <WebServer.h>
#include <OTA_Manager.h>

WebServer server(80);
OTA_Manager ota;

void setup() {
  Serial.begin(115200);
  WiFi.begin("SSID", "PASSWORD");
  while (WiFi.status() != WL_CONNECTED) delay(500);

  ota.setCurrentVersion("1.2.3");           // ← your firmware version
  ota.begin(server, "https://your-api.vercel.app");

  server.begin();
}

void loop() {
  server.handleClient();
}