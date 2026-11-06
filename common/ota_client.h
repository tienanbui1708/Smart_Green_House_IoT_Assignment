// common/ota_client.h
// OTA firmware qua CoreIOT/ThingsBoard MQTT (T14, TASKS_SPEC 6.2, COREIOT_SETUP.md 10.5).
// Chay TRONG task coreiot: PubSubClient khong thread-safe nen moi publish/loop phai cung mot task.
// Callback MQTT chi ghi flash + bam dan; moi publish duoc hoan sang otaService().
#pragma once
#include <Arduino.h>

struct OtaConfig {
  const char *title;        // FW_TITLE
  const char *version;      // FW_VERSION
  uint32_t chunkSize;       // OTA_CHUNK_SIZE (MQTT buffer phai >= chunk + 256)
  uint32_t chunkTimeoutMs;  // cho moi chunk
  uint8_t chunkRetries;     // so lan hoi lai mot chunk truoc khi bo
  uint8_t maxAttempts;      // so lan thu toi da cho mot fw_version dich (luu NVS)
  uint32_t progressPeriodMs;
  uint32_t rebootDelayMs;
  void (*safeOutputs)();    // dat dau ra an toan truoc khi khoi dong lai
};

void otaBegin(const OtaConfig &cfg);   // goi mot lan truoc linkBegin (doc NVS)
void otaOnConnected();                 // moi lan MQTT vua ket noi: bao phien ban + hoi shared attributes
void otaService();                     // moi vong task coreiot: timeout, hoi chunk, publish trang thai
bool otaBusy();                        // dang tai/ghi flash
// Tra true neu da xu ly topic (attributes / fw chunk). Goi tu callback MQTT.
bool otaHandleMessage(const char *topic, const uint8_t *payload, unsigned int len);
