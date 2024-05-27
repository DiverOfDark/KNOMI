#pragma once
#include "AbstractScene.h"
#include "SwitchSceneRequest.h"
#include "VoronScene.h"

class StandbyScene : public AbstractScene {
private:
  ResourceImage *ri_standby;

public:
  explicit StandbyScene(SceneDeps deps) : AbstractScene(deps) { ri_standby = KnownResourceImages::get_Standby(); }

  ~StandbyScene() override { delete ri_standby; }

  SwitchSceneRequest *NextScene() override {
    if (deps.klipperStreaming->homing)
      return new SwitchSceneRequest(deps, SceneId::Homing);
    else if (deps.klipperStreaming->probing)
      return new SwitchSceneRequest(deps, SceneId::Leveling);
    else if (deps.klipperStreaming->qgling)
      return new SwitchSceneRequest(deps, SceneId::QGLeveling);
    else if (deps.klipperStreaming->isHeatingBed())
      return new SwitchSceneRequest(deps, SceneId::BedHeating);
    else if (deps.klipperStreaming->isHeatingExtruder())
      return new SwitchSceneRequest(deps, SceneId::ExtruderHeating);
    else if (deps.klipperStreaming->isPrinting())
      return new SwitchSceneRequest(deps, SceneId::BeforePrint);
    else if (ri_standby->isPlayedToEnd())
      return new SwitchSceneRequest(deps, SceneId::Voron);
    else
      return nullptr;
  }

  void Tick() override { ri_standby->tick(deps.displayHAL); }
};