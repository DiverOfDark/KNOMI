#pragma once
#include "BaseConfig.h"

class NetworkConfig : BaseConfig {
private:
  JsonObject object;

public:
  explicit NetworkConfig(JsonObject obj) {
    LV_LOG_INFO("ctor");

    this->object = obj;
    defaultValue(object, "ssid", "");
    defaultValue(object, "psk", "");
    defaultValue(object, "hostname", "KNOMI");
  }

  String getSsid() { return this->object["ssid"]; }
  void setSsid(String ssid) { this->object["ssid"] = ssid; }
  String getPsk() { return this->object["psk"]; }
  void setPsk(String psk) { this->object["psk"] = psk; }
  String getHostname() { return this->object["hostname"]; }
  void setHostname(String hostname) { this->object["hostname"] = hostname; }
};