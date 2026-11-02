// node2_actuator/task_actuator.cpp
// D13 = quat mo phong; NeoPixel = muc do am khong khi (xanh duong / vang / do). Chi ghi khi doi.
#include <Adafruit_NeoPixel.h>
#include <app_log.h>
#include "config.h"
#include "shared_data.h"
#include "tasks.h"

namespace {
enum class HumColor : uint8_t { Off, Blue, Yellow, Red };

Adafruit_NeoPixel pixel(NEOPIXEL_COUNT, PIN_NEOPIXEL, NEOPIXEL_TYPE);

HumColor colorForHumidity(float h, bool stale) {
  if (stale) return HumColor::Off;
  if (h < NEO_H_BLUE_BELOW) return HumColor::Blue;
  if (h > NEO_H_RED_ABOVE) return HumColor::Red;
  return HumColor::Yellow;
}

void showColor(HumColor c) {
  uint32_t rgb = 0;
  switch (c) {
    case HumColor::Blue: rgb = pixel.Color(0, 0, 255); break;
    case HumColor::Yellow: rgb = pixel.Color(255, 180, 0); break;
    case HumColor::Red: rgb = pixel.Color(255, 0, 0); break;
    case HumColor::Off: rgb = 0; break;
  }
  pixel.fill(rgb);
  pixel.show();
}
}  // namespace

void taskActuator(void *) {
  pinMode(PIN_FAN_LED, OUTPUT);
  digitalWrite(PIN_FAN_LED, LOW);
  pixel.begin();
  pixel.setBrightness(NEOPIXEL_BRIGHTNESS);
  showColor(HumColor::Off);

  int shownFan = -1;
  int shownPump = -1;  // bom chi hien tren LCD, o day chi de log
  HumColor shownColor = HumColor::Off;
  for (;;) {
    ActuatorState a = sharedGetActuators();
    SensorView v = sharedGetSensor();

    if ((int)a.fan != shownFan || (int)a.pump != shownPump) {
      digitalWrite(PIN_FAN_LED, a.fan ? HIGH : LOW);
      appLog("ACT", "fan=%s pump=%s (%s)", a.fan ? "ON" : "OFF", a.pump ? "ON" : "OFF", a.autoMode ? "auto" : "manual");
      shownFan = a.fan;
      shownPump = a.pump;
    }
    HumColor c = colorForHumidity(v.humidity, a.dataStale);
    if (c != shownColor) {
      showColor(c);
      shownColor = c;
    }
    vTaskDelay(pdMS_TO_TICKS(ACTUATOR_PERIOD_MS));
  }
}
