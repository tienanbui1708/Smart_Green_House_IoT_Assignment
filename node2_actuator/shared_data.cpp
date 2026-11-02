// node2_actuator/shared_data.cpp
#include "shared_data.h"

namespace {
SemaphoreHandle_t dataMutex = nullptr;

SensorView sensor = {};
ActuatorState actuators = {true, false, false, true};
NetState net = {};

struct DataGuard {
  DataGuard() { xSemaphoreTake(dataMutex, portMAX_DELAY); }
  ~DataGuard() { xSemaphoreGive(dataMutex); }
};
}  // namespace

void sharedInit() {
  dataMutex = xSemaphoreCreateMutex();
  configASSERT(dataMutex != nullptr);
}

void sharedSetSensor(const SensorView &v) {
  DataGuard g;
  sensor = v;
}

SensorView sharedGetSensor() {
  DataGuard g;
  return sensor;
}

ActuatorState sharedGetActuators() {
  DataGuard g;
  return actuators;
}

void sharedSetAutoMode(bool autoMode) {
  DataGuard g;
  actuators.autoMode = autoMode;
}

CmdResult sharedSetManualFan(bool on) {
  DataGuard g;
  if (actuators.autoMode) return CmdResult::RejectedAutoMode;
  actuators.fan = on;
  return CmdResult::Ok;
}

CmdResult sharedSetManualPump(bool on) {
  DataGuard g;
  if (actuators.autoMode) return CmdResult::RejectedAutoMode;
  actuators.pump = on;
  return CmdResult::Ok;
}

void sharedApplyAuto(bool fan, bool pump, bool dataStale) {
  DataGuard g;
  actuators.dataStale = dataStale;
  if (!actuators.autoMode) return;
  actuators.fan = fan;
  actuators.pump = pump;
}

void sharedSetNet(const NetState &n) {
  DataGuard g;
  net = n;
}

NetState sharedGetNet() {
  DataGuard g;
  return net;
}
