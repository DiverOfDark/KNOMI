#pragma once

class KlipperStreaming;
class WifiManager;
class KnomiWebServer;
class UIConfig;
class DisplayHAL;
class UpdateProgress;

struct SceneDeps {
public:
  KlipperStreaming* klipperStreaming = nullptr;
  UpdateProgress *progress = nullptr;
  WifiManager *mgr = nullptr;
  UIConfig *styles = nullptr;
  DisplayHAL *displayHAL = nullptr;

  SceneDeps(KlipperStreaming *streaming, UpdateProgress *progress, WifiManager *mgr, KnomiWebServer *webServer, UIConfig *styles,
            DisplayHAL *displayHAL) {
    this->klipperStreaming = streaming;
    this->progress = progress;
    this->mgr = mgr;
    this->styles = styles;
    this->displayHAL = displayHAL;
  }
};