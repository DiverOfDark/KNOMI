#pragma once
#include "BaseConfig.h"
#include "KlipperConfig.h"
#include "NetworkConfig.h"
#include "UIConfig.h"
#include <LittleFS.h>

class Config : BaseConfig {
private:
  const char *configPath = "/config.json";

  JsonDocument doc = JsonDocument();

  NetworkConfig *networkConfig = nullptr;
  KlipperConfig *klipperConfig = nullptr;
  UIConfig *uiConfig = nullptr;

  void init() {
    delete networkConfig;
    delete klipperConfig;
    delete uiConfig;
    JsonObject networkObject = object(doc, "network");
    JsonObject klipperObject = object(doc, "klipper");
    JsonObject uiObject = object(doc, "ui");
    networkConfig = new NetworkConfig(networkObject);
    klipperConfig = new KlipperConfig(klipperObject);
    uiConfig = new UIConfig(uiObject);
    String buffer;
    serializeJson(doc, buffer);
    LV_LOG_INFO(buffer.c_str());
  }

public:
  Config() {
    fs::File file = LittleFS.open(configPath, "r", true);
    if (file) {
      const String &fileString = file.readString();
      LV_LOG_INFO("File:");
      LV_LOG_INFO(fileString.c_str());
      DeserializationError error = deserializeJson(doc, fileString.c_str());
      if (error) {
        LV_LOG_WARN("Failed to deserialize config: ");
        LV_LOG_WARN(error.c_str());
        doc.clear();
      }
    } else {
      doc.to<JsonObject>();
    }
    file.close();
    init();
  }

  NetworkConfig *getNetworkConfig() { return this->networkConfig; }
  KlipperConfig *getKlipperConfig() { return this->klipperConfig; }
  UIConfig *getUiConfig() { return this->uiConfig; }

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