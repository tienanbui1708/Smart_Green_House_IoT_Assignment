// node1_sensor/config.h
// Moi hang so cua Node 1. Khong chua token/mat khau (xem secrets.h).
#pragma once
#include <Arduino.h>

// ---- Firmware ----
#define FW_TITLE "greenhouse-node1"
#define FW_VERSION "1.0.0"
#define DEBUG_LOG 1  // 1: log moi mau cam bien va thong ke task

// ---- Chan: macro cua variant yolo_uno ----
#define PIN_SOIL A1                       // GPIO2
#define PIN_NEOPIXEL RGB_LED              // GPIO45 theo pins_arduino.h cua core
#define NEOPIXEL_COUNT 1                  // TODO(verify): so bong NeoPixel onboard
#define NEOPIXEL_TYPE (NEO_GRB + NEO_KHZ800)  // TODO(verify): thu tu mau tren kit
#define NEOPIXEL_BRIGHTNESS 60

// ---- I2C (DHT20 0x38 co dinh) ----
#define LCD_I2C_ADDR 0x21                 // TODO(verify): theo tools/i2c_scan
#define LCD_COLS 16
#define LCD_ROWS 2
#define I2C_LOCK_TIMEOUT_MS 200

// ---- Do am dat: hieu chinh hai diem (tools/adc_calibrate) ----
#define ADC_BITS 12
#define SOIL_RAW_DRY 0                    // TODO(verify): gia tri tam, chua do
#define SOIL_RAW_WET 4095                 // TODO(verify): gia tri tam, chua do
#define SOIL_SAMPLES 16
#define SOIL_SAMPLE_GAP_MS 2

// ---- DHT20 ----
#define DHT20_MEASURE_MS 100              // datasheet: do xong sau ~80 ms
#define DHT20_T_MIN (-10.0f)              // ngoai dai nay coi la loi (TINYML_GUIDE 2.3)
#define DHT20_T_MAX 80.0f
#define DHT20_H_MIN 0.0f
#define DHT20_H_MAX 100.0f
// SENTINEL_T / SENTINEL_H: common/protocol.h (dung chung Node 2 va TinyML)

// ---- Chu ky ----
#define SENSOR_PERIOD_MS 2000             // phai trung chu ky thu du lieu TinyML
#define DISPLAY_PERIOD_MS 500
#define LED_PERIOD_MS 200
#define STATS_PERIOD_MS 10000

// ---- Luat suoi mo phong (PROJECT.md 8.3) ----
#define HEATER_T_FULL_BELOW 28.0f
#define HEATER_T_OFF_FROM 31.0f
#define HEATER_PWM_FULL 255
#define HEATER_PWM_MID 80
#define HEATER_PWM_OFF 0
#define SAFETY_T_FORCE_AUTO 40.0f         // TODO(verify): chinh theo so do that (PROJECT.md 8.4)

// ---- Task: stack (byte) va uu tien ----
#define STACK_SENSOR 4096
#define STACK_DISPLAY 4096
#define STACK_LED 4096
#define STACK_COREIOT 8192
#define STACK_STATS 3072
#define PRIO_SENSOR 2
#define PRIO_COREIOT 2
#define PRIO_DISPLAY 1
#define PRIO_LED 1
#define PRIO_STATS 1

// ---- Mang (M3) ----
#define MQTT_CLIENT_ID "greenhouse-node1"
#define MQTT_BUFFER_SIZE 512
#define MQTT_KEEPALIVE_S 30
#define MQTT_SOCKET_TIMEOUT_S 5
#define MQTT_LOOP_MS 50
#define MQTT_RETRY_MS 5000
#define TELEMETRY_PERIOD_MS 5000
#define WIFI_ATTEMPT_TIMEOUT_MS 15000
#define WIFI_BACKOFF_LIST {3000, 5000, 10000, 20000, 30000}

// ---- ESP-NOW (M5, T06) ----
// MAC Node 2 lay bang tools/print_mac. Mac dinh broadcast: chay duoc ngay nhung khong co ACK tang MAC
// (callback gui luon bao OK); van do RTT duoc nho goi Ack cua ung dung.
#define NODE2_MAC {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}  // TODO(verify): dien MAC that cua Node 2
// 0 = theo kenh router. Khac 0: WiFi.begin() chi quet kenh nay va giu kenh khi mat router (TASKS_SPEC 3.5).
// Hai node PHAI cung gia tri; dat bang kenh router de ESP-NOW song sot khi router tat.
#define ESPNOW_CHANNEL 0
#define ESPNOW_SEND_PERIOD_MS 2000
#define ESPNOW_RTT_SLOTS 16                  // so goi dang cho Ack duoc nho de tinh RTT
#define ESPNOW_ACK_QUEUE_LEN 8
#define ESPNOW_WAIT_ACK_MS 100               // chu ky cho queue Ack trong task
#define ESPNOW_WIFI_WAIT_MS 100              // cho coreiot_link bat Wi-Fi truoc khi esp_now_init
#define STACK_ESPNOW 4096
#define PRIO_ESPNOW 2

// ---- Webserver + AP cuu ho (M6, T07) ----
#define WEB_PORT 80
#define WEB_LOOP_MS 5                     // handleClient() moi 5 ms; handler ngan
#define WEB_WIFI_WAIT_MS 100
#define AP_SSID "GREENHOUSE-N1-SETUP"      // AP_PASS dat trong secrets.h (>= 8 ky tu)
#define AP_BOOT_TIMEOUT_MS 20000          // khoi dong ma chua vao STA sau 20 s -> bat AP
#define AP_LOST_TIMEOUT_MS 60000          // mat STA qua 60 s -> bat AP
#define AP_IDLE_RETRY_MS 30000            // AP khong co may 30 s -> thu lai STA
#define STACK_WEB 8192
#define PRIO_WEB 1
