// common/app_log.cpp
#include <Arduino.h>
#include <stdarg.h>
#include "app_log.h"

namespace {
constexpr size_t LOG_LINE_MAX = 192;
constexpr TickType_t LOG_LOCK_TIMEOUT = pdMS_TO_TICKS(50);
SemaphoreHandle_t serialMutex = nullptr;

void writeLine(const char *tag, const char *line) {
  // Het thoi gian cho thi bo dong log, khong de task dieu khien bi chan vi Serial.
  if (serialMutex != nullptr && xSemaphoreTake(serialMutex, LOG_LOCK_TIMEOUT) != pdTRUE) return;
  if (tag != nullptr) Serial.printf("[%s] %s\n", tag, line);
  else Serial.printf("%s\n", line);
  if (serialMutex != nullptr) xSemaphoreGive(serialMutex);
}
}  // namespace

void logInit() {
  serialMutex = xSemaphoreCreateMutex();
}

void appLog(const char *tag, const char *fmt, ...) {
  // Dinh dang truoc khi lay mutex de giu mutex ngan nhat co the.
  char line[LOG_LINE_MAX];
  va_list args;
  va_start(args, fmt);
  vsnprintf(line, sizeof(line), fmt, args);
  va_end(args);
  writeLine(tag, line);
}

void appLogRaw(const char *fmt, ...) {
  char line[LOG_LINE_MAX];
  va_list args;
  va_start(args, fmt);
  vsnprintf(line, sizeof(line), fmt, args);
  va_end(args);
  writeLine(nullptr, line);
}
