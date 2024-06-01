#pragma once
#include "../config/Config.h"
#include "esp_websocket_client.h"
#include <WiFi.h>

class KlipperStreaming {
private:
  Config *config;
  String uri;
  esp_websocket_client_handle_t client = nullptr;
  bool isReady = false;
  bool isSubscribed = false;
  bool reset = false;

  static void websocket_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    auto *self = (KlipperStreaming *)arg;
    switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
      self->connectionEstablished();
      break;
    case WEBSOCKET_EVENT_DISCONNECTED:
      self->connectionLost();
      break;
    case WEBSOCKET_EVENT_DATA: {
      auto *websocketData = (esp_websocket_event_data_t *)event_data;
      self->receiveFrame(event_id, websocketData->data_len, (char *)websocketData->data_ptr, websocketData->payload_len,
                         websocketData->payload_offset);
    } break;
    case WEBSOCKET_EVENT_ERROR:
      LV_LOG_INFO("WebSocket Error");
      self->connectionLost();
      break;
    default:
      break;
    }
  }

  void connectionEstablished() {
    LV_LOG_INFO("WebSocket Connected");
    connected = true;
    isReady = false;
    isSubscribed = false;
  }

  void requestInfo() {
    String printerInfo = "{\n"
                         "    \"jsonrpc\": \"2.0\",\n"
                         "    \"method\": \"printer.info\",\n"
                         "    \"id\": 5153\n"
                         "}";
    esp_websocket_client_send_text(client, printerInfo.c_str(), printerInfo.length(), portMAX_DELAY);

    if (isReady && !isSubscribed) {
      isSubscribed = true;
      String subscribe = "{\n"
                         "    \"jsonrpc\": \"2.0\",\n"
                         "    \"method\": \"printer.objects.subscribe\",\n"
                         "    \"params\": {\n"
                         "        \"objects\": {\n"
                         "            \"gcode_move\": null,\n"
                         "            \"toolhead\": [\"position\", \"status\"],\n"
                         "            \"extruder\": null,\n"
                         "            \"heater_bed\": null,\n"
                         "            \"print_stats\":[\"state\",\"filament_used\"],"
                         "            \"virtual_sdcard\":[\"progress\"],"
                         "            \"idle_timeout\":[\"state\"],"
                         "            \"gcode_macro _KNOMI_STATUS\":null\n"
                         "        }\n"
                         "    },\n"
                         "    \"id\": 5434\n"
                         "}";

      esp_websocket_client_send_text(client, subscribe.c_str(), subscribe.length(), portMAX_DELAY);
    }
  }

  void connectionLost() {
    LV_LOG_INFO("WebSocket Disconnected");
    connected = false;
    isReady = false;
    isSubscribed = false;
    reset = true;
  }

  char *buffer = nullptr;

  void receiveFrame(int eventId, int len, char *data, int payloadLen, int payloadOffset) {
    LV_LOG_DEBUG("Received frame %i/%i (%i)", payloadOffset, payloadLen, len);
    if (payloadOffset == 0) {
      delete[] buffer;
      buffer = new char[payloadLen + 1];
      buffer[payloadLen] = 0;
    }
    memcpy(buffer + payloadOffset, data, len);
    if (payloadOffset + len == payloadLen) {
      frameComplete(buffer, payloadLen);
    }
  }

  void frameComplete(char *buffer, int size) {
    JsonDocument filter;
    filter["method"] = true;
    filter["id"] = true;

    JsonDocument doc;
    deserializeJson(doc, buffer, DeserializationOption::Filter(filter));

    String method = doc["method"].as<String>();
    int id = doc["id"].as<int>();

    if (method == "notify_proc_stat_update" || method == "notify_service_state_changed" ||
        method == "notify_filelist_changed" || method == "notify_gcode_response") {
      LV_LOG_DEBUG("Frame complete: %s", method.c_str());
      return;
    }

    if (method == "notify_klippy_disconected") {
      LV_LOG_INFO("Klipper disconnected");
      isReady = false;
      return;
    }

    if (method == "notify_klippy_ready") {
      LV_LOG_INFO("Klipper connected");
      isReady = true;
      return;
    }

    if (method == "notify_status_update") {
      deserializeJson(doc, buffer);
      const JsonObject &object = doc["params"][0].as<JsonObject>();
      parseResponseObjects(object);
      return;
    }

    if (id == 5434) {
      deserializeJson(doc, buffer);
      JsonObject obj = doc["result"]["status"];
      parseResponseObjects(obj);
      return;
    }

    if (id == 5153) {
      filter["result"]["state"] = true;
      deserializeJson(doc, buffer, DeserializationOption::Filter(filter));
      String state = doc["result"]["state"].as<String>();

      bool newIsReady = state == "ready";
      if (newIsReady != isReady) {
        isReady = newIsReady;
        LV_LOG_INFO("State: %s", state.c_str());
      }
      return;
    }

    LV_LOG_INFO("Unexpected frame: %.*s", size, buffer);
  }

  void parseResponseObjects(const JsonObject &object) {
    for (JsonPair iter : object) {
      String key = iter.key().c_str();
      JsonObject value = iter.value().as<JsonObject>();
      if (key == "extruder") {
        updateExtruder(value);
      } else if (key == "heater_bed") {
        updateHeaterBed(value);
      } else if (key == "gcode_move") {
        updatePosition(value);
      } else if (key == "toolhead") {
        updatePosition(value);
        if (value.containsKey("status")) {
          this->toolheadStatus = value["status"].as<String>();
          LV_LOG_INFO("Toolhead status: %s", toolheadStatus.c_str());
        }
      } else if (key == "print_stats") {
        if (value.containsKey("filament_used")) {
          this->filament_used = value["filament_used"].as<float>();
          LV_LOG_INFO("Filament used: %f", this->filament_used);
        }
        if (value.containsKey("state")) {
          this->printState = value["state"].as<String>();
          LV_LOG_INFO("State: %s", this->printState);
        }
      } else if (key == "virtual_sdcard") {
        if (value.containsKey("progress")) {
          this->progress = value["progress"].as<float>() * 100;
          LV_LOG_INFO("Progress: %f", this->progress);
        }
      } else if (key == "idle_timeout") {
        // Do nothing
      } else if (key == "gcode_macro _KNOMI_STATUS") {
        if (value.containsKey("homing")) {
          this->homing = value["homing"].as<bool>();
          LV_LOG_INFO("Homing: %i", this->homing);
        }
        if (value.containsKey("probing")) {
          this->probing = value["probing"].as<bool>();
          LV_LOG_INFO("Probing: %i", this->probing);
        }
        if (value.containsKey("qgling")) {
          this->qgling = value["qgling"].as<bool>();
          LV_LOG_INFO("QGLing: %i", this->qgling);
        }
        if (value.containsKey("heating_nozzle")) {
          this->heating_nozzle = value["heating_nozzle"].as<bool>();
          LV_LOG_INFO("Heating nozzle: %i", this->heating_nozzle);
        }
        if (value.containsKey("heating_bed")) {
          this->heating_bed = value["heating_bed"].as<bool>();
          LV_LOG_INFO("Heating bed: %i", this->heating_bed);
        }
      } else {
        String ser;
        serializeJson(value, ser);
        LV_LOG_INFO("Unexpected key: %s / %s", key.c_str(), ser.c_str());
      }
    }
  }

  void updatePosition(const JsonObject &value) {
    if (!value.containsKey("position")) {
      return;
    }

    const JsonArray &arr = value["position"];
    size_t cnt = arr.size();
    if (cnt > 0) {
      this->positionX = arr[0].as<double>();
      LV_LOG_INFO("Position X: %f", this->positionX);
    }
    if (cnt > 1) {
      this->positionY = arr[1].as<double>();
      LV_LOG_INFO("Position Y: %f", this->positionY);
    }
    if (cnt > 2) {
      this->positionZ = arr[2].as<double>();
      LV_LOG_INFO("Position Z: %f", this->positionZ);
    }
    if (cnt > 3) {
      this->positionE = arr[3].as<double>();
      LV_LOG_INFO("Position E: %f", this->positionE);
    }
  }

  void updateHeaterBed(JsonObject &object) {
    if (object.containsKey("temperature")) {
      float newTemperature = object["temperature"].as<float>();
      if (abs(this->bedTemperature - newTemperature) > 0.15) {
        this->bedTemperature = newTemperature;
        this->bedTemperatureString = formatTemperature(this->bedTemperature);
        LV_LOG_INFO("Heater bed temperature: %f", this->bedTemperature);
      }
    }
    if (object.containsKey("target")) {
      float newTemperature = object["target"].as<float>();
      if (abs(this->bedTarget - newTemperature) > 0.15) {
        this->bedTarget = newTemperature;
        this->bedTargetString = formatTemperature(this->bedTarget);
        LV_LOG_INFO("Bed target: %f", this->bedTarget);
      }
    }
  }

  void updateExtruder(JsonObject &object) {
    if (object.containsKey("temperature")) {
      float newTemperature = object["temperature"].as<float>();
      if (abs(this->extruderTemperature - newTemperature) > 0.15) {
        this->extruderTemperature = newTemperature;
        this->extruderTemperatureString = formatTemperature(this->extruderTemperature);
        LV_LOG_INFO("Extruder temperature: %f", this->extruderTemperature);
      }
    }
    if (object.containsKey("target")) {
      float newTemperature = object["target"].as<float>();
      if (abs(this->extruderTarget - newTemperature) > 0.15) {
        this->extruderTarget = newTemperature;
        this->extruderTargetString = formatTemperature(this->extruderTarget);
        LV_LOG_INFO("Extruder target: %f", this->extruderTarget);
      }
    }
  }

  static String formatTemperature(double value) {
    String formatted = String(value, 2);
    // TODO return degree (°) sign
    return formatted + " C";
  }

public:
  unsigned long lastRequest = 0;

  bool connected = false;

  float bedTemperature = 0;
  float bedTarget = 0;
  float extruderTemperature = 0;
  float extruderTarget = 0;

  String bedTemperatureString = "";
  String bedTargetString = "";
  String extruderTemperatureString = "";
  String extruderTargetString = "";

  float positionX = 0;
  float positionY = 0;
  float positionZ = 0;
  float positionE = 0;
  float filament_used = 0;
  float progress = 0;

  bool homing = false;
  bool probing = false;
  bool qgling = false;
  bool heating_nozzle = false;
  bool heating_bed = false;

  String toolheadStatus;
  String printState; // standby, printing, paused, error, complete

  bool isPrinting() const { return printState == "printing"; };

  bool isHeatingBed() const { return heating_bed || bedTemperature + 3 < bedTarget; };

  bool isHeatingExtruder() const { return heating_nozzle || extruderTemperature + 3 < extruderTarget; };

  explicit KlipperStreaming(Config *config) { this->config = config; }

  void start() {
    if (client != nullptr) {
      return;
    }
    LV_LOG_INFO("Starting websocket client");

    this->uri = "ws://" + config->getKlipperConfig()->getHost() + "/websocket";
    esp_websocket_client_config_t websocket_cfg = {};
    websocket_cfg.uri = uri.c_str();
    websocket_cfg.disable_auto_reconnect = false;

    client = esp_websocket_client_init(&websocket_cfg);
    LV_LOG_INFO("Client created %i", client == nullptr);
    esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)this);

    LV_LOG_INFO("Client start");
    esp_websocket_client_start(client);
  }

  void stop() {
    if (client != nullptr) {
      if (esp_websocket_client_is_connected(client)) {
        esp_websocket_client_close(client, 1000);
      }
      esp_websocket_client_stop(client);
      esp_websocket_client_destroy(client);
      client = nullptr;

      connected = false;
      isSubscribed = false;
    }
  }

  ~KlipperStreaming() { stop(); }

  void tick() {
    if (millis() - lastRequest <= 1000)
      return;

    lastRequest = millis();

    if (reset) {
      connected = false;
      isSubscribed = false;
      reset = false;
      stop();
      return;
    }

    if (WiFi.isConnected()) {
      start();
    } else {
      stop();
    }

    if (connected) {
      if (!esp_websocket_client_is_connected(client)) {
        connected = false;
        isSubscribed = false;
        reset = true;
        return;
      }
      requestInfo();
    }
  }
};