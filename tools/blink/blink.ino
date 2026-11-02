// tools/blink/blink.ino
const uint32_t HALF_PERIOD_MS = 500;

void setup() {
  pinMode(D13, OUTPUT);
}

void loop() {
  digitalWrite(D13, HIGH);
  delay(HALF_PERIOD_MS);
  digitalWrite(D13, LOW);
  delay(HALF_PERIOD_MS);
}
