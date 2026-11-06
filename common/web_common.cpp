// common/web_common.cpp
#include <WiFi.h>
#include "web_common.h"
#include "config_store.h"
#include "coreiot_link.h"
#include "protocol.h"
#include "app_log.h"

namespace {
volatile bool restartPending = false;
uint32_t restartAtMs = 0;

// Truong bo trong / khong gui -> giu gia tri cu (form che mat khau/token nen thuong de trong).
const char *pickField(JsonVariantConst v, const char *current) {
  const char *s = v | "";
  return s[0] != '\0' ? s : current;
}

void handleConfigPost(WebServer &server) {
  JsonDocument in;
  if (!webParseBody(server, in)) return;
  if (!(in["confirm"] | false)) {
    webSendError(server, 400, "confirm_required");
    return;
  }
  NetConfig cur = configGet();
  JsonVariantConst port = in["mqtt_port"];
  long portVal = port.isNull() ? cur.port : port.as<long>();
  if (portVal < 1 || portVal > 65535) {
    webSendError(server, 400, RPC_REASON_BAD_PARAMS);
    return;
  }
  NetConfig next = {};
  bool fits = configFill(next, pickField(in["wifi_ssid"], cur.ssid), pickField(in["wifi_pass"], cur.pass),
                         pickField(in["mqtt_host"], cur.host), (uint16_t)portVal,
                         pickField(in["device_token"], cur.token));
  if (!fits || configValidate(next) != nullptr) {
    webSendError(server, 400, RPC_REASON_BAD_PARAMS);
    return;
  }
  if (!configSave(next)) {
    webSendError(server, 500, "save_failed");
    return;
  }
  webSendOk(server);  // tra loi truoc, khoi dong lai sau (TASKS_SPEC 4.3)
  restartAtMs = millis() + WEB_RESTART_DELAY_MS;
  restartPending = true;
  appLog("WEB", "config saved, restart in %lu ms", (unsigned long)WEB_RESTART_DELAY_MS);
}
}  // namespace

void webSendJson(WebServer &server, int code, const JsonDocument &doc) {
  char buf[WEB_JSON_MAX];
  size_t n = serializeJson(doc, buf, sizeof(buf));
  if (n == 0 || n >= sizeof(buf) - 1) {
    server.send(500, "application/json", "{\"ok\":false,\"reason\":\"too_large\"}");
    return;
  }
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json", buf);
}

void webSendOk(WebServer &server) {
  server.send(200, "application/json", "{\"ok\":true}");
}

void webSendError(WebServer &server, int code, const char *reason) {
  char buf[96];
  snprintf(buf, sizeof(buf), "{\"ok\":false,\"reason\":\"%s\"}", reason);
  server.send(code, "application/json", buf);
}

bool webParseBody(WebServer &server, JsonDocument &doc) {
  // WebServer chi dua body qua arg("plain") (String); chi ton tai trong handler, khong o vong nong.
  if (!server.hasArg("plain") || deserializeJson(doc, server.arg("plain")) || !doc.is<JsonObject>()) {
    webSendError(server, 400, RPC_REASON_BAD_PARAMS);
    return false;
  }
  return true;
}

void webRegisterCommon(WebServer &server, const char *indexHtml, size_t indexLen) {
  server.on("/", HTTP_GET, [&server, indexHtml, indexLen]() {
    server.send_P(200, "text/html; charset=utf-8", indexHtml, indexLen);
  });
  server.on("/api/config", HTTP_POST, [&server]() { handleConfigPost(server); });
  server.onNotFound([&server]() { webSendError(server, 404, "not_found"); });
}

void webAddSystemInfo(JsonDocument &doc, const char *fwVersion, bool mqttUp) {
  doc["uptime"] = millis() / 1000;
  doc["heap"] = ESP.getFreeHeap();
  doc["min_heap"] = ESP.getMinFreeHeap();
  doc["rssi"] = linkRssi();
  doc["fw_version"] = fwVersion;
  doc["mqtt"] = mqttUp;
  doc["wifi"] = linkWifiUp();
  doc["ip"] = WiFi.localIP().toString();  // String ngan, chi trong handler
  doc["mac"] = WiFi.macAddress();
  doc["channel"] = linkChannel();
  doc["ap"] = linkApActive();
  doc["ap_clients"] = linkApClients();
  if (linkApActive()) doc["ap_ip"] = WiFi.softAPIP().toString();

  NetConfig c = configGet();
  JsonObject cfg = doc["cfg"].to<JsonObject>();
  cfg["wifi_ssid"] = c.ssid;
  cfg["mqtt_host"] = c.host;
  cfg["mqtt_port"] = c.port;
  cfg["pass_set"] = c.pass[0] != '\0';
  cfg["token_set"] = c.token[0] != '\0';  // khong tra token/mat khau (TASKS_SPEC 4.3)
  cfg["source"] = configSource() == ConfigSource::File ? "config.json" : "secrets.h";
}

bool webRestartDue() {
  return restartPending && (int32_t)(millis() - restartAtMs) >= 0;
}
