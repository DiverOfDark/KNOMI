#pragma once
#include <string>
// Simple compile-time log level gating
// 0=DEBUG, 1=INFO, 2=WARN, 3=ERROR, 4=OFF
#ifndef KNOMI_LOG_LEVEL
#define KNOMI_LOG_LEVEL 1
#endif

__attribute__((format(printf, 5, 6))) void logToSerial(const char *logLevel, const char *file, int line,
                                                       const char *func, const char *format, ...);

#define LV_LOG_IGNORE(...)                                                                                             \
  do {                                                                                                                 \
  } while (false)

#define LV_LOG_HTTP(...) LV_LOG_IGNORE(__VA_ARGS__)
#define LV_LOG_RESOURCEIMAGE(...) LV_LOG_IGNORE(__VA_ARGS__)

#if KNOMI_LOG_LEVEL <= 0
#define LV_LOG_DEBUG(...) logToSerial("DEBUG", __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)
#else
#define LV_LOG_DEBUG(...) LV_LOG_IGNORE(__VA_ARGS__)
#endif

#if KNOMI_LOG_LEVEL <= 1
#define LV_LOG_INFO(...) logToSerial("INFO", __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)
#else
#define LV_LOG_INFO(...) LV_LOG_IGNORE(__VA_ARGS__)
#endif

#if KNOMI_LOG_LEVEL <= 2
#define LV_LOG_WARN(...) logToSerial("WARN", __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)
#else
#define LV_LOG_WARN(...) LV_LOG_IGNORE(__VA_ARGS__)
#endif
