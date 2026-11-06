// tools/tinyml_parity/tinyml_parity.ino
// Parity test TinyML (T04, TASKS_SPEC 5.3): chay 20 vector cua ml/export_tflite.py tren thiet bi,
// so voi du doan Python, in thoi gian suy luan va arena thuc dung.
// model.h + test_vectors.h do 'python ml/export_tflite.py --install' chep vao thu muc nay.
// Luu log: docs/evidence/T04/parity_log.txt
#include <Chirale_TensorFlowLite.h>
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "model.h"
#include "test_vectors.h"

constexpr size_t kArenaSize = 8 * 1024;  // giam dan de tim muc toi thieu, roi + 20 %
constexpr int kClasses = 4;
alignas(16) static uint8_t tensorArena[kArenaSize];

void writeInput(TfLiteTensor *in, const float *x) {
  for (int k = 0; k < TV_DIM; k++) {
#if MODEL_IS_INT8
    long q = lroundf(x[k] / in->params.scale) + in->params.zero_point;  // scale/zp doc tu tensor
    in->data.int8[k] = (int8_t)constrain(q, -128L, 127L);
#else
    in->data.f[k] = x[k];
#endif
  }
}

int argmaxOutput(const TfLiteTensor *out, float &best) {
  int idx = 0;
  best = -1e9f;
  for (int c = 0; c < kClasses; c++) {
#if MODEL_IS_INT8
    float v = (out->data.int8[c] - out->params.zero_point) * out->params.scale;
#else
    float v = out->data.f[c];
#endif
    if (v > best) {
      best = v;
      idx = c;
    }
  }
  return idx;
}

void runParity() {
  const tflite::Model *model = tflite::GetModel(g_model);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    Serial.printf("schema %lu != %d\n", (unsigned long)model->version(), TFLITE_SCHEMA_VERSION);
    return;
  }
  static tflite::MicroMutableOpResolver<2> resolver;
  resolver.AddFullyConnected();
  resolver.AddSoftmax();
  static tflite::MicroInterpreter interp(model, resolver, tensorArena, kArenaSize);
  if (interp.AllocateTensors() != kTfLiteOk) {
    Serial.printf("AllocateTensors FAILED (arena %u B): tang kArenaSize\n", (unsigned)kArenaSize);
    return;
  }
  Serial.printf("model %u B, %s, demo=%d, arena %u B, dung %u B\n", g_model_len, MODEL_IS_INT8 ? "int8" : "float32",
                MODEL_IS_DEMO, (unsigned)kArenaSize, (unsigned)interp.arena_used_bytes());
  int ok = 0;
  uint32_t totalUs = 0;
  for (int i = 0; i < TV_N; i++) {
    writeInput(interp.input(0), TV_X[i]);
    uint32_t t0 = micros();
    TfLiteStatus st = interp.Invoke();
    uint32_t dt = micros() - t0;
    totalUs += dt;
    float p = 0;
    int got = st == kTfLiteOk ? argmaxOutput(interp.output(0), p) : -1;
    bool match = got == TV_EXPECT[i];
    ok += match;
    Serial.printf("vec %2d: device=%d expect=%d p=%.3f %s (%lu us)\n", i, got, TV_EXPECT[i], p, match ? "OK" : "MISMATCH",
                  (unsigned long)dt);
  }
  Serial.printf("PARITY: %d/%d khop, TB %lu us/lan suy luan\n", ok, TV_N, (unsigned long)(totalUs / TV_N));
}

void setup() {
  Serial.begin(115200);
  delay(2000);  // sketch kiem tra mot lan (khong phai firmware chinh): cho mo Serial Monitor
  runParity();
}

void loop() {
  delay(1000);
}
