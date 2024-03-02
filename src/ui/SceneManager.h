#pragma once
#include "AbstractScene.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "log.h"
#include "network/WifiManager.h"
#include "optional"
#include "soc/rtc_wdt.h"

class SceneManager {
private:
  AbstractScene *currentScene = nullptr;
  String currentSceneId;
  std::optional<String> nextSceneId;
  SceneDeps deps;
  ThemeConfig *themeConfig;
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
  explicit SceneManager(KnomiWebServer *webServer, ThemeConfig *themeConfig, UpdateProgress *progress,
                        KlipperApi *klipperApi, WifiManager *manager, UIConfig *config, DisplayHAL *displayHAL,
                        Button *btn)
      : deps(klipperApi, progress, manager, webServer, config, displayHAL) {
    this->themeConfig = themeConfig;
    this->nextSceneId = themeConfig->startingScene.c_str();
    this->button = btn;

    xTaskCreatePinnedToCore(
        refreshSceneCallback, "displayUpdate", 16000, /* Stack size in words */
        this,
        21,       // see https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/performance/speed.html
        NULL, 0); /* Core ID */
  }

  String getCurrentSceneId() { return currentSceneId; }

  void switchSceneIfRequired() {
    if (nextSceneId.has_value()) {
      LV_LOG_INFO("Deleting current scene");
      String &string = nextSceneId.value();

      delete currentScene;
      currentScene = nullptr;

      LV_LOG_INFO(("Switching scene to " + nextSceneId.value()).c_str());

      const std::vector<Scene>::iterator &iterator =
          std::find_if(themeConfig->scenes.begin(), themeConfig->scenes.end(),
                       [&](const Scene &item) { return item.id == nextSceneId.value().c_str(); });

      Scene &scene = iterator.operator*();
      if (iterator != themeConfig->scenes.end()) {
        currentScene = new AbstractScene(deps, scene);
        nextSceneId->clear();
      } else {
        LV_LOG_INFO("Failed to find Scene %s", nextSceneId.value().c_str());
        nextSceneId = themeConfig->startingScene.c_str();
      }

      nextSceneId->clear();
    }
  }

  void refreshScene() {
    if (deps.progress->isInProgress && this->getCurrentSceneId() != themeConfig->firmwareUpdateScene.c_str()) {
      this->nextSceneId = themeConfig->firmwareUpdateScene.c_str();
    } else if (deps.mgr->isInConfigMode() && this->getCurrentSceneId() != themeConfig->accessPointConfig.c_str() &&
               this->getCurrentSceneId() != themeConfig->startingScene.c_str()) {
      nextSceneId = themeConfig->accessPointConfig.c_str();
    } else if (WiFi.isConnected() && !button->isPressed()) {
      if (deps.klipperApi->isKlipperNotAvailable() &&
          this->getCurrentSceneId() != themeConfig->noKlipperScene.c_str()) {
        this->nextSceneId = themeConfig->noKlipperScene.c_str();
      }
    }

    if (currentScene != nullptr && !nextSceneId.has_value()) {
      auto next = currentScene->nextScene();
      if (next != nullptr) {
        nextSceneId = next;
      }
    }

    if (currentScene != nullptr) {
      currentScene->tick();
      switchSceneIfRequired();
    }
  }
};
