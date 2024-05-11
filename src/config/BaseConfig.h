#pragma once
#include <ArduinoJson.h>
#include "log.h"

class BaseConfig {
protected:
  static void defaultValue(JsonObject& object, const String& path, const char* value) {
    const char* current = object[path];
    if (!current) {
      object[path] = value;
    }
  }

  static void defaultValue(JsonObject& object, const String& path, uint32_t value) {
    uint32_t current = object[path];
    if (!current) {
      object[path] = value;
    }
  }

  static JsonObject object(JsonDocument& document, const String& path) {
    JsonObject current = document[path];
    if (!current) {
      current = document[path].to<JsonObject>();
    }
    return current;
  }
};
