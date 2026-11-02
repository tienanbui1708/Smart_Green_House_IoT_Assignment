// tools/adc_calibrate/adc_calibrate.ino
// In gia tri tho A1 (trung binh, min, max) de lay RAW_DRY / RAW_WET cho config.h.
const int SAMPLES = 16;
const uint32_t SAMPLE_GAP_MS = 10;
const uint32_t PRINT_PERIOD_MS = 500;

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);   // chot 12-bit (0..4095) de so do khop firmware
}

void loop() {
  long sum = 0;
  int lo = 4095, hi = 0;
  for (int i = 0; i < SAMPLES; i++) {
    int v = analogRead(A1);
    sum += v;
    lo = min(lo, v);
    hi = max(hi, v);
    delay(SAMPLE_GAP_MS);
  }
  Serial.printf("raw avg=%ld min=%d max=%d\n", sum / SAMPLES, lo, hi);
  delay(PRINT_PERIOD_MS);
}
