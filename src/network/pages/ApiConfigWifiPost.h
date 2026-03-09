#pragma once
#include "AbstractPage.h"

class ApiConfigWifiPost : public AbstractPage {
  WifiManager *wifimanager = nullptr;
  Config *config = nullptr;

public:
  explicit ApiConfigWifiPost(WifiManager *manager, httpd_handle_t server, Config *config)
      : AbstractPage(server, HTTP_POST, "/api/configwifi") {
    this->wifimanager = manager;
    this->config = config;
  }

protected:
  bool requiresAuth() override { return true; }

public:
  esp_err_t handler(httpd_req_t *req) override {
    String ssid;
    bool setSsid = false;

    String pass;
    bool setPass = false;

    String hostname;
    bool setHostname = false;

    String klipper;
    bool setKlipper = false;

    String printProgressMethod;
    bool setPrintProgressMethod = false;

    String skipStandbyAlternation;
    bool setSkipStandbyAlternation = false;

    if (!streamReadMultipart(req, [&](const String &formData, const String &fn) {
          if (formData.equals("ssid")) {
            setSsid = true;
            return readString(&ssid);
          }
          if (formData.equals("pass")) {
            setPass = true;
            return readString(&pass);
          }
          if (formData.equals("hostname")) {
            setHostname = true;
            return readString(&hostname);
          }
          if (formData.equals("klipper")) {
            setKlipper = true;
            return readString(&klipper);
          }
          if (formData.equals("printProgressMethod")) {
            setPrintProgressMethod = true;
            return readString(&printProgressMethod);
          }
          if (formData.equals("skipStandbyAlternation")) {
            setSkipStandbyAlternation = true;
            return readString(&skipStandbyAlternation);
          }
          return (ReadCallback) nullptr;
        })) {
      return ESP_OK;
    }

    if (setSsid) {
      this->config->getNetworkConfig()->setSsid(ssid);
      LV_LOG_INFO("got ssid: %s", ssid.c_str());
    }

    if (setPass) {
      this->config->getNetworkConfig()->setPsk(pass);
      LV_LOG_INFO("got password");
    }

    if (setHostname) {
      this->config->getNetworkConfig()->setHostname(hostname);
      LV_LOG_INFO("got hostname: %s", hostname.c_str());
    }

    if (setKlipper) {
      this->config->getKlipperConfig()->setHost(klipper);
      LV_LOG_INFO("got KlipperIP: %s", klipper.c_str());
    }

    if (setPrintProgressMethod) {
      this->config->getKlipperConfig()->setPrintPercentageMethod(printProgressMethod);
      LV_LOG_INFO("got PrintProgressMethod: %s", printProgressMethod.c_str());
    }

    if (setSkipStandbyAlternation) {
      this->config->getKlipperConfig()->setSkipStandbyAlternate(skipStandbyAlternation);
      LV_LOG_INFO("got SkipStandbyAlternation: %s", skipStandbyAlternation.c_str());
    }

    this->config->save();
    delay(200);

    // Validate that SSID and password are set before connecting
    String currentSsid = this->config->getNetworkConfig()->getSsid();
    String currentPsk = this->config->getNetworkConfig()->getPsk();

    if (currentSsid.length() == 0 || currentPsk.length() == 0) {
      httpd_resp_set_type(req, "application/json");
      httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "{\"error\":\"WiFi SSID and password must be set\"}");
      LV_LOG_WARN("Cannot connect to WiFi: SSID or password is empty");
      return ESP_OK;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"result\": \"ok\"}");
    LV_LOG_INFO("WiFi Connect requested for  SSID: %s, HOSTNAME: %s",
                this->config->getNetworkConfig()->getSsid().c_str(),
                this->config->getNetworkConfig()->getHostname().c_str());
    wifimanager->connectToWiFi();

    return ESP_OK;
  }
};