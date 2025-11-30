#pragma once
#include "BaseConfig.h"

class SecurityConfig : BaseConfig {
private:
  JsonObject object;
  String adminPassword;

public:
  explicit SecurityConfig(JsonObject obj) {
    LV_LOG_INFO("ctor");
    this->object = obj;
    // default admin password
    defaultValue(object, "adminPassword", "KNOMI");
    adminPassword = String(static_cast<const char *>(object["adminPassword"]));
  }

  const String &getAdminPassword() const { return adminPassword; }

  void setAdminPassword(const String &pwd) {
    object["adminPassword"] = pwd;
    adminPassword = pwd;
  }
};
