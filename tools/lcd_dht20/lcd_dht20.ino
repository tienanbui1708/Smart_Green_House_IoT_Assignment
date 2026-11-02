// tools/lcd_dht20/lcd_dht20.ino
// Kiem tra thu vien LCD + DHT20 theo bai 10 OhStem; khong lcd.clear() moi vong.
#include <Wire.h>
#include "LiquidCrystal_I2C.h"
#include "DHT20.h"

const uint8_t LCD_ADDR = 0x21;           // TODO(verify): doi theo ket qua i2c_scan
const uint32_t READ_PERIOD_MS = 2000;    // DHT20 tu choi doc nhanh hon 1 s

LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);
DHT20 dht;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  lcd.init();
  lcd.backlight();
  Serial.printf("DHT20 connected: %s\n", dht.begin() ? "yes" : "NO");
}

void loop() {
  int rc = dht.read();
  char line[17];
  if (rc == DHT20_OK) {
    float t = dht.getTemperature(), h = dht.getHumidity();
    Serial.printf("T=%.2f H=%.2f\n", t, h);
    snprintf(line, sizeof(line), "T:%4.1f H:%4.1f   ", t, h);
  } else {
    Serial.printf("DHT20 read failed rc=%d\n", rc);
    snprintf(line, sizeof(line), "DHT20 ERR %-6d", rc);
  }
  lcd.setCursor(0, 0);
  lcd.print(line);
  delay(READ_PERIOD_MS);
}
