#pragma once
#include "../../Version.h"
#include "AbstractPage.h"

class ApiStatusGet : public AbstractPage {
  Config *config;

public:
  explicit ApiStatusGet(Config *config, httpd_handle_t server) : AbstractPage(server, HTTP_GET, "/api/status") {
    this->config = config;
  }

  esp_err_t handler(httpd_req_t *req) override {
    httpd_resp_set_type(req, "application/json");

    JsonDocument doc;
    doc["hash"] = Version::getGitCommitSha1();
    doc["branch"] = Version::getGitBranch();
    doc["gitTimestamp"] = Version::getGitTimestamp();
    doc["buildTimestamp"] = Version::getBuildTimestamp();
#if KNOMIV1
    doc["device"] = "Knomi V1";
#elif KNOMIV2
    doc["device"] = "Knomi V2";
#else
#error "Unknown device"
#endif

    if (this->config->getNetworkConfig() != nullptr) {
      doc["ssid"] = this->config->getNetworkConfig()->getSsid();
      unsigned int pskLength = this->config->getNetworkConfig()->getPsk().length();
      String maskedPsk = String();
      for (unsigned int i = 0; i < pskLength; i++) {
        maskedPsk.concat("*");
      }
      doc["pass"] = maskedPsk;
      doc["hostname"] = this->config->getNetworkConfig()->getHostname();
    }

    if (this->config->getKlipperConfig() != nullptr) {
      doc["ip"] = this->config->getKlipperConfig()->getHost();
      doc["printProgressMethod"] = this->config->getKlipperConfig()->getPrintPercentageMethod();
      doc["skipStandbyAlternation"] = this->config->getKlipperConfig()->getSkipStandbyAlternate();
    }

    if (this->config->getUiConfig() != nullptr) {
      doc["accentColor"] = String("#") + String(this->config->getUiConfig()->getAccentColor(), HEX);
      doc["backgroundColor"] = String("#") + String(this->config->getUiConfig()->getBackgroundColor(), HEX);
    }

    doc["ota_partition"] = String(esp_ota_get_running_partition()->label);
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_DEFAULT);
    auto heap = doc["heap"].to<JsonObject>();
    heap["allocated_blocks"] = info.allocated_blocks;
    heap["free_blocks"] = info.free_blocks;
    heap["largest_free_block"] = info.largest_free_block;
    heap["minimum_free_bytes"] = info.minimum_free_bytes;
    heap["total_allocated_bytes"] = info.total_allocated_bytes;
    heap["total_blocks"] = info.total_blocks;
    heap["total_free_bytes"] = info.total_free_bytes;

    String output;
    serializeJsonPretty(doc, output);

    httpd_resp_send(req, output.c_str(), output.length());
    return ESP_OK;
  }
};