#pragma once
#include "ArduinoJson.h"
#include "StreamString.h"
#include "WString.h"
#include "map"
#include "string"
#include "ui/JsonThemeConfig.h"

// conflicts with ESPAsyncWebServer
namespace esp {
#include "esp_http_client.h"
#include "esp_tls.h"
} // namespace esp

using namespace esp;

class KlipperApiRequestBase {
private:
  String response = "";
  bool inProgress = false;
  ulong nextCall = 0;
  ulong lastCall = millis();

protected:
  int failCount = 0;
  int intervalCall = 2000; // ms

  virtual const char *getUrl() = 0;
  virtual void processJson(JsonDocument &doc) = 0;

public:
  int getFailCount() const { return failCount; }

  ulong getLastSuccessfullCall() const { return lastCall; }

  KlipperApiRequestBase() {}

  void Execute(String &klipper_ip) {
    if (inProgress) {
      return;
    }
    ulong now = millis();
    if (nextCall > now) {
      return;
    }
    nextCall = now + intervalCall;

    inProgress = true;
    response = "";

    const char *path = getUrl();
    LV_LOG_HTTP("Http request to %s %s", klipper_ip.c_str(), path);

    esp_http_client_config_t config = {.host = klipper_ip.c_str(),
                                       .path = path,
                                       .disable_auto_redirect = true,
                                       .event_handler = _http_event_handler,
                                       .user_data = this};
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
      LV_LOG_HTTP("HTTP GET Status = %d, content_length = %i", esp_http_client_get_status_code(client),
                  esp_http_client_get_content_length(client));
    } else {
      LV_LOG_INFO("HTTP GET request failed: %s", esp_err_to_name(err));
    }
    inProgress = false;
    esp_http_client_cleanup(client);
  }

  static esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
    auto kar = (KlipperApiRequestBase *)evt->user_data;
    return kar->httpEventHandler(evt);
  }

  esp_err_t httpEventHandler(esp_http_client_event_t *evt) {
    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
      LV_LOG_HTTP("HTTP_EVENT_ERROR");
      break;
    case HTTP_EVENT_ON_CONNECTED:
      // Incoming order: 1
      LV_LOG_HTTP("HTTP_EVENT_ON_CONNECTED");
      break;
    case HTTP_EVENT_HEADER_SENT:
      // Incoming order: 2
      LV_LOG_HTTP("HTTP_EVENT_HEADER_SENT");
      break;
    case HTTP_EVENT_ON_HEADER:
      // Incoming order: 3. Good place to save headers.
      LV_LOG_HTTP("HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
      break;
    case HTTP_EVENT_ON_DATA:
      // Incoming order: 4. Aggregate data
      LV_LOG_HTTP("HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
      char *buffer;
      buffer = new char[evt->data_len + 1];
      memset(buffer, 0, evt->data_len + 1);
      memcpy(buffer, evt->data, evt->data_len);
      response += String(buffer, evt->data_len);
      delete[] buffer;
      break;
    case HTTP_EVENT_ON_FINISH:
      // Incoming order: 5. Request fully buffered locally. process
      LV_LOG_HTTP("HTTP_EVENT_ON_FINISH");
      if (esp_http_client_get_status_code(evt->client) == 200) {
        JsonDocument doc;
        deserializeJson(doc, response.c_str());
        failCount = 0;
#if DEBUG
        unsigned int bufLen = response.length() * 2;
        auto buf = new char[bufLen];
        serializeJson(doc, buf, bufLen);
        LV_LOG_INFO("Parsed response:");
        LV_LOG_INFO(buf);
        delete[] buf;
#endif
        processJson(doc);
      } else {
        LV_LOG_INFO("Error on HTTP asyncHttpRequest");
        failCount++;
      }

      response = "";
      lastCall = millis();
      break;
    case HTTP_EVENT_DISCONNECTED: {
      LV_LOG_HTTP("HTTP_EVENT_DISCONNECTED");
      int mbedtls_err = 0;
      esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
      if (err != 0) {
        LV_LOG_INFO("Last esp error code: 0x%x", err);
        LV_LOG_INFO("Last mbedtls failure: 0x%x", mbedtls_err);
      }
    } break;
    default:
      LV_LOG_INFO("Unhandled eventId %i", evt->event_id);
      break;
    }
    yield();
    return ESP_OK;
  }
};

class KlipperApiRequest : public KlipperApiRequestBase {
private:
  String id;
  String url;
  std::map<String, String> variables;
  std::vector<Variable> variablesConfig;

public:
  KlipperApiRequest(Request &requestConfig) {
    this->id = requestConfig.id.c_str();
    this->url = requestConfig.url.c_str();
    this->variablesConfig = requestConfig.variables;
  }

  const char *getUrl() override { return this->url.c_str(); }

  void processJson(ArduinoJson::JsonDocument &doc) override {
    for (auto &var : variablesConfig) {
      JsonVariant jsonObj = doc;
      std::string s = var.path;
      std::string delimiter = ".";

      size_t pos = 0;
      std::string token;

      while ((pos = s.find(delimiter)) != std::string::npos) {
        token = s.substr(0, pos);

        if (!doc.containsKey(token.c_str())) {
          LV_LOG_WARN("Couldn't find json path %s at %s", s.c_str(), token.c_str());
          break;
        }

        jsonObj = jsonObj[token.c_str()];
        s.erase(0, pos + delimiter.length());
      }

      String value = jsonObj.as<String>();

      if (var.type == "double") {
        value = String(jsonObj.as<double>());
      }
      if (var.type == "boolean") {
        value = String(jsonObj.as<bool>());
      }
      if (var.type == "percentage") {
        value = String(jsonObj.as<double>() * 100);
      } else {
        LV_LOG_WARN("Unknown json type %s at %s", var.type.c_str(), var.name.c_str());
      }

      if (var.stringFormat.has_value()) {
        char format[256];
        snprintf(format, 256, var.stringFormat.value().c_str(), value.c_str());
        value = format;
      }

      variables[String(var.name.c_str())] = value.c_str();
    }
  }
};