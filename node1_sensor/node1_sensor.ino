// node1_sensor/node1_sensor.ino
// Node 1 (cam bien): khoi tao va tao task. Logic nam trong task_*.cpp.
#include <Wire.h>
#include <app_log.h>
#include "config.h"
#include "shared_data.h"
#include "tasks.h"

namespace {
struct TaskEntry {
  const char *name;
  TaskFunction_t fn;
  uint32_t stack;
  UBaseType_t prio;
  TaskHandle_t handle;
};

TaskEntry appTasks[] = {
    {"sensor", taskSensor, STACK_SENSOR, PRIO_SENSOR, nullptr},
    {"display", taskDisplay, STACK_DISPLAY, PRIO_DISPLAY, nullptr},
    {"led", taskLed, STACK_LED, PRIO_LED, nullptr},
    {"coreiot", taskCoreiot, STACK_COREIOT, PRIO_COREIOT, nullptr},
    {"espnow", taskEspnow, STACK_ESPNOW, PRIO_ESPNOW, nullptr},
    {"tinyml", taskTinyml, STACK_TINYML, PRIO_TINYML, nullptr},
    {"web", taskWeb, STACK_WEB, PRIO_WEB, nullptr},
};

// Dong STATS dung de ve do thi heap va chot stack (CLAUDE.md 4.1, 4.2).
void taskStats(void *) {
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(STATS_PERIOD_MS));
#if DEBUG_LOG
    char line[192];  // = LOG_LINE_MAX cua app_log; 7 task x ~14 ky tu
    int n = snprintf(line, sizeof(line), "up=%lus heap=%u minheap=%u stack_free:",
                     (unsigned long)(millis() / 1000), (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMinFreeHeap());
    for (const TaskEntry &t : appTasks) {
      if (t.handle == nullptr || n < 0 || n >= (int)sizeof(line)) continue;
      n += snprintf(line + n, sizeof(line) - n, " %s=%u", t.name, (unsigned)uxTaskGetStackHighWaterMark(t.handle));
    }
    appLog("STATS", "%s", line);
#endif
  }
}
}  // namespace

void setup() {
  Serial.begin(115200);
#if ARDUINO_USB_CDC_ON_BOOT
  Serial.setTxTimeoutMs(0);  // khong co may tinh mo cong thi khong de Serial chan task
#endif
  logInit();
  sharedInit();
  coreiotLoadConfig();  // truoc khi tao task: web va coreiot cung doc config_store
  Wire.begin();  // mot lan duy nhat, SDA/SCL mac dinh cua variant
  appLog("MAIN", "%s v%s boot", FW_TITLE, FW_VERSION);

  for (TaskEntry &t : appTasks) {
    if (xTaskCreate(t.fn, t.name, t.stack, nullptr, t.prio, &t.handle) != pdPASS) {
      appLog("MAIN", "create task %s FAILED", t.name);
    }
  }
  xTaskCreate(taskStats, "stats", STACK_STATS, nullptr, PRIO_STATS, nullptr);
}

void loop() {
  vTaskDelay(portMAX_DELAY);
}
