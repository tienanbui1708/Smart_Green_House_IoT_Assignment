// node2_actuator/task_web.cpp
// Webserver cuc bo Node 2 (T07, TASKS_SPEC 4.3): trang PROGMEM + REST JSON. Handler chi doc/ghi shared_data roi tra loi.
#include <WiFi.h>
#include <WebServer.h>
#include <app_log.h>
#include <protocol.h>
#include <web_common.h>
#include "config.h"
#include "shared_data.h"
#include "tasks.h"
#include "web_assets.h"

namespace {
WebServer server(WEB_PORT);

float round1(float v) {
  return roundf(v * 10.0f) / 10.0f;
}

void handleState() {
  SensorView v = sharedGetSensor();
  ActuatorState a = sharedGetActuators();
  NetState n = sharedGetNet();
  CloudAlertState al = sharedGetAlert();
  JsonDocument doc;
  doc["node"] = 2;
  if (!a.dataStale && !a.dataFault) {  // du lieu cu/hong: khong hien so
    doc[TK_TEMPERATURE] = round1(v.temperature);
    doc[TK_HUMIDITY] = round1(v.humidity);
    doc[TK_SOIL] = v.soil;
  }
  doc[TK_AI_CLASS] = v.aiClass;
  doc[TK_FAN] = a.fan;
  doc[TK_PUMP] = a.pump;
  doc[TK_AUTO] = a.autoMode;
  doc["data_stale"] = a.dataStale;
  doc["data_fault"] = a.dataFault;
  doc[TK_ESPNOW_AGE_S] = v.seq == 0 ? (uint32_t)ESPNOW_AGE_UNKNOWN_S : (millis() - v.receivedMs) / 1000;
  doc["espnow_seq"] = v.seq;
  doc[TK_ALERT_LEVEL] = al.level;
  doc["alert_msg"] = al.level ? al.msg : "";
  webAddSystemInfo(doc, FW_VERSION, n.mqtt);
  webSendJson(server, 200, doc);
}

void handleMode() {
  JsonDocument in;
  if (!webParseBody(server, in)) return;
  if (!in["auto"].is<bool>()) return webSendError(server, 400, RPC_REASON_BAD_PARAMS);
  sharedSetAutoMode(in["auto"].as<bool>());
  appLog("WEB", "mode -> %s", in["auto"].as<bool>() ? "auto" : "manual");
  webSendOk(server);
}

void handleSwitch(CmdResult (*setter)(bool)) {
  JsonDocument in;
  if (!webParseBody(server, in)) return;
  if (!in["on"].is<bool>()) return webSendError(server, 400, RPC_REASON_BAD_PARAMS);
  if (setter(in["on"].as<bool>()) == CmdResult::RejectedAutoMode) {
    return webSendError(server, 409, RPC_REASON_AUTO_MODE);  // giong RPC (TASKS_SPEC 4.3)
  }
  webSendOk(server);
}

void registerRoutes() {
  webRegisterCommon(server, INDEX_HTML, sizeof(INDEX_HTML) - 1);
  server.on("/api/state", HTTP_GET, handleState);
  server.on("/api/mode", HTTP_POST, handleMode);
  server.on("/api/fan", HTTP_POST, []() { handleSwitch(sharedSetManualFan); });
  server.on("/api/pump", HTTP_POST, []() { handleSwitch(sharedSetManualPump); });
}

// Dua chap hanh ve an toan truoc khi khoi dong lai (ep Auto, du lieu cu -> taskControl tat).
void safeBeforeRestart() {
  sharedSetAutoMode(true);
  sharedApplyAuto(false, false, true, false);
}
}  // namespace

void taskWeb(void *) {
  // WebServer can lwIP da khoi dong: cho coreiot_link bat Wi-Fi.
  while (WiFi.getMode() == WIFI_MODE_NULL) vTaskDelay(pdMS_TO_TICKS(WEB_WIFI_WAIT_MS));
  registerRoutes();
  server.begin();
  appLog("WEB", "server on port %u", WEB_PORT);
  for (;;) {
    server.handleClient();
    if (webRestartDue()) {
      safeBeforeRestart();
      appLog("WEB", "restarting to apply new config");
      vTaskDelay(pdMS_TO_TICKS(WEB_LOG_FLUSH_MS));
      ESP.restart();
    }
    vTaskDelay(pdMS_TO_TICKS(WEB_LOOP_MS));
  }
}
