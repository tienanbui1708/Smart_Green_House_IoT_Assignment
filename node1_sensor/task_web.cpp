// node1_sensor/task_web.cpp
// Webserver cuc bo Node 1 (T07, TASKS_SPEC 4.3): trang PROGMEM + REST JSON. Handler chi doc/ghi shared_data roi tra loi.
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
  SensorSample s = sharedGetSample();
  ControlState c = sharedGetControl();
  NetState n = sharedGetNet();
  EspNowStats e = sharedGetEspNow();
  uint8_t cls = sharedGetAiClass();
  JsonDocument doc;
  doc["node"] = 1;
  bool fault = (s.seq != 0 && !s.dhtOk) || cls == AI_CLASS_SENSOR_FAULT;
  if (s.seq != 0 && !fault) {  // giong telemetry: khong dua gia tri hong ra ngoai
    doc[TK_TEMPERATURE] = round1(s.temperature);
    doc[TK_HUMIDITY] = round1(s.humidity);
    doc[TK_SOIL] = s.soil;
  }
  doc[TK_AI_CLASS] = cls;
  doc[TK_SENSOR_FAULT] = fault;
  doc[TK_HEATER] = c.heater;
  doc[TK_AUTO] = c.autoMode;
  doc["espnow_ready"] = e.ready;
  doc["espnow_sent"] = e.sent;
  doc["espnow_acked"] = e.acked;
  doc["espnow_ack_pct"] = e.sent ? (uint32_t)((uint64_t)e.acked * 100 / e.sent) : 0;
  doc["espnow_rtt_us"] = e.lastRttUs;
  doc["espnow_ack_age_s"] = e.lastAckMs ? (millis() - e.lastAckMs) / 1000 : -1;
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

void handleHeater() {
  JsonDocument in;
  if (!webParseBody(server, in)) return;
  JsonVariantConst v = in["value"];
  if (!v.is<float>() || v.as<float>() < 0.0f || v.as<float>() > 255.0f) {
    return webSendError(server, 400, RPC_REASON_BAD_PARAMS);
  }
  if (sharedSetManualHeater((uint8_t)lroundf(v.as<float>())) == CmdResult::RejectedAutoMode) {
    return webSendError(server, 409, RPC_REASON_AUTO_MODE);  // giong RPC (TASKS_SPEC 4.3)
  }
  webSendOk(server);
}

void registerRoutes() {
  webRegisterCommon(server, INDEX_HTML, sizeof(INDEX_HTML) - 1);
  server.on("/api/state", HTTP_GET, handleState);
  server.on("/api/mode", HTTP_POST, handleMode);
  server.on("/api/heater", HTTP_POST, handleHeater);
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
      sharedForceSafe();  // tat suoi truoc khi khoi dong lai
      appLog("WEB", "restarting to apply new config");
      vTaskDelay(pdMS_TO_TICKS(WEB_LOG_FLUSH_MS));
      ESP.restart();
    }
    vTaskDelay(pdMS_TO_TICKS(WEB_LOOP_MS));
  }
}
