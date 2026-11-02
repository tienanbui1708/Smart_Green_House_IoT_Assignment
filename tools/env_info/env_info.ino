// tools/env_info/env_info.ino
// In lap lai vi cong USB CDC mat ket noi luc reset, in mot lan se bi lo log.
const uint32_t PRINT_PERIOD_MS = 5000;

void printEnvInfo() {
  Serial.println("=== ENV INFO ===");
  Serial.printf("Chip: %s, cores: %d, rev: %d\n", ESP.getChipModel(), ESP.getChipCores(), ESP.getChipRevision());
  Serial.printf("CPU MHz: %u, Flash: %u bytes, PSRAM: %u bytes\n", ESP.getCpuFreqMHz(), ESP.getFlashChipSize(), ESP.getPsramSize());
  Serial.printf("SDK: %s\n", ESP.getSdkVersion());
  Serial.printf("arduino-esp32 core: %d.%d.%d\n", ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR, ESP_ARDUINO_VERSION_PATCH);
  Serial.printf("Sketch: %u bytes, free OTA slot: %u bytes\n", ESP.getSketchSize(), ESP.getFreeSketchSpace());
}

void setup() {
  Serial.begin(115200);
}

void loop() {
  printEnvInfo();
  delay(PRINT_PERIOD_MS);
}
