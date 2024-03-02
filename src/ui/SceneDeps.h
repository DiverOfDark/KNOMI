#pragma once

class KlipperApi;
class WifiManager;
class KnomiWebServer;
class DisplayHAL;
class UpdateProgress;

struct SceneDeps {
public:
  KlipperApi *klipperApi = nullptr;
  UpdateProgress *progress = nullptr;
  WifiManager *mgr = nullptr;
  KnomiWebServer *webServer = nullptr;
  DisplayHAL *displayHAL = nullptr;
  ThemeConfig *themeConfig = nullptr;

  SceneDeps(KlipperApi *api, UpdateProgress *progress, WifiManager *mgr, KnomiWebServer *webServer,
            ThemeConfig *themeConfig, DisplayHAL *displayHAL) {
    this->klipperApi = api;
    this->progress = progress;
    this->mgr = mgr;
    this->webServer = webServer;
    this->themeConfig = themeConfig;
    this->displayHAL = displayHAL;
  }
};