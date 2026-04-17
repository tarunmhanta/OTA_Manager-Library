#include "ota_update.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>

bool performFirmwareUpdate(const char* backendUrl, const String& version) {
  if (WiFi.status() != WL_CONNECTED || !backendUrl || strlen(backendUrl) == 0) {
    Serial.println("[OTA] WiFi or backend not ready");
    return false;
  }

  String url = String(backendUrl) + "/api/firmware?version=" + version;
  Serial.printf("[OTA] Downloading: %s\n", url.c_str());

  HTTPClient http;
  http.setInsecure();           // HTTPS support (common for Vercel/etc.)
  http.setTimeout(60000);

  if (!http.begin(url)) {
    Serial.println("[OTA] http.begin() failed");
    return false;
  }

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("[OTA] HTTP error %d\n", httpCode);
    http.end();
    return false;
  }

  int totalLength = http.getSize();
  if (totalLength <= 0) {
    Serial.println("[OTA] Invalid firmware size");
    http.end();
    return false;
  }

  Serial.printf("[OTA] Firmware size: %d bytes\n", totalLength);

  WiFiClient* stream = http.getStreamPtr();
  if (!Update.begin(totalLength)) {
    Serial.printf("[OTA] Update.begin() failed: %s\n", Update.errorString().c_str());
    http.end();
    return false;
  }

  uint8_t buffer[1024];
  size_t written = 0;

  while (written < totalLength && stream->connected()) {
    size_t available = stream->available();
    if (available > 0) {
      size_t toRead = (available > sizeof(buffer)) ? sizeof(buffer) : available;
      size_t bytesRead = stream->read(buffer, toRead);
      if (bytesRead > 0) {
        if (Update.write(buffer, bytesRead) != bytesRead) {
          Serial.println("[OTA] Write to flash failed");
          break;
        }
        written += bytesRead;
      }
    } else {
      delay(1);
    }
  }

  bool success = (written == totalLength) && Update.end(true);

  if (success) {
    Serial.println("[OTA] SUCCESS — Firmware installed");
  } else {
    Serial.printf("[OTA] FAILED — Written %zu/%d | Error: %s\n", written, totalLength, Update.errorString().c_str());
  }

  http.end();
  return success;
}