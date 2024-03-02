#pragma once
#include "log.h"
#include "network/KlipperApi.h"
#include "network/WifiManager.h"
#include "ui/Arc.h"
#include "ui/DisplayHAL.h"
#include "ui/JsonThemeConfig.h"
#include "ui/SceneDeps.h"
#include "ui/SceneTimer.h"
#include "ui/TextLabel.h"

class AbstractScene {
private:
  SceneDeps deps;

public:
  AbstractScene(SceneDeps deps, Scene config) : deps(deps) {
    deps.displayHAL->setBackgroundColor(deps.styles->getBackgroundColor());
  }

  String nextScene() { return ""; }

  void tick() {}
};
