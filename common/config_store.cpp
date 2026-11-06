// common/config_store.cpp
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "config_store.h"
#include "app_log.h"

namespace {
constexpr const char *CONFIG_PATH = "/config.json";
constexpr const char *CONFIG_TMP_PATH = "/config.json.tmp";
constexpr size_t CONFIG_JSON_MAX = 512;

SemaphoreHandle_t cfgMutex = nullptr;   // bao ve 'current' (chi copy struct)
SemaphoreHandle_t fileMutex = nullptr;  // tuan tu hoa ghi flash (cham, khong giu cfgMutex luc ghi)
NetConfig current = {};
ConfigSource source = ConfigSource::Secrets;
volatile bool writing = false;
bool fsReady = false;

struct Guard {
  SemaphoreHandle_t m;
  explicit Guard(SemaphoreHandle_t mm) : m(mm) { xSemaphoreTake(m, portMAX_DELAY); }
  ~Guard() { xSemaphoreGive(m); }
};

bool copyField(char *dst, size_t dstSize, const char *src) {
  if (src == nullptr) src = "";
  size_t n = strlen(src);
  if (n >= dstSize) return false;
  memcpy(dst, src, n + 1);
  return true;
}

bool readFile(const char *path, NetConfig &out) {
  File f = LittleFS.open(path, "r");
  if (!f) return false;
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) {
    appLog("CFG", "%s parse error: %s", path, err.c_str());
    return false;
  }
  NetConfig c = {};
  if (!configFill(c, doc["wifi_ssid"] | "", doc["wifi_pass"] | "", doc["mqtt_host"] | "",
                  doc["mqtt_port"] | 0, doc["device_token"] | "")) {
    appLog("CFG", "%s field too long", path);
    return false;
  }
  const char *reason = configValidate(c);
  if (reason != nullptr) {
    appLog("CFG", "%s invalid: %s", path, reason);
    return false;
  }
  out = c;
  return true;
}

bool writeTmp(const char *json, size_t len) {
  File f = LittleFS.open(CONFIG_TMP_PATH, "w");
  if (!f) return false;
  size_t n = f.write((const uint8_t *)json, len);
  f.close();
  return n == len;
}
}  // namespace

bool configFill(NetConfig &out, const char *ssid, const char *pass, const char *host, uint16_t port, const char *token) {
  out.port = port;
  return copyField(out.ssid, sizeof(out.ssid), ssid) && copyField(out.pass, sizeof(out.pass), pass) &&
         copyField(out.host, sizeof(out.host), host) && copyField(out.token, sizeof(out.token), token);
}

const char *configValidate(const NetConfig &c) {
  if (c.ssid[0] == '\0') return "wifi_ssid empty";
  if (c.host[0] == '\0') return "mqtt_host empty";
  if (c.port == 0) return "mqtt_port out of range";
  if (c.token[0] == '\0') return "device_token empty";
  return nullptr;  // do dai da duoc configFill kiem tra (<= 32/64/63/64)
}

ConfigSource configLoad(const NetConfig &defaults) {
  cfgMutex = xSemaphoreCreateMutex();
  fileMutex = xSemaphoreCreateMutex();
  configASSERT(cfgMutex != nullptr && fileMutex != nullptr);
  current = defaults;
  fsReady = LittleFS.begin(true);  // true: format neu chua co FS (lan nap dau)
  if (!fsReady) {
    appLog("CFG", "LittleFS mount FAILED, using secrets.h");
    return source;
  }
  NetConfig fromFile;
  // .tmp chi con lai khi mat dien dung luc doi ten: van la ban moi nhat hop le.
  bool haveMain = LittleFS.exists(CONFIG_PATH);
  if (readFile(haveMain ? CONFIG_PATH : CONFIG_TMP_PATH, fromFile)) {
    current = fromFile;
    source = ConfigSource::File;
  }
  appLog("CFG", "using %s (ssid='%s' host=%s:%u)", source == ConfigSource::File ? CONFIG_PATH : "secrets.h",
         current.ssid, current.host, current.port);  // khong in mat khau/token
  return source;
}

NetConfig configGet() {
  Guard g(cfgMutex);
  return current;
}

ConfigSource configSource() {
  Guard g(cfgMutex);
  return source;
}

bool configWriting() {
  return writing;
}

bool configSave(const NetConfig &c) {
  if (!fsReady || configValidate(c) != nullptr) return false;
  JsonDocument doc;
  doc["wifi_ssid"] = c.ssid;
  doc["wifi_pass"] = c.pass;
  doc["mqtt_host"] = c.host;
  doc["mqtt_port"] = c.port;
  doc["device_token"] = c.token;
  char json[CONFIG_JSON_MAX];
  size_t len = serializeJson(doc, json, sizeof(json));
  if (len == 0 || len >= sizeof(json) - 1) return false;

  Guard fg(fileMutex);
  writing = true;
  // Ghi tmp roi doi ten: mat dien giua chung thi file cu van con nguyen.
  bool ok = writeTmp(json, len);
  if (ok && !LittleFS.rename(CONFIG_TMP_PATH, CONFIG_PATH)) {
    // Mot so ban VFS khong cho rename de file da co: xoa roi doi ten (load() co fallback .tmp).
    LittleFS.remove(CONFIG_PATH);
    ok = LittleFS.rename(CONFIG_TMP_PATH, CONFIG_PATH);
  }
  writing = false;
  appLog("CFG", "save %s", ok ? "OK" : "FAILED");
  if (!ok) return false;
  Guard g(cfgMutex);
  current = c;
  source = ConfigSource::File;
  return true;
}
