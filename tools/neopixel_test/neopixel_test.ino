// tools/neopixel_test/neopixel_test.ino
// RGB_LED la macro cua variant yolo_uno (GPIO45). Mau sai thu tu -> doi NEO_GRB sang NEO_RGB.
#include <Adafruit_NeoPixel.h>

const uint16_t PIXEL_COUNT = 1;          // TODO(verify): so bong NeoPixel onboard
const uint8_t BRIGHTNESS = 40;
const uint32_t STEP_MS = 700;

Adafruit_NeoPixel neo(PIXEL_COUNT, RGB_LED, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  neo.begin();
  neo.setBrightness(BRIGHTNESS);
}

void loop() {
  const char *names[] = {"RED", "GREEN", "BLUE", "YELLOW"};
  const uint32_t colors[] = {neo.Color(255, 0, 0), neo.Color(0, 255, 0), neo.Color(0, 0, 255), neo.Color(255, 180, 0)};
  for (int i = 0; i < 4; i++) {
    Serial.printf("NeoPixel (GPIO%d): %s\n", RGB_LED, names[i]);
    neo.fill(colors[i]);
    neo.show();
    delay(STEP_MS);
  }
}
