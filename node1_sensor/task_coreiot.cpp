// node1_sensor/task_coreiot.cpp
// Telemetry Node 1 + RPC setMode / setHeater (PROJECT.md 7.2, 7.3).
#if !__has_include("secrets.h")
#error "Thieu node1_sensor/secrets.h: chep tu secrets.h.example roi dien WiFi/token"
#endif
#include <ArduinoJson.h>
#include <app_log.h>
#include <coreiot_link.h>
#include <protocol.h>
#include "config.h"
#include "secrets.h"
#include "shared_data.h"
#include "tasks.h"

namespace {
constexpr uint32_t WIFI_BACKOFF_MS[] = WIFI_BACKOFF_LIST;
constexpr size_t TELEMETRY_JSON_MAX = MQTT_BUFFER_SIZE - 64;  // chua cho header + topic

float round1(float v) {
  return roundf(v * 10.0f) / 10.0f;
}

bool publishTelemetry() {
  SensorSample s = sharedGetSample();
  ControlState c = sharedGetControl();
  JsonDocument doc;
  if (s.seq != 0) {
    // DHT20 loi thi khong gui gia tri sentinel len Cloud (tranh bao dong gia).
    if (s.dhtOk) {
      doc[TK_TEMPERATURE] = round1(s.temperature);
      doc[TK_HUMIDITY] = round1(s.humidity);
    }
    doc[TK_SOIL] = s.soil;
  }
  // TODO(M7): them ai_class va sensor_fault khi co task_tinyml.
  doc[TK_HEATER] = c.heater;
  doc[TK_AUTO] = c.autoMode;
  doc[TK_RSSI] = linkRssi();
  doc[TK_HEAP] = ESP.getFreeHeap();
  doc[TK_UPTIME] = millis() / 1000;
  doc[TK_FW_VERSION] = FW_VERSION;

  char json[TELEMETRY_JSON_MAX];
  size_t n = serializeJson(doc, json, sizeof(json));
  if (n == 0 || n >= sizeof(json) - 1) {
    appLog("MQTT", "telemetry too large (%u bytes)", (unsigned)n);
    return false;
  }
  bool ok = linkPublish(TOPIC_TELEMETRY, json);
#if DEBUG_LOG
  appLog("MQTT", "telemetry %s: %s", ok ? "sent" : "FAILED", json);
#else
  if (!ok) appLog("MQTT", "telemetry publish FAILED");
#endif
  return ok;
}

const char *rpcSetMode(JsonVariantConst params) {
  if (!params.is<bool>()) return RPC_REASON_BAD_PARAMS;
  sharedSetAutoMode(params.as<bool>());
  return nullptr;
}

const char *rpcSetHeater(JsonVariantConst params) {
  if (!params.is<float>()) return RPC_REASON_BAD_PARAMS;  // knob/slider co the gui so thuc
  float v = params.as<float>();
  if (v < 0.0f || v > 255.0f) return RPC_REASON_BAD_PARAMS;
  if (sharedSetManualHeater((uint8_t)lroundf(v)) == CmdResult::RejectedAutoMode) return RPC_REASON_AUTO_MODE;
  return nullptr;
}

void handleRpc(const RpcRequest &req) {
  JsonDocument in;
  const char *reason = nullptr;
  const char *method = "";
  if (deserializeJson(in, req.payload)) {
    reason = RPC_REASON_BAD_PARAMS;
  } else {
    method = in["method"] | "";
    JsonVariantConst params = in["params"];
    if (strcmp(method, RPC_SET_MODE) == 0) reason = rpcSetMode(params);
    else if (strcmp(method, RPC_SET_HEATER) == 0) reason = rpcSetHeater(params);
    else reason = RPC_REASON_UNKNOWN_METHOD;
  }
  bool sent = linkRespondRpc(req.id, reason);
  appLog("RPC", "id=%s method=%s -> %s%s", req.id, method, reason ? reason : "ok", sent ? "" : " (response FAILED)");
}
}  // namespace

void taskCoreiot(void *) {
  const LinkConfig cfg = {
      WIFI_SSID, WIFI_PASS, MQTT_HOST, MQTT_PORT, DEVICE_TOKEN, MQTT_CLIENT_ID,
      MQTT_BUFFER_SIZE, MQTT_KEEPALIVE_S, MQTT_SOCKET_TIMEOUT_S, MQTT_RETRY_MS,
      WIFI_ATTEMPT_TIMEOUT_MS, WIFI_BACKOFF_MS, sizeof(WIFI_BACKOFF_MS) / sizeof(WIFI_BACKOFF_MS[0])};
  if (!linkBegin(cfg)) {
    appLog("MQTT", "init FAILED (out of memory), task stopped");
    vTaskDelete(nullptr);
  }

  uint32_t lastTelemetryMs = 0;
  for (;;) {
    linkService();
    sharedSetNet({linkWifiUp(), linkMqttUp(), linkRssi()});

    RpcRequest req;
    while (linkNextRpc(req)) handleRpc(req);

    bool fresh = linkTakeJustConnected();
    if (linkMqttUp() && (fresh || millis() - lastTelemetryMs >= TELEMETRY_PERIOD_MS)) {
      publishTelemetry();
      lastTelemetryMs = millis();
    }
    vTaskDelay(pdMS_TO_TICKS(MQTT_LOOP_MS));
  }
}
