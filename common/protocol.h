// common/protocol.h
// Nguon su that: PROJECT.md muc 7. Sua o day phai sua cung luc ca hai node, dashboard, RuleChain va PROJECT.md.
#pragma once

constexpr unsigned PROTOCOL_VERSION = 1;

// ---- MQTT topics (ThingsBoard/CoreIOT device API) ----
constexpr const char *TOPIC_TELEMETRY = "v1/devices/me/telemetry";
constexpr const char *TOPIC_RPC_REQUEST_SUB = "v1/devices/me/rpc/request/+";
constexpr const char *TOPIC_RPC_REQUEST_PREFIX = "v1/devices/me/rpc/request/";
constexpr const char *TOPIC_RPC_RESPONSE_PREFIX = "v1/devices/me/rpc/response/";

// ---- RPC methods (muc 7.3) ----
constexpr const char *RPC_SET_MODE = "setMode";        // params: true=Auto, false=Manual
constexpr const char *RPC_SET_HEATER = "setHeater";    // Node 1, params 0..255, chi Manual
constexpr const char *RPC_SET_FAN = "setFan";          // Node 2, params bool, chi Manual
constexpr const char *RPC_SET_PUMP = "setPump";        // Node 2, params bool, chi Manual
constexpr const char *RPC_CLOUD_ALERT = "cloudAlert";  // Node 2, lam o M4

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
