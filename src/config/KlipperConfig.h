#pragma once
#include "BaseConfig.h"

class KlipperConfig : BaseConfig {
private:
  JsonObject object;

public:
  explicit KlipperConfig(JsonObject object) {
    LV_LOG_INFO("ctor");
    this->object = object;
    defaultValue(object, "host", "");
    defaultValue(object, "apiKey", "");
  }

  String getHost() { return this->object["host"]; }
  void setHost(String host) { this->object["host"] = host; }
  String getApiKey() { return this->object["apiKey"]; }
  void setApiKey(String apiKey) { this->object["apiKey"] = apiKey; }
};