#pragma once
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "log.h"
#include "network/WifiManager.h"
#include "scenes/AbstractScene.h"
#include "scenes/BootupLogo.h"
#include "scenes/SwitchSceneRequest.h"

class SceneManager {
private:
  AbstractScene *currentScene = nullptr;
  SceneId currentSceneId;
  SceneDeps deps;
  Button *button;

  static void refreshSceneCallback(void *arg) {
    esp_task_wdt_add(NULL);
    while (true) {
      esp_task_wdt_reset();
      ((SceneManager *)arg)->refreshScene();
      vTaskDelay(15);
    }
  }

public:
  explicit SceneManager(KnomiWebServer *webServer, UpdateProgress *progress, KlipperStreaming *klipperStreaming,
                        WifiManager *manager, UIConfig *config, DisplayHAL *displayHAL, Button *btn)
      : deps(klipperStreaming, progress, manager, webServer, config, displayHAL) {
    this->currentScene = new BootupLogoScene(deps);
    this->currentSceneId = SceneId::BootupLogo;
    this->button = btn;
    xTaskCreatePinnedToCore(
        refreshSceneCallback, "displayUpdate", 16000, /* Stack size in words */
        this,
        21,       // see https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/performance/speed.html
        NULL, 0); /* Core ID */
  }

  void refreshScene() {
    if (!deps.webServer->started) {
      currentScene->Tick();
      return;
    }

    SwitchSceneRequest *switchSceneRequest = nullptr;

    if (deps.progress->isInProgress && currentSceneId != SceneId::FirmwareUpdate) {
      LV_LOG_INFO(("Switching to firmware update scene from " + String(currentSceneId)).c_str());
      switchSceneRequest = new SwitchSceneRequest(deps, SceneId::FirmwareUpdate);
    } else {
      if (deps.mgr->isInConfigMode() && currentSceneId != SceneId::APConfig && currentSceneId != SceneId::BootupLogo) {
        switchSceneRequest = new SwitchSceneRequest(deps, SceneId::APConfig);
        LV_LOG_INFO(("Switching to APConfig scene from " + String(currentSceneId)).c_str());
      } else {
        if (WiFi.isConnected() && !button->isPressed() && !deps.klipperStreaming->connected &&
            currentSceneId != SceneId::NoKlipper && currentSceneId != SceneId::BootupLogo &&
            currentSceneId != SceneId::APConfig) {
                    switchSceneRequest = new SwitchSceneRequest(deps, SceneId::NoKlipper);
                    LV_LOG_INFO(("Switching to NoKlipper scene from " + String(currentSceneId)).c_str());
                  }
      }
    }

    if (switchSceneRequest == nullptr) {
      switchSceneRequest = currentScene->NextScene();
      if (switchSceneRequest != nullptr) {
        LV_LOG_INFO(("Going to switch from " + String(currentSceneId) + " to " + String(switchSceneRequest->id)).c_str());
      }
    }

    currentScene->Tick();

    if (switchSceneRequest != nullptr) {
      LV_LOG_INFO(("Deleting current scene " + String(currentSceneId)).c_str());
      delete currentScene;
      currentScene = nullptr;
      LV_LOG_INFO((String("Switching scene to ") + String(switchSceneRequest->id)).c_str());
      currentScene = switchSceneRequest->Provide();
      currentSceneId = switchSceneRequest->id;
      delete switchSceneRequest;
    }
  }
};
