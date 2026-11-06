// node2_actuator/task_actuator.cpp
// D13 = quat mo phong; NeoPixel = muc do am khong khi (xanh duong / vang / do). Chi ghi khi doi.
// cloudAlert ghi de mau NeoPixel (chop), nhung khong dong vao D13 va trang thai bom/quat.
#include <Adafruit_NeoPixel.h>
#include <app_log.h>
#include "config.h"
#include "shared_data.h"
#include "tasks.h"

namespace {
Adafruit_NeoPixel pixel(NEOPIXEL_COUNT, PIN_NEOPIXEL, NEOPIXEL_TYPE);

uint32_t colorForHumidity(float h, bool stale) {
  if (stale) return 0;
  if (h < NEO_H_BLUE_BELOW) return pixel.Color(0, 0, 255);
  if (h > NEO_H_RED_ABOVE) return pixel.Color(255, 0, 0);
  return pixel.Color(255, 180, 0);
}

// Sang nua dau chu ky, tat nua sau; tinh tu millis() nen khong can delay.
uint32_t colorForAlert(uint8_t level) {
  uint32_t period = level >= 2 ? ALERT_BLINK_L2_MS : ALERT_BLINK_L1_MS;
  bool on = (millis() % period) < period / 2;
  if (!on) return 0;
  return level >= 2 ? pixel.Color(ALERT_L2_RGB) : pixel.Color(ALERT_L1_RGB);
}

void showColor(uint32_t rgb) {
  pixel.fill(rgb);
  pixel.show();
}

void updateFan(const ActuatorState &a, int &shownFan, int &shownPump) {
  if ((int)a.fan == shownFan && (int)a.pump == shownPump) return;
  digitalWrite(PIN_FAN_LED, a.fan ? HIGH : LOW);
  appLog("ACT", "fan=%s pump=%s (%s)", a.fan ? "ON" : "OFF", a.pump ? "ON" : "OFF", a.autoMode ? "auto" : "manual");
  shownFan = a.fan;
  shownPump = a.pump;
}
}  // namespace

void taskActuator(void *) {
  pinMode(PIN_FAN_LED, OUTPUT);
  digitalWrite(PIN_FAN_LED, LOW);
  pixel.begin();
  pixel.setBrightness(NEOPIXEL_BRIGHTNESS);
  showColor(0);

  int shownFan = -1;
  int shownPump = -1;  // bom chi hien tren LCD, o day chi de log
  uint32_t shownColor = 0;
  uint8_t shownAlert = 0;
  for (;;) {
    ActuatorState a = sharedGetActuators();
    SensorView v = sharedGetSensor();
    CloudAlertState al = sharedGetAlert();
    updateFan(a, shownFan, shownPump);

    if (al.level != shownAlert) {
      appLog("ACT", "cloudAlert level=%u", al.level);
      shownAlert = al.level;
    }
    uint32_t c = al.level != 0 ? colorForAlert(al.level) : colorForHumidity(v.humidity, a.dataStale);
    if (c != shownColor) {
      showColor(c);
      shownColor = c;
    }
    vTaskDelay(pdMS_TO_TICKS(ACTUATOR_PERIOD_MS));
  }
}
