// node1_sensor/shared_data.cpp
#include "shared_data.h"
#include "config.h"

namespace {
SemaphoreHandle_t dataMutex = nullptr;
SemaphoreHandle_t i2cMutex = nullptr;

SensorSample sample = {};
ControlState control = {true, HEATER_PWM_OFF};
NetState net = {};

// RAII: moi duong return deu tra mutex.
struct DataGuard {
  DataGuard() { xSemaphoreTake(dataMutex, portMAX_DELAY); }
  ~DataGuard() { xSemaphoreGive(dataMutex); }
};
}  // namespace

void sharedInit() {
  dataMutex = xSemaphoreCreateMutex();
  i2cMutex = xSemaphoreCreateMutex();
  configASSERT(dataMutex != nullptr && i2cMutex != nullptr);
}

void sharedSetSample(const SensorSample &s) {
  DataGuard g;
  sample = s;
}

SensorSample sharedGetSample() {
  DataGuard g;
  return sample;
}

ControlState sharedGetControl() {
  DataGuard g;
  return control;
}

void sharedSetAutoMode(bool autoMode) {
  DataGuard g;
  control.autoMode = autoMode;
}

CmdResult sharedSetManualHeater(uint8_t value) {
  DataGuard g;
  if (control.autoMode) return CmdResult::RejectedAutoMode;
  control.heater = value;
  return CmdResult::Ok;
}

void sharedSetAutoHeater(uint8_t value) {
  DataGuard g;
  if (control.autoMode) control.heater = value;
}

bool sharedForceSafe() {
  DataGuard g;
  bool changed = !control.autoMode || control.heater != HEATER_PWM_OFF;
  control.autoMode = true;
  control.heater = HEATER_PWM_OFF;
  return changed;
}

void sharedSetNet(const NetState &n) {
  DataGuard g;
  net = n;
}

NetState sharedGetNet() {
  DataGuard g;
  return net;
}

bool i2cLock(uint32_t timeoutMs) {
  return xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
}

void i2cUnlock() {
  xSemaphoreGive(i2cMutex);
}
