// common/web_common.h
// Phan dung chung cua Webserver hai node (T07, TASKS_SPEC 4.3): JSON, loi, /api/config, thong tin he thong.
// Chi goi tu task web cua moi node (WebServer.h dong bo, mot client mot luc).
#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebServer.h>

constexpr size_t WEB_JSON_MAX = 1024;
constexpr uint32_t WEB_RESTART_DELAY_MS = 1500;  // tra loi xong roi moi khoi dong lai
constexpr uint32_t WEB_LOG_FLUSH_MS = 100;       // cho dong log cuoi ra Serial truoc ESP.restart()

void webSendJson(WebServer &server, int code, const JsonDocument &doc);
void webSendOk(WebServer &server);
void webSendError(WebServer &server, int code, const char *reason);  // {"ok":false,"reason":...}
bool webParseBody(WebServer &server, JsonDocument &doc);             // false: da tra 400 bad_params

// GET / (trang PROGMEM), POST /api/config, 404.
void webRegisterCommon(WebServer &server, const char *indexHtml, size_t indexLen);
// Them uptime, heap, rssi, fw_version, mqtt, ssid, ip, mac, ap..., cfg{...}. Khong bao gio co mat khau/token.
void webAddSystemInfo(JsonDocument &doc, const char *fwVersion, bool mqttUp);
bool webRestartDue();  // true khi da qua WEB_RESTART_DELAY_MS sau POST /api/config thanh cong
