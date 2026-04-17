#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <WebServer.h>

class OTA_Manager {
public:
    void begin(WebServer &server);
};

extern OTA_Manager ota;

#endif