#pragma once
#include "../config/NetworkConfig.h"
#include <ESPmDNS.h>

class WifiStation {
private:
  NetworkConfig *config;

public:
  WifiStation(NetworkConfig *config) {
    this->config = config;
    WiFi.setAutoReconnect(true);
    LV_LOG_INFO((String("Connecting to WIFI ") + config->getSsid() + " / " + config->getPsk()).c_str());
    WiFi.begin(config->getSsid(), config->getPsk());
    MDNS.begin(config->getHostname().c_str());
  }

  ~WifiStation()
  {
    MDNS.end();
    WiFi.disconnect(true);
  }
};