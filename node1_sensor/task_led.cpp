// node1_sensor/task_led.cpp
// Luat suoi Auto (PROJECT.md 8.3) + an toan cung (8.4); NeoPixel kenh do = muc suoi mo phong.
#include <Adafruit_NeoPixel.h>
#include <app_log.h>
#include "config.h"
#include "shared_data.h"
#include "tasks.h"

namespace {
Adafruit_NeoPixel pixel(NEOPIXEL_COUNT, PIN_NEOPIXEL, NEOPIXEL_TYPE);

uint8_t heaterForTemperature(float t) {
  if (t < HEATER_T_FULL_BELOW) return HEATER_PWM_FULL;
  if (t < HEATER_T_OFF_FROM) return HEATER_PWM_MID;
  return HEATER_PWM_OFF;
}

void applyAutoRules(const SensorSample &s) {
  // Mat cam bien thi tat suoi: khong biet nhiet do thi khong suoi.
  if (!s.dhtOk) {
    sharedSetAutoHeater(HEATER_PWM_OFF);
    return;
  }
  if (s.temperature >= SAFETY_T_FORCE_AUTO) {
    if (sharedForceSafe()) appLog("LED", "SAFETY: T=%.1f >= %.1f, force Auto + heater off", s.temperature, SAFETY_T_FORCE_AUTO);
    return;
  }
  sharedSetAutoHeater(heaterForTemperature(s.temperature));
}

void showHeater(uint8_t heater) {
  pixel.fill(pixel.Color(heater, 0, 0));
  pixel.show();
}
}  // namespace

void taskLed(void *) {
  pixel.begin();
  pixel.setBrightness(NEOPIXEL_BRIGHTNESS);
  showHeater(HEATER_PWM_OFF);

  uint32_t lastSeq = 0;
  int shownHeater = -1;
  for (;;) {
    SensorSample s = sharedGetSample();
    if (s.seq != 0 && s.seq != lastSeq) {
      applyAutoRules(s);
      lastSeq = s.seq;
    }
    ControlState c = sharedGetControl();
    if (c.heater != shownHeater) {  // chi show() khi doi
      showHeater(c.heater);
      appLog("LED", "heater=%u (%s)", c.heater, c.autoMode ? "auto" : "manual");
      shownHeater = c.heater;
    }
    vTaskDelay(pdMS_TO_TICKS(LED_PERIOD_MS));
  }
}
