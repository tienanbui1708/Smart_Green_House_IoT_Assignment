// common/protocol.h
// Nguon su that: PROJECT.md muc 7. Sua o day phai sua cung luc ca hai node, dashboard, RuleChain va PROJECT.md.
#pragma once
#include <stdint.h>

constexpr unsigned PROTOCOL_VERSION = 1;

// ---- Sentinel khi DHT20 loi (TINYML_GUIDE 2.3); dung chung goi ESP-NOW va TinyML ----
constexpr float SENTINEL_T = -40.0f;
constexpr float SENTINEL_H = 0.0f;

// ---- Nhan TinyML (PROJECT.md 8.1) ----
constexpr uint8_t AI_CLASS_NORMAL = 0;
constexpr uint8_t AI_CLASS_SOIL_DRY = 1;
constexpr uint8_t AI_CLASS_HOT_HUMID = 2;
constexpr uint8_t AI_CLASS_SENSOR_FAULT = 3;
constexpr uint8_t AI_CLASS_BOOTING = 255;  // chua du cua so / chua co AI: KHONG coi la binh thuong

// ---- Goi ESP-NOW (PROJECT.md 7.1). Hai node phai bien dich cung file nay. ----
#pragma pack(push, 1)
struct EspNowSensorPkt {  // Node 1 -> Node 2
  uint8_t ver;            // PROTOCOL_VERSION
  uint32_t seq;           // tang dan, bat dau tu 1 sau moi lan Node 1 khoi dong
  float temperature;      // °C hoac SENTINEL_T
  float humidity;         // % hoac SENTINEL_H
  uint8_t soil;           // % 0..100
  uint8_t ai_class;       // 0..3; AI_CLASS_BOOTING
  uint8_t heater;         // 0..255
  uint32_t tx_ms;         // millis() Node 1 (chi de log, khong tinh tre mot chieu)
};
struct EspNowAckPkt {     // Node 2 -> Node 1
  uint8_t ver;
  uint32_t seq;           // seq cua goi duoc ack
  uint32_t rx_ms;         // millis() Node 2 (dong ho khac Node 1: chi de log)
};
#pragma pack(pop)
static_assert(sizeof(EspNowSensorPkt) == 20, "EspNowSensorPkt doi kich thuoc: kiem tra ca hai node");
static_assert(sizeof(EspNowAckPkt) == 9, "EspNowAckPkt doi kich thuoc: kiem tra ca hai node");

// ---- MQTT topics (ThingsBoard/CoreIOT device API) ----
constexpr const char *TOPIC_TELEMETRY = "v1/devices/me/telemetry";
constexpr const char *TOPIC_RPC_REQUEST_SUB = "v1/devices/me/rpc/request/+";
constexpr const char *TOPIC_RPC_REQUEST_PREFIX = "v1/devices/me/rpc/request/";
constexpr const char *TOPIC_RPC_RESPONSE_PREFIX = "v1/devices/me/rpc/response/";

// ---- OTA (ThingsBoard device API, PROJECT.md 7.2 / COREIOT_SETUP.md 10.5) ----
constexpr const char *TOPIC_ATTRIBUTES = "v1/devices/me/attributes";                    // shared attr cap nhat
constexpr const char *TOPIC_ATTR_RESPONSE_SUB = "v1/devices/me/attributes/response/+";
constexpr const char *TOPIC_ATTR_RESPONSE_PREFIX = "v1/devices/me/attributes/response/";
constexpr const char *TOPIC_ATTR_REQUEST_PREFIX = "v1/devices/me/attributes/request/";
constexpr const char *TOPIC_FW_RESPONSE_SUB = "v2/fw/response/+/chunk/+";
constexpr const char *TOPIC_FW_RESPONSE_PREFIX = "v2/fw/response/";                    // <reqId>/chunk/<i>
constexpr const char *TOPIC_FW_REQUEST_FMT = "v2/fw/request/%lu/chunk/%lu";            // payload: chunk size (thap phan)
constexpr const char *FW_SHARED_KEYS = "fw_title,fw_version,fw_size,fw_checksum,fw_checksum_algorithm";
constexpr const char *TK_CURRENT_FW_TITLE = "current_fw_title";
constexpr const char *TK_CURRENT_FW_VERSION = "current_fw_version";
constexpr const char *TK_FW_STATE = "fw_state";       // DOWNLOADING DOWNLOADED VERIFIED UPDATING UPDATED FAILED
constexpr const char *TK_FW_ERROR = "fw_error";
constexpr const char *TK_FW_PROGRESS = "fw_progress";  // % khi DOWNLOADING

// ---- RPC methods (muc 7.3) ----
constexpr const char *RPC_SET_MODE = "setMode";        // params: true=Auto, false=Manual
constexpr const char *RPC_SET_HEATER = "setHeater";    // Node 1, params 0..255, chi Manual
constexpr const char *RPC_SET_FAN = "setFan";          // Node 2, params bool, chi Manual
constexpr const char *RPC_SET_PUMP = "setPump";        // Node 2, params bool, chi Manual
constexpr const char *RPC_CLOUD_ALERT = "cloudAlert";  // Node 2, moi che do; params {"level":0..2,"msg":"<=16"}

// ---- cloudAlert ----
constexpr unsigned CLOUD_ALERT_LEVEL_MAX = 2;  // 0 xoa, 1 canh bao (vang), 2 nghiem trong (do)
constexpr unsigned CLOUD_ALERT_MSG_MAX = 16;   // vua mot dong LCD 16x2

// ---- Ly do tu choi RPC ----
constexpr const char *RPC_REASON_AUTO_MODE = "auto_mode";
constexpr const char *RPC_REASON_UNKNOWN_METHOD = "unknown_method";
constexpr const char *RPC_REASON_BAD_PARAMS = "bad_params";

// ---- Khoa telemetry (muc 7.2) ----
constexpr const char *TK_TEMPERATURE = "temperature";
constexpr const char *TK_HUMIDITY = "humidity";
constexpr const char *TK_SOIL = "soil";
constexpr const char *TK_AI_CLASS = "ai_class";
constexpr const char *TK_SENSOR_FAULT = "sensor_fault";
constexpr const char *TK_HEATER = "heater";
constexpr const char *TK_FAN = "fan";
constexpr const char *TK_PUMP = "pump";
constexpr const char *TK_AUTO = "auto";
constexpr const char *TK_RSSI = "rssi";
constexpr const char *TK_HEAP = "heap";
constexpr const char *TK_UPTIME = "uptime";
constexpr const char *TK_FW_VERSION = "fw_version";
constexpr const char *TK_ESPNOW_AGE_S = "espnow_age_s";
constexpr const char *TK_ALERT_LEVEL = "alert_level";  // Node 2: muc cloudAlert dang hien (0 = khong)
