#pragma once
#include "../log.h"
#include "AbstractScene.h"

class Printing1PercentScene : public AbstractScene {
public:
  TextLabel *text;
  Arc *arc;
  SceneTimer *timer = nullptr;

  explicit Printing1PercentScene(SceneDeps deps) : AbstractScene(deps) {
    text = new TextLabel(deps.styles, fontSize::large, 0, 0);
    arc = new Arc(deps.styles);
  }
  ~Printing1PercentScene() override {
    delete text;
    delete arc;
    delete timer;
  }

  SwitchSceneRequest *NextScene() override {
    if (!deps.klipperStreaming->isPrinting()) {
      return new SwitchSceneRequest(deps, SceneId::Standby);
    };

    int progress = this->getPrintProgress();
    if (progress == 100) {
      if (timer == nullptr) {
        timer = new SceneTimer(7000);
      } else if (timer->isCompleted()) {
        return new SwitchSceneRequest(deps, SceneId::Printing100Percent);
      }
    }
    String result = String(progress) + "%";
    text->setText(result);
    arc->setProgress(progress);

    return nullptr;
  }

  void Tick() override {
    arc->tick(deps.displayHAL);
    text->tick(deps.displayHAL);
  }

  int cachedProgress = 0;
  int ticksUntilRefresh = 0;

  int getPrintProgress() {
    // avoid pointlessly recalculating progress, printers arent that fast...
    if (ticksUntilRefresh > 0) {
      ticksUntilRefresh--;
      return cachedProgress;
    }

    String method = deps.klipperConfig->getPrintPercentageMethod();
    float progress = 0;

    if (method == "file-relative") {
      int gcodeStartByte = deps.klipperStreaming->file_gcode_start_byte;
      int gcodeEndByte = deps.klipperStreaming->file_gcode_end_byte;
      int filePosition = deps.klipperStreaming->virtual_sdcard_file_position;

      if (gcodeStartByte != 0 && gcodeEndByte != 0 && filePosition != 0) {
        float currentPos = filePosition - gcodeStartByte;
        float maxPos = gcodeEndByte - gcodeStartByte;

        if (currentPos > 0 && maxPos > 0) {
          progress = (currentPos / maxPos) * 100;
        }
      }

      // fallback
      progress = deps.klipperStreaming->virtual_sdcard_progress * 100;
    } else if (method == "file-absolute") {
      progress = deps.klipperStreaming->virtual_sdcard_progress * 100;
    } else if (method == "slicer") {
      // relies on M73 progress updates - if not present klippy returns virtual_sdcard progress
      progress = deps.klipperStreaming->display_status_progress;
    } else if (method = "filament") {
      progress = (deps.klipperStreaming->filament_used / deps.klipperStreaming->file_filament_total) * 100;
    }

    LV_LOG_INFO("Print Progress: %f method: %s", progress, method.c_str());
    cachedProgress = round(progress);
    ticksUntilRefresh = 200;
    return cachedProgress;
  }
};
