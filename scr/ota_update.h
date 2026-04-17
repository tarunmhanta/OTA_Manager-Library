#ifndef OTA_UPDATE_H
#define OTA_UPDATE_H

#include <Arduino.h>

bool performFirmwareUpdate(const char* backendUrl, const String& version);

#endif