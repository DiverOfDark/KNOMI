#pragma once

class SceneTimer {
private:
  int endAt;

public:
  explicit SceneTimer(int duration) {
    this->endAt = millis() + duration;
    LV_LOG_INFO(("Created timer for " + String(endAt)).c_str());
  }

  bool isCompleted() { return millis() >= endAt; }
};