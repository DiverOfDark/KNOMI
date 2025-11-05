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

    if (millis() - lastSubscribeUpdate > 30000 && isSubscribed) {
      isSubscribed = false;
      LV_LOG_WARN("Haven't received updates from subscription in a while. Going to resubscribe.");
    }

    if (isReady && !isSubscribed) {
      lastSubscribeUpdate = millis();
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
                         "            \"print_stats\":[\"state\",\"filament_used\", \"filename\"],"
                         "            \"virtual_sdcard\":[\"progress\", \"file_position\", \"is_active\"],"
                         "            \"display_status\":[\"progress\"],"
                         "            \"idle_timeout\":[\"state\"],"
                         "            \"gcode_macro _KNOMI_STATUS\":null\n"
                         "        }\n"
                         "    },\n"
                         "    \"id\": 5434\n"
                         "}";

      esp_websocket_client_send_text(client, subscribe.c_str(), subscribe.length(), portMAX_DELAY);
    }

    if (this->print_status_file_changed) {
      this->print_status_file_changed = false;
      this->requestFileMetadata();
    }
  }

  void requestFileMetadata() {
    LV_LOG_INFO("Requesting print metadata");

    String fileMetadataRequest = "{\n"
                                 "    \"id\": 66829,\n"
                                 "    \"method\": \"server.files.metadata\",\n"
                                 "    \"jsonrpc\": \"2.0\",\n"
                                 "    \"params\": {\n"
                                 "        \"filename\": \"" +
                                 this->print_stats_filename +
                                 "\"\n"
                                 "    }\n"
                                 "}";

    if (this->print_stats_filename != "" && isReady && isSubscribed) {
      esp_websocket_client_send_text(client, fileMetadataRequest.c_str(), fileMetadataRequest.length(), portMAX_DELAY);
    }
  }

  void connectionLost() {
    LV_LOG_INFO("WebSocket Disconnected");
    connected = false;
    isReady = false;
    isSubscribed = false;
    reset = true;
    this->print_stats_filename = "";
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
      lastSubscribeUpdate = millis();
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

    if (id == 66829) {
      deserializeJson(doc, buffer);

      if (doc["result"].is<JsonObject>()) {
        const JsonObject &result = doc["result"].as<JsonObject>();
        if (result["gcode_start_byte"].is<int>()) {
          this->file_gcode_start_byte = result["gcode_start_byte"].as<int>();
          LV_LOG_INFO("Gcode start byte: %i", this->file_gcode_start_byte);
        }
        if (result["gcode_end_byte"].is<int>()) {
          this->file_gcode_end_byte = result["gcode_end_byte"].as<int>();
          LV_LOG_INFO("Gcode end byte: %i", this->file_gcode_end_byte);
        }
        if (result["filament_total"].is<float>()) {
          this->file_filament_total = result["filament_total"].as<float>();
          LV_LOG_INFO("Filament total: %f", this->file_filament_total);
        }
      }
      return;
    }
    // LV_LOG_INFO("Unexpected frame: %.*s", size, buffer);
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
        if (value["status"].is<String>()) {
          this->toolheadStatus = value["status"].as<String>();
          LV_LOG_INFO("Toolhead status: %s", toolheadStatus.c_str());
        }
      } else if (key == "print_stats") {
        if (value["filament_used"].is<float>()) {
          this->filament_used = value["filament_used"].as<float>();
          LV_LOG_INFO("Filament used: %f", this->filament_used);
        }
        if (value["state"].is<String>()) {
          this->printState = value["state"].as<String>();
          LV_LOG_INFO("State: %s", this->printState.c_str());
        }
        if (value["filename"].is<String>()) {
          String newFilename = value["filename"].as<String>();

          if (newFilename != this->print_stats_filename) {
            this->print_stats_filename = newFilename;

            // Prevent stale data causing us to report the print as complete
            this->file_filament_total = 0;
            this->file_gcode_end_byte = 0;
            this->file_gcode_start_byte = 0;

            this->print_status_file_changed = true;

            LV_LOG_INFO("File changed, new filename: %s will trigger metadata fetch on next tick",
                        this->print_stats_filename.c_str());
          }
        }
      } else if (key == "virtual_sdcard") {
        if (value["progress"].is<float>()) {
          this->virtual_sdcard_progress = value["progress"].as<float>();
          LV_LOG_INFO("Virtual SDCard Progress: %f", this->virtual_sdcard_progress);
        }
        if (value["file_position"].is<int>()) {
          this->virtual_sdcard_file_position = value["file_position"].as<int>();
          LV_LOG_INFO("Virtual SDCard File Position: %f", this->virtual_sdcard_file_position);
        }
        if (value["is_active"].is<bool>()) {
          this->virtual_sdcard_is_active = value["is_active"].as<bool>();
          LV_LOG_INFO("Virtual SDCard Active: %i", this->virtual_sdcard_is_active);
        }
      } else if (key == "display_status") {
        if (value["progress"].is<float>()) {
          this->display_status_progress = value["progress"].as<float>() * 100;
          LV_LOG_INFO("Display Status Progress: %f", this->display_status_progress);
        }
      } else if (key == "idle_timeout") {
        // Do nothing
      } else if (key == "gcode_macro _KNOMI_STATUS") {
        if (value["homing"].is<bool>()) {
          this->homing = value["homing"].as<bool>();
          LV_LOG_INFO("Homing: %i", this->homing);
        }
        if (value["probing"].is<bool>()) {
          this->probing = value["probing"].as<bool>();
          LV_LOG_INFO("Probing: %i", this->probing);
        }
        if (value["qgling"].is<bool>()) {
          this->qgling = value["qgling"].as<bool>();
          LV_LOG_INFO("QGLing: %i", this->qgling);
        }
        if (value["heating_nozzle"].is<bool>()) {
          this->heating_nozzle = value["heating_nozzle"].as<bool>();
          LV_LOG_INFO("Heating nozzle: %i", this->heating_nozzle);
        }
        if (value["heating_bed"].is<bool>()) {
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
    if (!value["position"].is<JsonArray>()) {
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
    if (object["temperature"].is<float>()) {
      float newTemperature = object["temperature"].as<float>();
      if (abs(this->bedTemperature - newTemperature) > 0.15) {
        this->bedTemperature = newTemperature;
        this->bedTemperatureString = formatTemperature(this->bedTemperature);
        LV_LOG_INFO("Heater bed temperature: %f", this->bedTemperature);
      }
    }
    if (object["target"].is<float>()) {
      float newTemperature = object["target"].as<float>();
      if (abs(this->bedTarget - newTemperature) > 0.15) {
        this->bedTarget = newTemperature;
        this->bedTargetString = formatTemperature(this->bedTarget);
        LV_LOG_INFO("Bed target: %f", this->bedTarget);
      }
    }
  }

  void updateExtruder(JsonObject &object) {
    if (object["temperature"].is<float>()) {
      float newTemperature = object["temperature"].as<float>();
      if (abs(this->extruderTemperature - newTemperature) > 0.15) {
        this->extruderTemperature = newTemperature;
        this->extruderTemperatureString = formatTemperature(this->extruderTemperature);
        LV_LOG_INFO("Extruder temperature: %f", this->extruderTemperature);
      }
    }
    if (object["target"].is<float>()) {
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
  unsigned long lastSubscribeUpdate = 0;

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
  float virtual_sdcard_progress = 0;
  int virtual_sdcard_file_position = 0;
  bool virtual_sdcard_is_active = false;
  float display_status_progress = 0;
  String print_stats_filename = "";
  bool print_status_file_changed = false;
  int file_gcode_start_byte = 0;
  int file_gcode_end_byte = 0;
  float file_filament_total = 0;

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
