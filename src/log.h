#pragma once
#include <string>

void logToSerial(const char *logLevel, const char *file, int line, const char *func, const char *format, ...);

#define LV_LOG_IGNORE(...)                                                                                             \
  do {                                                                                                                 \
  } while (false)

#define LV_LOG_HTTP(...) LV_LOG_IGNORE(__VA_ARGS__)
#define LV_LOG_RESOURCEIMAGE(...) LV_LOG_IGNORE(__VA_ARGS__)

#define LV_LOG_DEBUG(...)                                                                                              \
  LV_LOG_IGNORE(__VA_ARGS__) // logToSerial("DEBUG", __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)

#define LV_LOG_INFO(...) logToSerial("INFO", __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)
#define LV_LOG_WARN(...) logToSerial("WARN", __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)
