#pragma once
#include "BaseConfig.h"
#include "KlipperConfig.h"
#include "NetworkConfig.h"
#include "SecurityConfig.h"
#include "UIConfig.h"
#include <LittleFS.h>

class Config : public BaseConfig {
private:
  const char *configPath = "/config.json";

  JsonDocument doc = JsonDocument();

  NetworkConfig *networkConfig = nullptr;
  KlipperConfig *klipperConfig = nullptr;
  UIConfig *uiConfig = nullptr;
  SecurityConfig *securityConfig = nullptr;

  void init() {
    delete networkConfig;
    delete klipperConfig;
    delete uiConfig;
    delete securityConfig;
    JsonObject networkObject = object(doc, "network");
    JsonObject klipperObject = object(doc, "klipper");
    JsonObject uiObject = object(doc, "ui");
    JsonObject securityObject = object(doc, "security");
    networkConfig = new NetworkConfig(networkObject);
    klipperConfig = new KlipperConfig(klipperObject);
    uiConfig = new UIConfig(uiObject);
    securityConfig = new SecurityConfig(securityObject);
    String buffer;
    serializeJson(doc, buffer);
    LV_LOG_INFO(buffer.c_str()); // TODO let each component log its config and obscure PSK
  }

public:
  Config() {
    fs::File file = LittleFS.open(configPath, "r");
    if (file) {
      const String &fileString = file.readString();
      LV_LOG_INFO("File: %s", fileString.c_str());
      DeserializationError error = deserializeJson(doc, fileString.c_str());
      if (error) {
        LV_LOG_WARN("Failed to deserialize config: %s", error.c_str());
        doc.clear();
      }
    } else {
      LV_LOG_INFO("No file loaded, creating new config");
      doc.to<JsonObject>();
    }
    file.close();
    init();
  }

  NetworkConfig *getNetworkConfig() { return this->networkConfig; }
  KlipperConfig *getKlipperConfig() { return this->klipperConfig; }
  UIConfig *getUiConfig() { return this->uiConfig; }
  SecurityConfig *getSecurityConfig() { return this->securityConfig; }

  void reset() {
    LV_LOG_INFO("reset");
    LittleFS.remove(configPath);
    doc.clear();
    init();
  }

  void save() {
    LV_LOG_INFO("save");
    fs::File configFile = LittleFS.open(configPath, "w", true);
    if (configFile) {
      serializeJson(doc, configFile);
      configFile.flush();
      configFile.close();

      LV_LOG_INFO("file saved");
    }
  }
};