#pragma once
#include "BaseConfig.h"

class UIConfig : BaseConfig {
private:
  JsonObject object;

public:
  UIConfig(JsonObject object) {
    LV_LOG_INFO("ctor");
    this->object = object;
    defaultValue(object, "accentColor", 0xFF0000);
    defaultValue(object, "backgroundColor", (uint32_t)0x000000);
  }

  uint32_t getAccentColor() { return this->object["accentColor"]; }
  void setAccentColor(uint32_t accentColor) { this->object["accentColor"] = accentColor; }

  uint32_t getBackgroundColor() { return this->object["backgroundColor"]; }
  void setBackgroundColor(uint32_t backgroundColor) { this->object["backgroundColor"] = backgroundColor; }
};