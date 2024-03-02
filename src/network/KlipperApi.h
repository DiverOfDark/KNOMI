#pragma once
#include "../config/Config.h"
#include "KlipperApiRequest.h"
#include "log.h"
#include "ui/JsonThemeConfig.h"
#include <ArduinoJson.h>
#include <queue>
#include <vector>

class KlipperApi {
private:
  Config *config;
  ThemeConfig *themeConfig;
  std::vector<KlipperApiRequest> requests;

public:
  KlipperApi(ThemeConfig *themeConfig, Config *config) {
    this->themeConfig = themeConfig;
    this->config = config;

    std::for_each(themeConfig->requests.begin(), themeConfig->requests.end(),
                  [&](const Request &item) { requests.push_back(KlipperApiRequest(item.id)); });
  }

  ulong getLastSuccessfullCall() const {
    ulong lastCall = 0;
    for (auto &req : requests) {
      ulong last = req.getLastSuccessfullCall();
      if (last > lastCall) {
        lastCall = last;
      }
    }
    return lastCall;
  }

  bool isKlipperNotAvailable() {
    int failCount = 0;
    for (auto &req : requests) {
      failCount += req.getFailCount();
    }
    return failCount > 3;
  }

  void refreshData() {
    if (config->isInitialised()) {
      String klipper_ip = config->getKlipperConfig()->getHost();
      klipper_ip.toLowerCase();
      for (auto &req : requests) {
        req.Execute(klipper_ip);
      }
    }
  }
};