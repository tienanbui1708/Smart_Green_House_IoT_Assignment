// node2_actuator/task_control.cpp
// Luat Auto cua Node 2 (PROJECT.md 8.3) + che do an toan khi mat du lieu (8.2).
// M2: nguon du lieu la song tam giac gia de quet qua moi nguong; M5 thay bang ESP-NOW.
#include <app_log.h>
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
  v.receivedMs = millis();
  sharedSetSensor(v);
#if DEBUG_LOG
  appLog("CTRL", "fake seq=%lu T=%.1f H=%.1f soil=%u", (unsigned long)v.seq, v.temperature, v.humidity, v.soil);
#endif
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

void applyRules() {
  SensorView v = sharedGetSensor();
  ActuatorState a = sharedGetActuators();
  bool stale = v.seq == 0 || millis() - v.receivedMs > DATA_STALE_MS;
  if (stale != a.dataStale) appLog("CTRL", stale ? "data stale -> safe mode" : "data fresh");
  if (stale) {
    sharedApplyAuto(false, false, true);  // an toan: tat chap hanh mo phong
    return;
  }
  sharedApplyAuto(fanRule(v), pumpRule(v, a.pump), false);
}
}  // namespace

void taskControl(void *) {
#if USE_FAKE_SENSOR_DATA
  appLog("CTRL", "USING FAKE SENSOR DATA (no ESP-NOW yet)");
  uint32_t seq = 0;
  uint32_t lastFakeMs = millis() - FAKE_DATA_PERIOD_MS;
#endif
  for (;;) {
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
