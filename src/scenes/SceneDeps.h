#pragma once

class KlipperStreaming;
class WifiManager;
class KnomiWebServer;
class UIConfig;
class DisplayHAL;
class UpdateProgress;
class KlipperConfig;

struct SceneDeps {
public:
  KlipperStreaming *klipperStreaming = nullptr;
  UpdateProgress *progress = nullptr;
  WifiManager *mgr = nullptr;
  UIConfig *styles = nullptr;
  KlipperConfig *klipperConfig = nullptr;
  DisplayHAL *displayHAL = nullptr;
  KnomiWebServer *webServer = nullptr;

  SceneDeps(KlipperStreaming *streaming, UpdateProgress *progress, WifiManager *mgr, KnomiWebServer *webServer,
            UIConfig *styles, DisplayHAL *displayHAL, KlipperConfig *klipperConfig) {
    this->klipperStreaming = streaming;
    this->progress = progress;
    this->mgr = mgr;
    this->styles = styles;
    this->displayHAL = displayHAL;
    this->webServer = webServer;
    this->klipperConfig = klipperConfig;
  }
};