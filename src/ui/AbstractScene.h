#pragma once
#include "../../lib/ui/JsonThemeConfig.h"
#include "log.h"
#include "network/KlipperApi.h"
#include "network/WifiManager.h"
#include "ui/Arc.h"
#include "ui/DisplayHAL.h"
#include "ui/ResourceImage.h"
#include "ui/SceneDeps.h"
#include "ui/SceneTimer.h"
#include "ui/TextLabel.h"
#include <vector>

class AbstractScene {
private:
  SceneDeps deps;
  Scene config;
  std::vector<Arc *> arcs;
  std::vector<ResourceImage *> images;
  std::vector<TextLabel *> labels;
  std::vector<SceneTimer *> timers;

public:
  AbstractScene(SceneDeps deps, Scene config) : deps(deps) {
    this->config = config;
    if (config.backgroundColor().has_value()) {
      deps.displayHAL->setBackgroundColor(config.backgroundColor().value());
    } else {
      deps.displayHAL->setBackgroundColor(deps.themeConfig->variables.backgroundColor());
    }

    std::for_each(config.content.begin(), config.content.end(), [&](const Content &item) {
      switch (item.type) {
      case Type::ARC:
        arcs.push_back(new Arc(deps.themeConfig));
        break;
      case Type::IMAGE: {
        images.push_back(new ResourceImage(item.file.value().c_str(), item.x.value_or(0), item.y.value_or(0),
                                           item.loop.value_or(true)));
      } break;
      case Type::TEXT: {
        Size size = item.size.value_or(Size::SMALL);
        fontSize fontSize = size == Size::SMALL ? fontSize::small : fontSize::large;
        labels.push_back(new TextLabel(deps.themeConfig, fontSize, item.x.value_or(0), item.y.value_or(0)));
      } break;
      case Type::TIMER:
        timers.push_back(new SceneTimer(item.duration.value_or(0)));
        break;
      case Type::UNKNOWN:
        LV_LOG_WARN("Unknown object tried to be inserted at scene %s", config.id.c_str());
        break;
      }
    });
  }

  ~AbstractScene() {
    std::for_each(arcs.begin(), arcs.end(), [&](Arc *arc) { delete arc; });
    std::for_each(images.begin(), images.end(), [&](ResourceImage *image) { delete image; });
    std::for_each(labels.begin(), labels.end(), [&](TextLabel *label) { delete label; });
    std::for_each(timers.begin(), timers.end(), [&](SceneTimer *timer) { delete timer; });
  }

  String nextScene() {
    for (Transition& transition : this->config.transitions) {
      // todo
      if (transition.condition == "false") {
        return transition.target.c_str();
      }
    }
    return "";
  }

  void tick() {
    for(Arc *arc: arcs) {
      // todo set value
      arc->setProgress(0);
      arc->tick(this->deps.displayHAL);
    }

    for(ResourceImage *image: images) {
      // todo set value
      image->tick(this->deps.displayHAL);
    }

    for(TextLabel* label: labels) {
      // todo set value
      String str = "abc";
      label->setText(str);
      label->tick(this->deps.displayHAL);
    }
  }
};
