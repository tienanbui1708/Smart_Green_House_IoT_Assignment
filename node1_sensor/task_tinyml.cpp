// node1_sensor/task_tinyml.cpp
// Suy luan MLP 15->32->16->4 (TFLite Micro, Chirale_TensorFlowLite) tren cua so 5 mau (T04, TASKS_SPEC 5.4).
// Chuan hoa PHAI khop ml/features.py. Nhan cong bo = da so 3 lan gan nhat; chua du 5 mau -> AI_CLASS_BOOTING.
#include <Chirale_TensorFlowLite.h>
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include <app_log.h>
#include <protocol.h>
#include "config.h"
#include "model.h"
#include "shared_data.h"
#include "tasks.h"

namespace {
struct Sample3 {
  float t, h, s;
};

alignas(16) uint8_t tensorArena[AI_ARENA_SIZE];
tflite::MicroInterpreter *interp = nullptr;

bool initModel() {
  const tflite::Model *model = tflite::GetModel(g_model);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    appLog("AI", "schema %lu != %d", (unsigned long)model->version(), TFLITE_SCHEMA_VERSION);
    return false;
  }
  static tflite::MicroMutableOpResolver<2> resolver;  // model chi dung FULLY_CONNECTED + SOFTMAX
  resolver.AddFullyConnected();
  resolver.AddSoftmax();
  static tflite::MicroInterpreter staticInterp(model, resolver, tensorArena, sizeof(tensorArena));
  if (staticInterp.AllocateTensors() != kTfLiteOk) {
    appLog("AI", "AllocateTensors FAILED (arena %u B)", (unsigned)sizeof(tensorArena));
    return false;
  }
  interp = &staticInterp;
  appLog("AI", "model %u B %s, arena %u B used %u B", g_model_len, MODEL_IS_INT8 ? "int8" : "float32",
         (unsigned)sizeof(tensorArena), (unsigned)interp->arena_used_bytes());
#if MODEL_IS_DEMO
  appLog("AI", "WARNING: DEMO model (synthetic data) - retrain with real data before reporting");
#endif
  return true;
}

void setInput(int k, float x) {
  TfLiteTensor *in = interp->input(0);
#if MODEL_IS_INT8
  long q = lroundf(x / in->params.scale) + in->params.zero_point;  // scale/zp doc tu tensor, khong hard-code
  in->data.int8[k] = (int8_t)constrain(q, -128L, 127L);
#else
  in->data.f[k] = x;
#endif
}

float outputProb(int c) {
  const TfLiteTensor *out = interp->output(0);
#if MODEL_IS_INT8
  return (out->data.int8[c] - out->params.zero_point) * out->params.scale;
#else
  return out->data.f[c];
#endif
}

// Cu -> moi, giong ml/features.py windows().
void fillInput(const Sample3 *ring, uint8_t head) {
  for (int i = 0; i < AI_WINDOW; i++) {
    const Sample3 &x = ring[(head + i) % AI_WINDOW];
    setInput(i * AI_FEATURES + 0, (x.t - AI_NORM_T_OFFSET) / AI_NORM_T_SCALE);
    setInput(i * AI_FEATURES + 1, (x.h - AI_NORM_H_OFFSET) / AI_NORM_H_SCALE);
    setInput(i * AI_FEATURES + 2, (x.s - AI_NORM_S_OFFSET) / AI_NORM_S_SCALE);
  }
}

uint8_t argmax(float &best) {
  uint8_t idx = 0;
  best = outputProb(0);
  for (int c = 1; c < AI_CLASSES; c++) {
    float v = outputProb(c);
    if (v > best) {
      best = v;
      idx = c;
    }
  }
  return idx;
}

// Da so trong n nhan gan nhat; hoa thi lay nhan moi nhat (tranh nhap nhay).
uint8_t majority(const uint8_t *hist, uint8_t n, uint8_t newest) {
  uint8_t bestCls = newest, bestCnt = 0;
  for (uint8_t i = 0; i < n; i++) {
    uint8_t cnt = 0;
    for (uint8_t j = 0; j < n; j++) cnt += hist[j] == hist[i];
    if (cnt > bestCnt || (cnt == bestCnt && hist[i] == newest)) {
      bestCnt = cnt;
      bestCls = hist[i];
    }
  }
  return bestCnt * 2 > n ? bestCls : newest;
}
}  // namespace

void taskTinyml(void *) {
  if (!initModel()) {
    appLog("AI", "init FAILED, ai_class stays %u", AI_CLASS_BOOTING);
    vTaskDelete(nullptr);
  }
  Sample3 ring[AI_WINDOW] = {};
  uint8_t head = 0, filled = 0;  // head = vi tri mau cu nhat
  uint8_t hist[AI_SMOOTH_N] = {};
  uint8_t histLen = 0, histPos = 0;
  for (;;) {
    if (!sharedWaitNewSample(AI_WAIT_SAMPLE_MS)) {
      appLog("AI", "no new sample in %u ms", AI_WAIT_SAMPLE_MS);
      continue;
    }
    SensorSample s = sharedGetSample();  // da ap sentinel khi DHT20 loi
    ring[(head + filled) % AI_WINDOW] = {s.temperature, s.humidity, (float)s.soil};
    if (filled < AI_WINDOW) filled++;
    else head = (head + 1) % AI_WINDOW;
    if (filled < AI_WINDOW) {
      sharedSetAiClass(AI_CLASS_BOOTING);
      continue;
    }

    fillInput(ring, head);
    uint32_t t0 = micros();
    TfLiteStatus st = interp->Invoke();
    uint32_t dt = micros() - t0;
    if (st != kTfLiteOk) {
      appLog("AI", "Invoke FAILED status=%d", (int)st);
      continue;
    }
    float p = 0;
    uint8_t raw = argmax(p);
    hist[histPos] = raw;
    histPos = (histPos + 1) % AI_SMOOTH_N;
    if (histLen < AI_SMOOTH_N) histLen++;
    uint8_t cls = majority(hist, histLen, raw);
    sharedSetAiClass(cls);
    appLog("AI", "class=%u raw=%u p=%.2f t_us=%lu seq=%lu", cls, raw, p, (unsigned long)dt, (unsigned long)s.seq);
  }
}
