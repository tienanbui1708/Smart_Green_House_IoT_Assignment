// node2_actuator/shared_data.cpp
#include "shared_data.h"

namespace {
SemaphoreHandle_t dataMutex = nullptr;

SensorView sensor = {};
ActuatorState actuators = {true, false, false, true, false};
NetState net = {};
CloudAlertState alert = {};

struct DataGuard {
  DataGuard() { xSemaphoreTake(dataMutex, portMAX_DELAY); }
  ~DataGuard() { xSemaphoreGive(dataMutex); }
};

// LCD1602 khong co dau tieng Viet: moi ky tu ngoai ASCII in duoc (ke ca chuoi UTF-8 nhieu byte) -> '?'.
void sanitizeAlertMsg(const char *in, char *out, size_t outSize) {
  size_t n = 0;
  for (const uint8_t *p = (const uint8_t *)(in ? in : ""); *p != 0 && n + 1 < outSize; p++) {
    if ((*p & 0xC0) == 0x80) continue;  // byte noi tiep UTF-8: da thay bang '?' o byte dau
    out[n++] = (*p >= 0x20 && *p < 0x7F) ? (char)*p : '?';
  }
  out[n] = '\0';
}

bool alertActive(const CloudAlertState &a) {
  return a.level != 0 && (int32_t)(millis() - a.expireMs) < 0;  // an toan khi millis() tran
}
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

void sharedApplyAuto(bool fan, bool pump, bool dataStale, bool dataFault) {
  DataGuard g;
  actuators.dataStale = dataStale;
  actuators.dataFault = dataFault;
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

void sharedSetAlert(uint8_t level, const char *msg, uint32_t durationMs) {
  CloudAlertState a = {};
  a.level = level;
  sanitizeAlertMsg(msg, a.msg, sizeof(a.msg));  // xu ly ngoai mutex
  a.expireMs = millis() + (level == 0 ? 0 : durationMs);
  DataGuard g;
  alert = a;
}

CloudAlertState sharedGetAlert() {
  CloudAlertState a;
  {
    DataGuard g;
    a = alert;
  }
  if (!alertActive(a)) a.level = 0;
  return a;
}
