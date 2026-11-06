// node1_sensor/shared_data.cpp
#include "shared_data.h"
#include <protocol.h>
#include "config.h"

namespace {
SemaphoreHandle_t dataMutex = nullptr;
SemaphoreHandle_t i2cMutex = nullptr;
SemaphoreHandle_t newSampleSem = nullptr;  // binary: taskSensor give, taskTinyml take

SensorSample sample = {};
ControlState control = {true, HEATER_PWM_OFF};
NetState net = {};
bool safeHold = false;  // true: sap khoi dong lai, khong cho bat suoi nua
EspNowStats espnow = {};
uint8_t aiClass = AI_CLASS_BOOTING;

// RAII: moi duong return deu tra mutex.
struct DataGuard {
  DataGuard() { xSemaphoreTake(dataMutex, portMAX_DELAY); }
  ~DataGuard() { xSemaphoreGive(dataMutex); }
};
}  // namespace

void sharedInit() {
  dataMutex = xSemaphoreCreateMutex();
  i2cMutex = xSemaphoreCreateMutex();
  newSampleSem = xSemaphoreCreateBinary();
  configASSERT(dataMutex != nullptr && i2cMutex != nullptr && newSampleSem != nullptr);
}

void sharedSetSample(const SensorSample &s) {
  {
    DataGuard g;
    sample = s;
  }
  xSemaphoreGive(newSampleSem);  // khong chan taskSensor neu chua ai lay
}

bool sharedWaitNewSample(uint32_t timeoutMs) {
  return xSemaphoreTake(newSampleSem, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
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
  if (!safeHold) control.autoMode = autoMode;
}

CmdResult sharedSetManualHeater(uint8_t value) {
  DataGuard g;
  if (control.autoMode || safeHold) return CmdResult::RejectedAutoMode;
  control.heater = value;
  return CmdResult::Ok;
}

void sharedSetAutoHeater(uint8_t value) {
  DataGuard g;
  if (control.autoMode && !safeHold) control.heater = value;
}

bool sharedForceSafe() {
  DataGuard g;
  bool changed = !control.autoMode || control.heater != HEATER_PWM_OFF;
  control.autoMode = true;
  control.heater = HEATER_PWM_OFF;
  return changed;
}

void sharedHoldSafe() {
  DataGuard g;
  safeHold = true;
  control.autoMode = true;
  control.heater = HEATER_PWM_OFF;
}

void sharedSetNet(const NetState &n) {
  DataGuard g;
  net = n;
}

NetState sharedGetNet() {
  DataGuard g;
  return net;
}

void sharedSetAiClass(uint8_t cls) {
  DataGuard g;
  aiClass = cls;
}

uint8_t sharedGetAiClass() {
  DataGuard g;
  return aiClass;
}

void sharedSetEspNow(const EspNowStats &s) {
  DataGuard g;
  espnow = s;
}

EspNowStats sharedGetEspNow() {
  DataGuard g;
  return espnow;
}

bool i2cLock(uint32_t timeoutMs) {
  return xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
}

void i2cUnlock() {
  xSemaphoreGive(i2cMutex);
}
