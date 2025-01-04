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
    defaultValue(object, "printPercentageMethod", "file-relative");
    defaultValue(object, "skipStandbyAlternate", "false");
  }

  String getHost() { return this->object["host"]; }
  void setHost(String host) { this->object["host"] = host; }
  String getApiKey() { return this->object["apiKey"]; }
  void setApiKey(String apiKey) { this->object["apiKey"] = apiKey; }
  String getPrintPercentageMethod() { return this->object["printPercentageMethod"]; }
  void setPrintPercentageMethod(String printPercentageMethod) {
    this->object["printPercentageMethod"] = printPercentageMethod;
  }
  bool getSkipStandbyAlternate() { return this->object["skipStandbyAlternate"] == "true"; }
  void setSkipStandbyAlternate(String skipStandbyAlternate) {
    this->object["skipStandbyAlternate"] = skipStandbyAlternate;
  }
};