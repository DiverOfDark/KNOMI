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

uint32_t lastLogTime = 0;
void logToSerial(const char *logLevel, const char *file, int line, const char *func, const char *format, ...) {
  // Take the mutex for the entire formatting + output to avoid interleaved logs from multiple tasks
  xSemaphoreTake(xMutex, portMAX_DELAY);

  va_list args;
  va_start(args, format);
  char msg[768];
  int tlen = vsnprintf(msg, sizeof(msg), format, args);
  va_end(args);
  if (tlen < 0) {
    // formatting error; ensure msg is terminated
    msg[0] = '\0';
  } else if (tlen >= static_cast<int>(sizeof(msg))) {
    // truncated; indicate
    const char *ellipsis = "...";
    size_t el = strlen(ellipsis);
    memcpy(&msg[sizeof(msg) - el - 1], ellipsis, el);
    msg[sizeof(msg) - 1] = '\0';
  }

  uint32_t t = millis();
  char buf[1024];
  snprintf(buf, sizeof(buf), "[%s] \t[%u] [%s] \t(%lu.%03lu, +%lu)\t %s: %s\t(in %s:%d)\n", logLevel,
           esp_get_free_heap_size(), pcTaskGetName(xTaskGetCurrentTaskHandle()), static_cast<unsigned long>(t / 1000),
           static_cast<unsigned long>(t % 1000), static_cast<unsigned long>(t - lastLogTime), func, msg, file, line);
  lastLogTime = t;

  ets_printf("%s", buf);
  // release the mutex before forwarding to websocket to avoid reentrancy/deadlocks
  xSemaphoreGive(xMutex);
  // forward to websocket if available (must not log to avoid recursion)
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
  sceneManager = new SceneManager(webServer, progress, klipperStreaming, wifiManager, config->getUiConfig(),
                                  config->getKlipperConfig(), displayHAL, btn);
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
