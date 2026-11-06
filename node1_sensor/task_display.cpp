// node1_sensor/task_display.cpp
// LCD 16x2: ghi de chuoi do dai co dinh, khong clear(); chi ghi khi noi dung doi.
//   Dong 0: "T:28.4 H:64.2 0C"   (so = nhan AI 0..3, '-' dang khoi dong; C = MQTT, W = chi Wi-Fi)
//   Dong 1: "Soil 41% Ht255 A"   (A = Auto, M = Manual)
#include "LiquidCrystal_I2C.h"
#include <app_log.h>
#include <protocol.h>
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

char aiChar(uint8_t cls) {
  return cls <= AI_CLASS_SENSOR_FAULT ? (char)('0' + cls) : '-';
}

void formatLine0(char *out, size_t size, const SensorSample &s, uint8_t cls, const NetState &n) {
  char body[LCD_COLS];
  if (s.seq == 0) {
    snprintf(body, sizeof(body), "Starting...");
  } else if (!s.dhtOk || cls == AI_CLASS_SENSOR_FAULT) {
    snprintf(body, sizeof(body), "FAULT %s", s.dhtOk ? "(AI)" : "DHT20");
  } else {
    snprintf(body, sizeof(body), "T:%4.1f H:%4.1f %c", s.temperature, s.humidity, aiChar(cls));
  }
  snprintf(out, size, "%-*.*s%c", LCD_COLS - 1, LCD_COLS - 1, body, netChar(n));
}

void formatLine1(char *out, size_t size, const SensorSample &s, const ControlState &c) {
  snprintf(out, size, "Soil%3u%% Ht%3u %c", s.soil, c.heater, c.autoMode ? 'A' : 'M');
}

bool writeLines(const char *line0, const char *line1) {
  if (!i2cLock(I2C_LOCK_TIMEOUT_MS)) return false;
  lcd.setCursor(0, 0);
  lcd.print(line0);
  lcd.setCursor(0, 1);
  lcd.print(line1);
  i2cUnlock();
  return true;
}
}  // namespace

void taskDisplay(void *) {
  if (i2cLock(I2C_LOCK_TIMEOUT_MS)) {
    lcd.init();
    lcd.backlight();
    i2cUnlock();
  } else {
    appLog("LCD", "init: I2C busy");
  }

  char shown0[LCD_COLS + 1] = "";
  char shown1[LCD_COLS + 1] = "";
  for (;;) {
    SensorSample s = sharedGetSample();
    ControlState c = sharedGetControl();
    NetState n = sharedGetNet();

    char line0[LCD_COLS + 1], line1[LCD_COLS + 1];
    formatLine0(line0, sizeof(line0), s, sharedGetAiClass(), n);
    formatLine1(line1, sizeof(line1), s, c);

    bool changed = strcmp(line0, shown0) != 0 || strcmp(line1, shown1) != 0;
    if (changed && writeLines(line0, line1)) {
      strcpy(shown0, line0);
      strcpy(shown1, line1);
    }
    vTaskDelay(pdMS_TO_TICKS(DISPLAY_PERIOD_MS));
  }
}
