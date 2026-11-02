// tools/i2c_scan/i2c_scan.ino
// Quet lap lai: USB CDC mat log luc reset, va de cam day roi xem ket qua ngay.
#include <Wire.h>

const uint32_t SCAN_PERIOD_MS = 5000;

void scanBus() {
  Serial.printf("Scanning I2C (SDA=%d, SCL=%d)...\n", SDA, SCL);
  int found = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("Found device at 0x%02X\n", addr);
      found++;
    }
  }
  Serial.printf("Done, %d device(s)\n", found);
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
}

void loop() {
  scanBus();
  delay(SCAN_PERIOD_MS);
}
