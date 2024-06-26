#pragma once
#include "AbstractScene.h"

class Printing100PercentScene : public AbstractScene {
private:
  ResourceImage *ri_ok;

public:
  explicit Printing100PercentScene(SceneDeps deps) : AbstractScene(deps) {
    ri_ok = KnownResourceImages::get_Print_ok();
  }
  ~Printing100PercentScene() override { delete ri_ok; }

  SwitchSceneRequest *NextScene() override {
    if (ri_ok->isPlayedToEnd())
      return new SwitchSceneRequest(deps, SceneId::AfterPrint);
    return nullptr;
  }

  void Tick() override { ri_ok->tick(deps.displayHAL); }
};