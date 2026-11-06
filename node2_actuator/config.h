// node2_actuator/config.h
// Moi hang so cua Node 2. Khong chua token/mat khau (xem secrets.h).
#pragma once
#include <Arduino.h>

// ---- Firmware ----
#define FW_TITLE "greenhouse-node2"
#define FW_VERSION "1.0.0"
#define DEBUG_LOG 1

// 1: bo qua ESP-NOW, dung song tam giac gia de thu hien thi/luat (chi khi khong co Node 1).
#define USE_FAKE_SENSOR_DATA 0

// ---- Chan: macro cua variant yolo_uno ----
#define PIN_FAN_LED D13                   // LED onboard = quat mo phong
#define PIN_NEOPIXEL RGB_LED              // GPIO45 theo pins_arduino.h cua core
#define NEOPIXEL_COUNT 1                  // TODO(verify)
#define NEOPIXEL_TYPE (NEO_GRB + NEO_KHZ800)  // TODO(verify)
#define NEOPIXEL_BRIGHTNESS 60

// ---- I2C ----
#define LCD_I2C_ADDR 0x21                 // TODO(verify): theo tools/i2c_scan
#define LCD_COLS 16
#define LCD_ROWS 2

// ---- Chu ky ----
#define CONTROL_PERIOD_MS 500
#define FAKE_DATA_PERIOD_MS 2000          // cung chu ky Node 1 gui
#define DISPLAY_PERIOD_MS 500
#define ACTUATOR_PERIOD_MS 50             // du min de chop canh bao 250 ms
#define STATS_PERIOD_MS 10000
#define ESPNOW_STALE_MS 10000             // PROJECT.md 8.2: > 10 s la NO DATA -> safe mode

// ---- Luat dieu khien (PROJECT.md 8.3) ----
#define FAN_ON_T_ABOVE 32.0f
#define FAN_ON_H_ABOVE 80.0f
#define PUMP_ON_SOIL_BELOW 20
#define PUMP_OFF_SOIL_ABOVE 60
#define NEO_H_BLUE_BELOW 50.0f
#define NEO_H_RED_ABOVE 75.0f
#define NEO_NO_DATA_RGB 120, 60, 160      // tim nhat co dinh khi mat/loi du lieu (khong chop: khac cloudAlert)

// ---- cloudAlert (M4, PROJECT.md 7.3) ----
#define ALERT_DURATION_MS 10000           // msg hien tren LCD 10 s, nhan lai thi gia han
#define ALERT_BLINK_L1_MS 500             // chu ky chop vang (sang nua chu ky)
#define ALERT_BLINK_L2_MS 250             // chu ky chop do
#define ALERT_L1_RGB 255, 180, 0
#define ALERT_L2_RGB 255, 0, 0
// 1: taskControl tu kich hoat level 1 (5 s) -> level 2 (5 s) -> 0 de thu LCD/NeoPixel khong can Cloud.
#define SELF_TEST_ALERT 0
#define SELF_TEST_START_MS 3000
#define SELF_TEST_STEP_MS 5000

// ---- Task ----
#define STACK_CONTROL 4096
#define STACK_DISPLAY 4096
#define STACK_ACTUATOR 4096
#define STACK_COREIOT 8192
#define STACK_STATS 3072
#define PRIO_CONTROL 3
#define PRIO_COREIOT 2
#define PRIO_DISPLAY 1
#define PRIO_ACTUATOR 1
#define PRIO_STATS 1

// ---- Mang (M3) ----
#define MQTT_CLIENT_ID "greenhouse-node2"
#define MQTT_BUFFER_SIZE 512
#define MQTT_KEEPALIVE_S 30
#define MQTT_SOCKET_TIMEOUT_S 5
#define MQTT_LOOP_MS 50
#define MQTT_RETRY_MS 5000
#define TELEMETRY_PERIOD_MS 5000
#define WIFI_ATTEMPT_TIMEOUT_MS 15000
#define WIFI_BACKOFF_LIST {3000, 5000, 10000, 20000, 30000}

// ---- ESP-NOW (M5, T06) ----
// Bo comment va dien MAC Node 1 (tools/print_mac) de chi nhan goi tu Node 1.
// #define NODE1_MAC {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}  // TODO(verify)
#define ESPNOW_CHANNEL 0                  // PHAI giong Node 1 (xem node1_sensor/config.h)
#define ESPNOW_RX_QUEUE_LEN 8
#define ESPNOW_RX_WAIT_MS 500
#define ESPNOW_WIFI_WAIT_MS 100
#define ESPNOW_AGE_UNKNOWN_S 999          // espnow_age_s khi chua nhan goi nao
#define STACK_ESPNOW 4096
#define PRIO_ESPNOW 3

// ---- Webserver + AP cuu ho (M6, T07) ----
#define WEB_PORT 80
#define WEB_LOOP_MS 5                     // handleClient() moi 5 ms; handler ngan
#define WEB_WIFI_WAIT_MS 100
#define AP_SSID "GREENHOUSE-N2-SETUP"      // AP_PASS dat trong secrets.h (>= 8 ky tu)
#define AP_BOOT_TIMEOUT_MS 20000          // khoi dong ma chua vao STA sau 20 s -> bat AP
#define AP_LOST_TIMEOUT_MS 60000          // mat STA qua 60 s -> bat AP
#define AP_IDLE_RETRY_MS 30000            // AP khong co may 30 s -> thu lai STA
#define STACK_WEB 8192
#define PRIO_WEB 1
