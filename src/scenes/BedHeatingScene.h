#pragma once
#include "AbstractScene.h"

class BedHeatingScene : public AbstractScene {
private:
  ResourceImage *ri_bed;
  TextLabel *actualTemp;
  TextLabel *targetTemp;

public:
  explicit BedHeatingScene(SceneDeps deps) : AbstractScene(deps) {
    ri_bed = KnownResourceImages::get_bed_temp();
    actualTemp = new TextLabel(deps.styles, fontSize::small, 0, 75);
    targetTemp = new TextLabel(deps.styles, fontSize::small, 0, -75);
  }

  ~BedHeatingScene() override {
    delete ri_bed;
    delete actualTemp;
    delete targetTemp;
  }

  SwitchSceneRequest *NextScene() override {
    if (!deps.klipperStreaming->isHeatingBed()) {
      return new SwitchSceneRequest(deps, SceneId::Standby);
    }

    return nullptr;
  }

  void Tick() override {
    actualTemp->setText(deps.klipperStreaming->bedTemperatureString);
    targetTemp->setText(deps.klipperStreaming->bedTargetString);

    ri_bed->tick(deps.displayHAL);
    actualTemp->tick(deps.displayHAL);
    targetTemp->tick(deps.displayHAL);
  }
};