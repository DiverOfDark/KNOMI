#pragma once
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

    int progress = (int)round(deps.klipperStreaming->progress);
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
};
