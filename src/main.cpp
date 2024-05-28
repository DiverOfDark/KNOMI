#include "button/Button.h"
#include "config/Config.h"
#include "hal/pinout.h"
#include "log.h"
#include "network/KlipperStreaming.h"
#include "network/KnomiWebServer.h"
#include "network/WifiManager.h"
#include "ui/DisplayHAL.h"
#include "ui/SceneManager.h"
#include "watchdog.h"
#include <Arduino.h>
#include <WiFi.h>

using namespace std;

Config *config = nullptr;
WifiManager *wifiManager = nullptr;
Button *btn = nullptr;
KnomiWebServer *webServer = nullptr;
KlipperStreaming *klipperStreaming = nullptr;
__attribute__((unused)) SceneManager *sceneManager = nullptr;
DisplayHAL *displayHAL = nullptr;
UpdateProgress *progress = nullptr;
Watchdog *watchDog = nullptr;

uint32_t netcheck_nexttime = 0;

SemaphoreHandle_t xMutex = xSemaphoreCreateMutex();

ulong lastLogTime;
void logToSerial(const char *logLevel, const char *file, int line, const char *func, const char *format, ...) {
  va_list args;
  va_start(args, format);
  static char msg[1024];
  ulong t = millis();
  vsnprintf(msg, sizeof(msg), format, args);
  va_end(args);

  static char buf[2048];
  snprintf(buf, sizeof(buf), "[%s] \t[%u] [%s] \t(%lu.%03lu, +%lu)\t %s: %s\t(in %s:%d)\n", logLevel,
           esp_get_free_heap_size(), pcTaskGetName(xTaskGetCurrentTaskHandle()), t / 1000, t % 1000, t - lastLogTime,
           func, msg, file, line);
  lastLogTime = t;
  xSemaphoreTake(xMutex, portMAX_DELAY);
  ets_printf(buf);
  xSemaphoreGive(xMutex);

//printf(buf);
  if (webServer != nullptr) {
    webServer->websocketLog(buf);
  }
}

__attribute__((unused)) void setup() {
  LV_LOG_INFO("Setup");
  Serial.begin(115200);
  LittleFS.begin();
  delay(200);
  LV_LOG_INFO("LittleFS started");
  config = new Config();
  LV_LOG_INFO("Config created");
  wifiManager = new WifiManager(config);
  LV_LOG_INFO("WifiManager created");
  btn = new Button(wifiManager, config);
  LV_LOG_INFO("Timer and button created");
  displayHAL = new DisplayHAL();
  LV_LOG_INFO("DisplayHAL created");
  klipperStreaming = new KlipperStreaming(config);
  LV_LOG_INFO("KlipperStreaming started");
  progress = new UpdateProgress();
  webServer = new KnomiWebServer(config, wifiManager, progress);
  LV_LOG_INFO("WebServer started");
  wifiManager->connectToWiFi();
  LV_LOG_INFO("Connected to wifi");
  sceneManager = new SceneManager(webServer, progress, klipperStreaming, wifiManager, config->getUiConfig(), displayHAL, btn);
  LV_LOG_INFO("SceneManager started");
  watchDog = new Watchdog(klipperStreaming);
  LV_LOG_INFO("Watchdog started");
}

__attribute__((unused)) void loop() {
  uint32_t nowtime = millis();

  klipperStreaming->tick();

  if (nowtime > netcheck_nexttime) {
    wifiManager->tick();
    webServer->tick();
    netcheck_nexttime = nowtime + 100UL;
  }
}
