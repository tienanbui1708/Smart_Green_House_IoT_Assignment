// tools/print_mac/print_mac.ino
// MAC dung de khai bao peer ESP-NOW; phai vao WIFI_STA truoc khi doc.
#include <WiFi.h>

const uint32_t PRINT_PERIOD_MS = 5000;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
}

void loop() {
  Serial.printf("MAC: %s\n", WiFi.macAddress().c_str());
  delay(PRINT_PERIOD_MS);
}
