#pragma once

#include "../generated/themeConfig.h"
#include "ArduinoJson.h"
#include "optional"
#include "string"
#include "vector"
#include <optional>

struct Variable {
  void apply(JsonObject obj) {
    name = obj["name"].as<const char *>();
    path = obj["path"].as<const char *>();
    stringFormat = obj["stringFormat"].as<const char *>();
    type = obj["type"].as<const char *>();
  }

  std::string name;
  std::string path;
  std::optional<std::string> stringFormat;
  std::string type;
};

struct Request {
  void apply(JsonObject obj) {
    id = obj["id"].as<const char *>();
    url = obj["url"].as<const char *>();
    JsonArray variables = obj["variables"].as<JsonArray>();
    for (int i = 0; i < variables.size(); i++) {
      auto v = Variable();
      v.apply(variables[i].as<JsonObject>());
      this->variables.push_back(v);
    }
  }
  std::string id;
  std::string url;
  std::vector<Variable> variables;
};

enum class Size : int { LARGE, SMALL };

enum class Type : int { ARC, IMAGE, TEXT, TIMER, UNKNOWN };

struct Content {
  Size toSize(const char *size) {
    if (strcmp(size, "large") == 0) {
      return Size::LARGE;
    } else {
      return Size::SMALL;
    }
  }

  Type toType(const char *type) {
    if (strcmp(type, "arc") == 0) {
      return Type::ARC;
    } else if (strcmp(type, "image") == 0) {
      return Type::IMAGE;
    } else if (strcmp(type, "text") == 0) {
      return Type::TEXT;
    } else if (strcmp(type, "timer") == 0) {
      return Type::TIMER;
    } else
      return Type::UNKNOWN;
  }

  void apply(JsonObject obj) {
    size = toSize(obj["size"].as<const char *>());
    type = toType(obj["type"].as<const char *>());
    value = obj["value"].as<const char *>();
    x = obj["x"].as<double>();
    y = obj["y"].as<double>();
    file = obj["file"].as<const char *>();
    loop = obj["loop"].as<bool>();
    duration = obj["duration"].as<double>();
  }

  std::optional<Size> size;
  Type type;
  std::optional<std::string> value;
  std::optional<double> x;
  std::optional<double> y;
  std::optional<std::string> file;
  std::optional<bool> loop;
  std::optional<double> duration;
};

struct Transition {
  void apply(JsonObject obj) {
    condition = obj["condition"].as<const char *>();
    target = obj["target"].as<const char *>();
  }

  std::string condition;
  std::string target;
};

struct Scene {
  void apply(JsonObject obj) {
    id = obj["id"].as<const char *>();
    background = obj["background"].as<const char *>();
    JsonArray content = obj["content"].as<JsonArray>();
    for (int i = 0; i < content.size(); i++) {
      auto c = Content();
      c.apply(content[i].as<JsonObject>());
      this->content.push_back(c);
    }
    JsonArray transitions = obj["transitions"].as<JsonArray>();
    for (int i = 0; i < transitions.size(); i++) {
      auto t = Transition();
      t.apply(transitions[i].as<JsonObject>());
      this->transitions.push_back(t);
    }
  }
  std::optional<std::string> background;
  std::vector<Content> content;
  std::string id;
  std::vector<Transition> transitions;
};

struct Variables {
  void apply(JsonObject obj) {
    background = obj["background"].as<const char *>();
    accent = obj["accent"].as<const char *>();
  }

  std::string background;
  std::string accent;
};

struct ThemeConfig {
public:
  void apply(JsonDocument doc) {
    firmwareUpdateScene = doc["firmwareUpdateScene"].as<const char *>();
    noKlipperScene = doc["noKlipperScene"].as<const char *>();
    startingScene = doc["startingScene"].as<const char *>();
    JsonArray requests = doc["requests"].as<JsonArray>();
    for (int i = 0; i < requests.size(); i++) {
      auto req = Request();
      req.apply(requests[i].as<JsonObject>());
      this->requests.push_back(req);
    }
    JsonArray scenes = doc["scenes"].as<JsonArray>();
    for (int i = 0; i < scenes.size(); i++) {
      auto scene = Scene();
      scene.apply(scenes[i].as<JsonObject>());
      this->scenes.push_back(scene);
    }
    this->variables.apply(doc["variables"].as<JsonObject>());
  }

  std::string firmwareUpdateScene;
  std::string noKlipperScene;
  std::vector<Request> requests;
  std::vector<Scene> scenes;
  std::string startingScene;
  Variables variables;
};

class ThemeConfigParser {
private:
  ThemeConfig _config;

public:
  ThemeConfigParser() {
    JsonDocument doc;
    deserializeJson(doc, THEMECONFIG, THEMECONFIG_SIZE);
    _config.apply(doc);
  }

  ThemeConfig *getConfig() { return &_config; }
};