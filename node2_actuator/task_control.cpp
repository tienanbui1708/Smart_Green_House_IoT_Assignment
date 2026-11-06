// node2_actuator/task_control.cpp
// Luat Auto cua Node 2 (PROJECT.md 8.3) + che do an toan khi mat du lieu (8.2).
// Nguon du lieu: ESP-NOW tu Node 1 (task_espnow_rx) hoac song tam giac gia (USE_FAKE_SENSOR_DATA).
#include <app_log.h>
#include <protocol.h>
#include "config.h"
#include "shared_data.h"
#include "tasks.h"

namespace {
#if USE_FAKE_SENSOR_DATA
constexpr uint32_t FAKE_STEPS = 40;  // 40 buoc x 2 s = chu ky 80 s

// Gia tri chay tu lo -> hi -> lo theo buoc.
float triangle(uint32_t step, float lo, float hi) {
  uint32_t phase = step % FAKE_STEPS;
  uint32_t half = FAKE_STEPS / 2;
  float frac = phase < half ? (float)phase / half : (float)(FAKE_STEPS - phase) / half;
  return lo + (hi - lo) * frac;
}

void produceFakeSample(uint32_t &seq) {
  SensorView v = {};
  v.seq = ++seq;
  v.temperature = triangle(seq, 26.0f, 35.0f);
  v.humidity = triangle(seq + FAKE_STEPS / 4, 40.0f, 90.0f);
  v.soil = (uint8_t)triangle(seq + FAKE_STEPS / 2, 10.0f, 75.0f);
  v.aiClass = AI_CLASS_BOOTING;
  v.receivedMs = millis();
  sharedSetSensor(v);
#if DEBUG_LOG
  appLog("CTRL", "fake seq=%lu T=%.1f H=%.1f soil=%u", (unsigned long)v.seq, v.temperature, v.humidity, v.soil);
#endif
}
#endif

#if SELF_TEST_ALERT
// Thu LCD/NeoPixel cua cloudAlert khong can Cloud: L1 -> L2 -> xoa, moi buoc SELF_TEST_STEP_MS.
void runAlertSelfTest(uint8_t &step) {
  static const uint8_t LEVELS[] = {1, 2, 0};
  static const char *const MSGS[] = {"SELFTEST WARN", "SELFTEST CRIT", ""};
  if (step >= sizeof(LEVELS)) return;
  if (millis() < SELF_TEST_START_MS + step * (uint32_t)SELF_TEST_STEP_MS) return;
  sharedSetAlert(LEVELS[step], MSGS[step], SELF_TEST_STEP_MS);
  appLog("CTRL", "self-test cloudAlert level=%u", LEVELS[step]);
  step++;
}
#endif

bool fanRule(const SensorView &v) {
  return v.temperature > FAN_ON_T_ABOVE || v.humidity > FAN_ON_H_ABOVE;
}

// Tre (hysteresis): bat duoi 20 %, tat tren 60 %, o giua giu nguyen.
bool pumpRule(const SensorView &v, bool pumpNow) {
  if (v.soil < PUMP_ON_SOIL_BELOW) return true;
  if (v.soil > PUMP_OFF_SOIL_ABOVE) return false;
  return pumpNow;
}

// Node 1 bao loi bang ai_class 3 hoac gia tri sentinel: khong dieu khien theo gia tri hong.
bool sensorFault(const SensorView &v) {
  bool sentinel = v.temperature == SENTINEL_T && v.humidity == SENTINEL_H;  // gan nguyen gia tri, so sanh bang an toan
  return v.aiClass == AI_CLASS_SENSOR_FAULT || sentinel;
}

void applyRules() {
  SensorView v = sharedGetSensor();
  ActuatorState a = sharedGetActuators();
  bool stale = v.seq == 0 || millis() - v.receivedMs > ESPNOW_STALE_MS;
  bool fault = !stale && sensorFault(v);
  if (stale != a.dataStale) appLog("CTRL", stale ? "data stale -> safe mode" : "data fresh");
  if (fault != a.dataFault) appLog("CTRL", fault ? "node1 sensor fault -> safe mode" : "node1 sensor ok");
  if (stale || fault) {
    sharedApplyAuto(false, false, stale, fault);  // an toan: tat chap hanh mo phong
    return;
  }
  sharedApplyAuto(fanRule(v), pumpRule(v, a.pump), false, false);
}
}  // namespace

void taskControl(void *) {
#if USE_FAKE_SENSOR_DATA
  appLog("CTRL", "USING FAKE SENSOR DATA (USE_FAKE_SENSOR_DATA=1, ESP-NOW ignored)");
  uint32_t seq = 0;
  uint32_t lastFakeMs = millis() - FAKE_DATA_PERIOD_MS;
#endif
#if SELF_TEST_ALERT
  uint8_t selfTestStep = 0;
#endif
  for (;;) {
#if SELF_TEST_ALERT
    runAlertSelfTest(selfTestStep);
#endif
#if USE_FAKE_SENSOR_DATA
    if (millis() - lastFakeMs >= FAKE_DATA_PERIOD_MS) {
      produceFakeSample(seq);
      lastFakeMs = millis();
    }
#endif
    applyRules();
    vTaskDelay(pdMS_TO_TICKS(CONTROL_PERIOD_MS));
  }
}
