// node2_actuator/task_display.cpp
// LCD 16x2, ghi de do dai co dinh, khong clear(); chi task nay dung I2C o Node 2.
//   Dong 0: "T28.4 H 64 S 41C"   (C = MQTT, W = chi Wi-Fi)
//   Dong 1: "PUMP:ON  F:OFF A"   (A = Auto, M = Manual); khi co cloudAlert: msg canh bao 16 ky tu.
#include "LiquidCrystal_I2C.h"
#include "config.h"
#include "shared_data.h"
#include "tasks.h"

namespace {
LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);

char netChar(const NetState &n) {
  if (n.mqtt) return 'C';
  if (n.wifi) return 'W';
  return ' ';
}

void formatLine0(char *out, size_t size, const SensorView &v, const ActuatorState &a, const NetState &n) {
  char body[LCD_COLS];
  if (a.dataStale) {
    snprintf(body, sizeof(body), "NO DATA");
  } else {
    snprintf(body, sizeof(body), "T%4.1f H%3.0f S%3u", v.temperature, v.humidity, v.soil);
  }
  snprintf(out, size, "%-*.*s%c", LCD_COLS - 1, LCD_COLS - 1, body, netChar(n));
}

void formatLine1(char *out, size_t size, const ActuatorState &a, const CloudAlertState &al) {
  if (al.level != 0) {
    char body[LCD_COLS + 1];
    if (al.msg[0] != '\0') snprintf(body, sizeof(body), "%s", al.msg);
    else snprintf(body, sizeof(body), "ALERT L%u", al.level);
    snprintf(out, size, "%-*.*s", LCD_COLS, LCD_COLS, body);  // pad du 16 de ghi de, khong clear()
    return;
  }
  snprintf(out, size, "PUMP:%-3s F:%-3s %c", a.pump ? "ON" : "OFF", a.fan ? "ON" : "OFF", a.autoMode ? 'A' : 'M');
}
}  // namespace

void taskDisplay(void *) {
  lcd.init();
  lcd.backlight();

  char shown0[LCD_COLS + 1] = "";
  char shown1[LCD_COLS + 1] = "";
  for (;;) {
    SensorView v = sharedGetSensor();
    ActuatorState a = sharedGetActuators();
    NetState n = sharedGetNet();
    CloudAlertState al = sharedGetAlert();

    char line0[LCD_COLS + 1], line1[LCD_COLS + 1];
    formatLine0(line0, sizeof(line0), v, a, n);
    formatLine1(line1, sizeof(line1), a, al);
    if (strcmp(line0, shown0) != 0) {
      lcd.setCursor(0, 0);
      lcd.print(line0);
      strcpy(shown0, line0);
    }
    if (strcmp(line1, shown1) != 0) {
      lcd.setCursor(0, 1);
      lcd.print(line1);
      strcpy(shown1, line1);
    }
    vTaskDelay(pdMS_TO_TICKS(DISPLAY_PERIOD_MS));
  }
}
