// common/config_store.h
// Cau hinh mang luu LittleFS /config.json (TASKS_SPEC 4.2). File hop le uu tien hon secrets.h.
// Chi luu chuoi trong struct (khong tra con tro ra ngoai) de doc an toan tu nhieu task.
#pragma once
#include <Arduino.h>

constexpr size_t CFG_SSID_MAX = 32;
constexpr size_t CFG_PASS_MAX = 64;
constexpr size_t CFG_HOST_MAX = 63;
constexpr size_t CFG_TOKEN_MAX = 64;

struct NetConfig {
  char ssid[CFG_SSID_MAX + 1];
  char pass[CFG_PASS_MAX + 1];
  char host[CFG_HOST_MAX + 1];
  uint16_t port;
  char token[CFG_TOKEN_MAX + 1];
};

enum class ConfigSource : uint8_t { Secrets, File };

// Goi mot lan trong setup() truoc khi tao task. defaults = gia tri tu secrets.h.
ConfigSource configLoad(const NetConfig &defaults);
NetConfig configGet();         // ban sao
ConfigSource configSource();
const char *configValidate(const NetConfig &c);  // nullptr = hop le, nguoc lai ly do ngan
bool configSave(const NetConfig &c);             // ghi .tmp roi doi ten; loi -> giu file cu
bool configWriting();          // dang ghi flash (OTA khong duoc bat dau)
bool configFill(NetConfig &out, const char *ssid, const char *pass, const char *host, uint16_t port, const char *token);
