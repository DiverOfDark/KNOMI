#pragma once

#include <optional>

struct Variable {
  std::string name;
  std::string path;
  std::optional<std::string> stringFormat;
  std::string type;
};

struct Request {
  std::string id;
  std::string url;
  std::vector<Variable> variables;
};

enum class Size : int { LARGE, SMALL };

enum class Type : int { ARC, IMAGE, TEXT, TIMER };

struct Content {
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
  std::string condition;
  std::string target;
};

struct Scene {
  std::optional<std::string> background;
  std::vector<Content> content;
  std::string id;
  std::vector<Transition> transitions;
};

struct Variables {
  std::string background;
};

struct ThemeConfig {
  std::string firmwareUpdateScene;
  std::string noKlipperScene;
  std::vector<Request> requests;
  std::vector<Scene> scenes;
  std::string startingScene;
  Variables variables;
};

class ThemeConfigParser {
private:
  ThemeConfig* _config;

public:
  ThemeConfigParser() {

  }

  ThemeConfig* getConfig() {
    return _config;
  }
};