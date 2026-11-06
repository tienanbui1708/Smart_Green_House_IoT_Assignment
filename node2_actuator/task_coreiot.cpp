// node2_actuator/task_coreiot.cpp
// Telemetry Node 2 + RPC setMode / setFan / setPump (PROJECT.md 7.2, 7.3).
#if !__has_include("secrets.h")
#error "Thieu node2_actuator/secrets.h: chep tu secrets.h.example roi dien WiFi/token"
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
constexpr size_t TELEMETRY_JSON_MAX = MQTT_BUFFER_SIZE - 64;

bool publishTelemetry() {
  ActuatorState a = sharedGetActuators();
  JsonDocument doc;
  doc[TK_FAN] = a.fan ? 1 : 0;
  doc[TK_PUMP] = a.pump ? 1 : 0;
  doc[TK_AUTO] = a.autoMode;
  doc[TK_RSSI] = linkRssi();
  doc[TK_HEAP] = ESP.getFreeHeap();
  doc[TK_UPTIME] = millis() / 1000;
  doc[TK_FW_VERSION] = FW_VERSION;
  doc[TK_ALERT_LEVEL] = sharedGetAlert().level;
  // TODO(M5): them espnow_age_s khi co du lieu ESP-NOW that (du lieu gia khong co y nghia).

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

const char *rpcSetSwitch(JsonVariantConst params, CmdResult (*setter)(bool)) {
  if (!params.is<bool>()) return RPC_REASON_BAD_PARAMS;
  if (setter(params.as<bool>()) == CmdResult::RejectedAutoMode) return RPC_REASON_AUTO_MODE;
  return nullptr;
}

// Moi che do; chi hien thi, khong doi fan/pump/auto (COREIOT_SETUP.md 9.2).
const char *rpcCloudAlert(JsonVariantConst params) {
  if (!params.is<JsonObjectConst>()) return RPC_REASON_BAD_PARAMS;
  JsonVariantConst level = params["level"];
  if (!level.is<int>()) return RPC_REASON_BAD_PARAMS;
  int lv = level.as<int>();
  if (lv < 0 || lv > (int)CLOUD_ALERT_LEVEL_MAX) return RPC_REASON_BAD_PARAMS;
  const char *msg = params["msg"] | "";
  sharedSetAlert((uint8_t)lv, msg, ALERT_DURATION_MS);
  appLog("RPC", "cloudAlert level=%d msg='%.16s'", lv, msg);
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
    else if (strcmp(method, RPC_SET_FAN) == 0) reason = rpcSetSwitch(params, sharedSetManualFan);
    else if (strcmp(method, RPC_SET_PUMP) == 0) reason = rpcSetSwitch(params, sharedSetManualPump);
    else if (strcmp(method, RPC_CLOUD_ALERT) == 0) reason = rpcCloudAlert(params);
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
