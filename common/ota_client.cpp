// common/ota_client.cpp
// Quy trinh: hoi shared attr -> so phien ban -> Update.begin -> hoi tung chunk (chi hoi chunk ke tiep sau khi ghi xong)
// -> bam dan SHA256/MD5 -> DOWNLOADED -> so checksum -> VERIFIED -> Update.end -> UPDATING -> khoi dong lai.
// Loi bat ky: Update.abort(), FAILED + fw_error, giu firmware cu. So lan thu theo fw_version dich luu NVS.
#include <ArduinoJson.h>
#include <Preferences.h>
#include <Update.h>
#include <mbedtls/md5.h>
#include <mbedtls/sha256.h>
#include "ota_client.h"
#include "app_log.h"
#include "config_store.h"
#include "coreiot_link.h"
#include "protocol.h"

namespace {
enum class OtaPhase : uint8_t { Idle, Downloading, Downloaded };
enum class HashAlgo : uint8_t { None, Sha256, Md5 };

struct FwTarget {
  char title[48];
  char version[32];
  uint32_t size;
  char checksum[129];
  char algo[16];
};

constexpr const char *NVS_NS = "ota";
constexpr size_t ERR_MAX = 40;

OtaConfig cfg = {};
FwTarget incoming = {};  // callback gop shared attr moi nhat vao day
FwTarget target = {};    // ban chup luc bat dau tai: attr den giua chung khong doi size/checksum dang dung
volatile bool targetChanged = false;  // callback dat, otaService xu ly
OtaPhase phase = OtaPhase::Idle;
HashAlgo algo = HashAlgo::None;
mbedtls_sha256_context sha;
mbedtls_md5_context md5;

uint32_t requestId = 0;
uint32_t chunkIndex = 0;
uint32_t received = 0;
bool waitingChunk = false;
uint32_t chunkSentMs = 0;
uint8_t chunkTries = 0;
uint32_t lastProgressMs = 0;
uint32_t startMs = 0;
char failReason[ERR_MAX] = "";  // callback/service dat, service publish
char blockedVersion[32] = "";   // da het luot thu: khong bao lai moi lan attr den
char pendingVersion[32] = "";   // NVS: phien ban ky vong sau khi khoi dong lai

bool publishState(const char *state, const char *err = nullptr, int progress = -1) {
  char buf[160];
  int n = snprintf(buf, sizeof(buf), "{\"%s\":\"%s\"", TK_FW_STATE, state);
  if (err != nullptr) n += snprintf(buf + n, sizeof(buf) - n, ",\"%s\":\"%s\"", TK_FW_ERROR, err);
  if (progress >= 0) n += snprintf(buf + n, sizeof(buf) - n, ",\"%s\":%d", TK_FW_PROGRESS, progress);
  snprintf(buf + n, sizeof(buf) - n, "}");
  bool ok = linkPublish(TOPIC_TELEMETRY, buf);
  appLog("OTA", "state %s%s%s%s", state, err ? " err=" : "", err ? err : "", ok ? "" : " (publish FAILED)");
  return ok;
}

uint8_t loadTries(const char *version) {
  Preferences p;
  if (!p.begin(NVS_NS, true)) return 0;
  char tgt[32] = "";
  p.getString("tgt", tgt, sizeof(tgt));
  uint8_t tries = strcmp(tgt, version) == 0 ? p.getUChar("tries", 0) : 0;
  p.end();
  return tries;
}

void saveTries(const char *version, uint8_t tries) {
  Preferences p;
  if (!p.begin(NVS_NS, false)) return;
  p.putString("tgt", version);
  p.putUChar("tries", tries);
  p.end();
}

void savePending(const char *version) {
  Preferences p;
  if (!p.begin(NVS_NS, false)) return;
  p.putString("pending", version);
  p.end();
}

void hashStart() {
  if (algo == HashAlgo::Sha256) {
    mbedtls_sha256_init(&sha);
    mbedtls_sha256_starts(&sha, 0);
  } else {
    mbedtls_md5_init(&md5);
    mbedtls_md5_starts(&md5);
  }
}

void hashUpdate(const uint8_t *d, size_t n) {
  if (algo == HashAlgo::Sha256) mbedtls_sha256_update(&sha, d, n);
  else mbedtls_md5_update(&md5, d, n);
}

void hashFinishHex(char *hex, size_t size) {
  uint8_t dig[32];
  size_t n = 0;
  if (algo == HashAlgo::Sha256) {
    mbedtls_sha256_finish(&sha, dig);
    mbedtls_sha256_free(&sha);
    n = 32;
  } else {
    mbedtls_md5_finish(&md5, dig);
    mbedtls_md5_free(&md5);
    n = 16;
  }
  for (size_t i = 0; i < n && 2 * i + 2 < size; i++) snprintf(hex + 2 * i, 3, "%02x", dig[i]);
}

HashAlgo parseAlgo(const char *a) {
  if (strcasecmp(a, "SHA256") == 0) return HashAlgo::Sha256;
  if (strcasecmp(a, "MD5") == 0) return HashAlgo::Md5;
  return HashAlgo::None;
}

// Huy phien tai (khong publish: co the dang mat MQTT). Ly do se duoc bao khi co ket noi.
void abortDownload(const char *reason) {
  if (phase != OtaPhase::Idle) {
    Update.abort();
    if (algo == HashAlgo::Sha256) mbedtls_sha256_free(&sha);
    else if (algo == HashAlgo::Md5) mbedtls_md5_free(&md5);
  }
  phase = OtaPhase::Idle;
  waitingChunk = false;
  snprintf(failReason, sizeof(failReason), "%s", reason);
  appLog("OTA", "abort: %s (received %lu/%lu)", reason, (unsigned long)received, (unsigned long)target.size);
}

bool requestChunk() {
  char topic[64], body[12];
  snprintf(topic, sizeof(topic), TOPIC_FW_REQUEST_FMT, (unsigned long)requestId, (unsigned long)chunkIndex);
  snprintf(body, sizeof(body), "%lu", (unsigned long)cfg.chunkSize);
  waitingChunk = true;
  chunkSentMs = millis();
  return linkPublish(topic, body);
}

bool needsUpdate() {
  if (target.size == 0 || target.version[0] == '\0') return false;
  // Theo TASKS_SPEC 6.2.4: khac title HOAC version. Goi OTA gan theo device profile nen khong lan sang node khac.
  return strcmp(target.title, cfg.title) != 0 || strcmp(target.version, cfg.version) != 0;
}

void startDownload() {
  if (strcmp(blockedVersion, target.version) == 0) return;  // da bao FAILED cho ban nay
  algo = parseAlgo(target.algo);
  if (algo == HashAlgo::None) {
    snprintf(blockedVersion, sizeof(blockedVersion), "%s", target.version);
    publishState("FAILED", "unsupported checksum");
    return;
  }
  uint8_t tries = loadTries(target.version);
  if (tries >= cfg.maxAttempts) {
    snprintf(blockedVersion, sizeof(blockedVersion), "%s", target.version);
    publishState("FAILED", "too many attempts");
    return;
  }
  saveTries(target.version, tries + 1);
  if (!Update.begin(target.size, U_FLASH)) {
    appLog("OTA", "Update.begin(%lu) failed: %s", (unsigned long)target.size, Update.errorString());
    publishState("FAILED", "update begin");
    return;
  }
  hashStart();
  phase = OtaPhase::Downloading;
  requestId++;
  chunkIndex = 0;
  received = 0;
  chunkTries = 0;
  startMs = millis();
  lastProgressMs = 0;
  appLog("OTA", "start %s %s -> %s, %lu B, %s, attempt %u/%u", target.title, cfg.version, target.version,
         (unsigned long)target.size, target.algo, tries + 1, cfg.maxAttempts);
  publishState("DOWNLOADING", nullptr, 0);
  requestChunk();
}

void finishDownload() {
  appLog("OTA", "downloaded %lu B in %lu ms", (unsigned long)received, (unsigned long)(millis() - startMs));
  publishState("DOWNLOADED");
  char hex[65] = "";
  hashFinishHex(hex, sizeof(hex));
  algo = HashAlgo::None;  // da free context
  if (strcasecmp(hex, target.checksum) != 0) {
    appLog("OTA", "checksum mismatch got=%s want=%s", hex, target.checksum);
    Update.abort();
    phase = OtaPhase::Idle;
    publishState("FAILED", "checksum mismatch");
    return;
  }
  publishState("VERIFIED");
  if (!Update.end(true)) {
    appLog("OTA", "Update.end failed: %s", Update.errorString());
    phase = OtaPhase::Idle;
    publishState("FAILED", "update end");
    return;
  }
  savePending(target.version);
  publishState("UPDATING");
  if (cfg.safeOutputs) cfg.safeOutputs();
  vTaskDelay(pdMS_TO_TICKS(cfg.rebootDelayMs));  // cho tin UPDATING di het; khong dung delay()
  ESP.restart();
}

void serviceDownload() {
  if (phase == OtaPhase::Downloaded) {
    finishDownload();
    return;
  }
  if (!waitingChunk) {
    chunkTries = 0;
    requestChunk();
  } else if (millis() - chunkSentMs >= cfg.chunkTimeoutMs) {
    if (++chunkTries > cfg.chunkRetries) {
      abortDownload("chunk timeout");
      return;
    }
    appLog("OTA", "chunk %lu timeout, retry %u", (unsigned long)chunkIndex, chunkTries);
    requestChunk();
  }
  if (millis() - lastProgressMs >= cfg.progressPeriodMs) {
    lastProgressMs = millis();
    publishState("DOWNLOADING", nullptr, (int)((uint64_t)received * 100 / target.size));
  }
}

void copyStr(char *dst, size_t size, JsonVariantConst v) {
  if (v.isNull()) return;  // thong bao cap nhat chi chua khoa thay doi: giu gia tri cu
  snprintf(dst, size, "%s", v.as<const char *>() ? v.as<const char *>() : "");
}

void handleAttributes(const uint8_t *payload, unsigned int len) {
  JsonDocument doc;
  if (deserializeJson(doc, (const char *)payload, len)) return;
  // Phan hoi request: {"shared":{...}}; thong bao cap nhat: dang phang {...}.
  JsonVariantConst src = doc["shared"].isNull() ? doc.as<JsonVariantConst>() : doc["shared"].as<JsonVariantConst>();
  if (src["fw_version"].isNull() && src["fw_size"].isNull() && doc["deleted"].isNull()) return;
  copyStr(incoming.title, sizeof(incoming.title), src["fw_title"]);
  copyStr(incoming.version, sizeof(incoming.version), src["fw_version"]);
  copyStr(incoming.checksum, sizeof(incoming.checksum), src["fw_checksum"]);
  copyStr(incoming.algo, sizeof(incoming.algo), src["fw_checksum_algorithm"]);
  if (!src["fw_size"].isNull()) incoming.size = src["fw_size"].as<uint32_t>();
  if (!doc["deleted"].isNull()) incoming.size = 0;  // go gan firmware
  targetChanged = true;
}

// Chay trong mqtt.loop(): ghi flash ngay de khong phai copy 4 KB; chi hoi chunk ke tiep o otaService().
void handleChunk(const char *topic, const uint8_t *payload, unsigned int len) {
  unsigned long rid = 0, idx = 0;
  if (sscanf(topic + strlen(TOPIC_FW_RESPONSE_PREFIX), "%lu/chunk/%lu", &rid, &idx) != 2) return;
  if (phase != OtaPhase::Downloading || rid != requestId || idx != chunkIndex) return;  // cu/sai thu tu: bo
  uint32_t remaining = target.size - received;
  if (len == 0 || len > remaining) {
    abortDownload(len == 0 ? "empty chunk" : "chunk too large");
    return;
  }
  size_t written = Update.write(const_cast<uint8_t *>(payload), len);
  if (written != len) {
    abortDownload("flash write");
    return;
  }
  hashUpdate(payload, len);
  received += len;
  chunkIndex++;
  waitingChunk = false;
  if (received == target.size) phase = OtaPhase::Downloaded;
}
}  // namespace

void otaBegin(const OtaConfig &c) {
  cfg = c;
  Preferences p;
  if (p.begin(NVS_NS, true)) {
    p.getString("pending", pendingVersion, sizeof(pendingVersion));
    p.end();
  }
}

void otaOnConnected() {
  char buf[200];
  if (pendingVersion[0] != '\0') {
    bool ok = strcmp(pendingVersion, cfg.version) == 0;
    snprintf(buf, sizeof(buf), "{\"%s\":\"%s\",\"%s\":\"%s\",\"%s\":\"%s\"%s}", TK_CURRENT_FW_TITLE, cfg.title,
             TK_CURRENT_FW_VERSION, cfg.version, TK_FW_STATE, ok ? "UPDATED" : "FAILED",
             ok ? "" : ",\"fw_error\":\"version unchanged\"");
    if (linkPublish(TOPIC_TELEMETRY, buf)) {
      if (!ok) saveTries(pendingVersion, cfg.maxAttempts);  // khong thu lai ban da khoi dong hong
      else saveTries(pendingVersion, 0);
      savePending("");
      appLog("OTA", "after reboot: expected %s running %s -> %s", pendingVersion, cfg.version, ok ? "UPDATED" : "FAILED");
      pendingVersion[0] = '\0';
    }
  } else {
    snprintf(buf, sizeof(buf), "{\"%s\":\"%s\",\"%s\":\"%s\"}", TK_CURRENT_FW_TITLE, cfg.title, TK_CURRENT_FW_VERSION,
             cfg.version);
    linkPublish(TOPIC_TELEMETRY, buf);
  }
  // Chu dong hoi: firmware gan luc thiet bi tat van duoc cap nhat (TASKS_SPEC 6.2.2).
  char topic[64];
  snprintf(topic, sizeof(topic), "%s1", TOPIC_ATTR_REQUEST_PREFIX);
  snprintf(buf, sizeof(buf), "{\"sharedKeys\":\"%s\"}", FW_SHARED_KEYS);
  if (!linkPublish(topic, buf)) appLog("OTA", "attribute request publish FAILED");
}

void otaService() {
  if (!linkMqttUp()) {
    if (phase != OtaPhase::Idle) abortDownload("mqtt disconnected");  // giu firmware cu
    return;
  }
  if (failReason[0] != '\0') {
    publishState("FAILED", failReason);
    failReason[0] = '\0';
  }
  if (phase != OtaPhase::Idle) {
    serviceDownload();
    return;
  }
  if (!targetChanged) return;
  if (configWriting()) return;  // dang ghi /config.json: de lan sau (TASKS_SPEC 6.2.11)
  targetChanged = false;
  target = incoming;
  if (needsUpdate()) startDownload();
}

bool otaBusy() {
  return phase != OtaPhase::Idle;
}

bool otaHandleMessage(const char *topic, const uint8_t *payload, unsigned int len) {
  if (strncmp(topic, TOPIC_FW_RESPONSE_PREFIX, strlen(TOPIC_FW_RESPONSE_PREFIX)) == 0) {
    handleChunk(topic, payload, len);
    return true;
  }
  if (strcmp(topic, TOPIC_ATTRIBUTES) == 0 ||
      strncmp(topic, TOPIC_ATTR_RESPONSE_PREFIX, strlen(TOPIC_ATTR_RESPONSE_PREFIX)) == 0) {
    handleAttributes(payload, len);
    return true;
  }
  return false;
}
