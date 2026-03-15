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
    // Log each component's config with sensitive values obscured
    auto maskSecret = [](const String &s) -> String {
      if (s.isEmpty())
        return String("(empty)");
      if (s.length() <= 2)
        return String("**");
      String out;
      out.reserve(s.length());
      out += s.substring(0, 1);
      for (size_t i = 0; i < s.length() - 2; ++i)
        out += '*';
      out += s.substring(s.length() - 1);
      return out;
    };

    // Network
    {
      String ssid = networkConfig->getSsid();
      String psk = networkConfig->getPsk();
      String hostname = networkConfig->getHostname();
      LV_LOG_INFO("Network: ssid=%s, psk=%s (len=%u), hostname=%s", ssid.c_str(), maskSecret(psk).c_str(),
                  static_cast<unsigned>(psk.length()), hostname.c_str());
    }

    // Klipper
    {
      String host = klipperConfig->getHost();
      String apiKey = klipperConfig->getApiKey();
      String ppm = klipperConfig->getPrintPercentageMethod();
      bool skip = klipperConfig->getSkipStandbyAlternate();
      LV_LOG_INFO("Klipper: host=%s, apiKey=%s (len=%u), printPercentageMethod=%s, skipStandbyAlternate=%s",
                  host.c_str(), maskSecret(apiKey).c_str(), static_cast<unsigned>(apiKey.length()), ppm.c_str(),
                  skip ? "true" : "false");
    }

    // UI
    {
      uint32_t accent = uiConfig->getAccentColor();
      uint32_t bg = uiConfig->getBackgroundColor();
      LV_LOG_INFO("UI: accentColor=0x%08X, backgroundColor=0x%08X", static_cast<unsigned>(accent),
                  static_cast<unsigned>(bg));
    }

    // Security
    {
      String admin = securityConfig->getAdminPassword();
      LV_LOG_INFO("Security: adminPassword=%s (len=%u)", maskSecret(admin).c_str(),
                  static_cast<unsigned>(admin.length()));
    }
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