#pragma once
#include "../config/Config.h"
#include "KlipperStreaming.h"
#include "log.h"
#include <ArduinoJson.h>
#include <queue>

class KlipperApi {
private:
  Config *config;
  KlipperStreaming *klipper;

public:
  KlipperApi(KlipperStreaming* klipperStreaming, Config *config) {
    this->config = config;
    this->klipper = klipperStreaming;
  }

  String& getExtruderActualTemp() { return klipper->extruderTemperatureString; }
  String& getExtruderTargetTemp() { return klipper->extruderTargetString; }
  String& getBedActualTemp() { return klipper->bedTemperatureString; }
  String& getBedTargetTemp() { return klipper->bedTargetString; }
  int getProgressData() const { return (int) round(klipper->progress); }

  bool isHoming() const { return klipper->homing; }
  bool isLeveling() const { return klipper->probing; }
  bool isQGLeveling() const { return klipper->qgling; }
  bool isPrinting() const { return klipper->isPrinting(); }
  bool isHeatingBed() const { return klipper->heating_bed || klipper->bedTemperature + 3 < klipper->bedTarget; }
  bool isHeatingNozzle() const { return klipper->heating_nozzle || klipper->extruderTemperature + 3 < klipper->extruderTarget; }

  ulong getLastSuccessfullCall() const { return klipper->lastRequest; }

  bool isKlipperNotAvailable() const { return !klipper->connected; }
};