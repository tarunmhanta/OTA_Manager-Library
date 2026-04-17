#include "OTA_Manager.h"
#include "ota_web.h"
#include "ota_update.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

OTA_Manager::OTA_Manager() {}

void OTA_Manager::setCurrentVersion(const char* version) {
  if (version && strlen(version) > 0) {
    _currentVersion = version;
  }
}

void OTA_Manager::begin(WebServer& server, const char* backendUrl) {
  _server = &server;
  _backendUrl = (backendUrl != nullptr) ? backendUrl : "";

  // Register routes (non-intrusive)
  _server->on("/ota", HTTP_GET, [this]() { this->handleRootOTA(); });
  _server->on("/ota/info", HTTP_GET, [this]() { this->handleInfo(); });
  _server->on("/ota/install", HTTP_GET, [this]() { this->handleInstall(); });

  Serial.println("OTA_Manager initialized - routes registered: /ota, /ota/info, /ota/install");
}

void OTA_Manager::handleRootOTA() {
  _server->send_P(200, "text/html", otaIndexHTML);
}

void OTA_Manager::handleInfo() {
  DynamicJsonDocument doc(2048);

  if (WiFi.status() != WL_CONNECTED || _backendUrl.length() == 0) {
    doc["error"] = "WiFi not connected or backend URL not configured";
    String output;
    serializeJson(doc, output);
    _server->send(200, "application/json", output);
    return;
  }

  String url = _backendUrl + "/api/firmware-info";
  HTTPClient http;
  http.setInsecure();
  http.setTimeout(10000);

  if (http.begin(url)) {
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      DynamicJsonDocument backendDoc(2048);
      DeserializationError err = deserializeJson(backendDoc, payload);

      if (!err) {
        if (backendDoc.containsKey("success") && !backendDoc["success"].as<bool>()) {
          doc["error"] = backendDoc["error"] | "Backend returned error";
        } else {
          doc["current_version"] = _currentVersion;
          doc["latest_version"] = backendDoc["latest_version"] | "N/A";
          if (backendDoc.containsKey("versions")) {
            doc["versions"] = backendDoc["versions"];
          }
        }
      } else {
        doc["error"] = "Invalid JSON from backend";
      }
    } else {
      doc["error"] = String("Backend HTTP error: ") + httpCode;
    }
    http.end();
  } else {
    doc["error"] = "Failed to connect to backend";
  }

  if (doc["error"].isNull() && doc["versions"].isNull()) {
    doc["error"] = "No firmware data received";
  }

  String output;
  serializeJson(doc, output);
  _server->send(200, "application/json", output);
}

void OTA_Manager::handleInstall() {
  if (!_server->hasArg("version")) {
    DynamicJsonDocument doc(512);
    doc["error"] = "Missing version parameter";
    String output;
    serializeJson(doc, output);
    _server->send(400, "application/json", output);
    return;
  }

  String version = _server->arg("version");
  if (version.length() == 0) {
    DynamicJsonDocument doc(512);
    doc["error"] = "Version cannot be empty";
    String output;
    serializeJson(doc, output);
    _server->send(400, "application/json", output);
    return;
  }

  bool success = performFirmwareUpdate(_backendUrl.c_str(), version);

  DynamicJsonDocument doc(512);
  if (success) {
    doc["success"] = true;
    doc["message"] = "Firmware updated successfully. Restarting...";
    String output;
    serializeJson(doc, output);
    _server->send(200, "application/json", output);
    delay(800);           // Ensure response is sent before restart
    ESP.restart();
  } else {
    doc["error"] = "Update failed. Check Serial Monitor for details.";
    String output;
    serializeJson(doc, output);
    _server->send(500, "application/json", output);
  }
}