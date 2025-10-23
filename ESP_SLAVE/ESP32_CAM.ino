#include "esp_camera.h"
#include <WiFi.h>
#include <esp_now.h>

#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

// ===== Wi-Fi credentials for web mode =====
const char *ssid = "ESP32_Webcam";
const char *password = "FA123456";

// ===== GPIO pins =====
#define BUZZER_PIN 12
#define BUTTON_PIN 13

// ===== ESP-NOW broadcast MAC (change to your receiver) =====
uint8_t broadcastAddress[] = {0xFC, 0xE8, 0xC0, 0x7B, 0x73, 0x50};

// ===== ESP-NOW structure =====
typedef struct struct_message {
  int slot;
  int type;
  int data_flow;
  int data;
} struct_message;

struct_message dataOut;
esp_now_peer_info_t peerInfo;

// ===== Mode variable =====
// 0 = Color-detection & ESP-NOW send
// 1 = Camera webserver
int mode = 0;

void startCameraServer();
void detectColorCenter();
void beep(int times);
void setupESPNow();
void sendESPNow(int rgbValue);

// ==================================================
// SETUP
// ==================================================
void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println("\nESP32-CAM Booting...");

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // ---- Read mode from button ----
  delay(300);
  mode = (digitalRead(BUTTON_PIN) == LOW) ? 1 : 0;

  // ---- Buzzer indication ----
  beep(mode + 1);  // 1 beep = color mode, 2 beeps = webserver

  // ---- Camera config ----
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = (mode == 0) ? PIXFORMAT_RGB565 : PIXFORMAT_JPEG;
  config.frame_size   = (mode == 0) ? FRAMESIZE_QVGA : FRAMESIZE_UXGA;
  config.fb_count = 1;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

  // ---- Init camera ----
  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera init failed");
    return;
  }
  Serial.println("Camera initialized.");

  if (mode == 0) {
    Serial.println("Mode 0: Color-Detection + ESP-NOW");
    setupESPNow();
    return;
  }

  // ---- Webserver mode ----
  Serial.println("Mode 1: Web Server");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.printf("Access Point '%s' → http://%s (pass:%s)\n",
                ssid, IP.toString().c_str(), password);
  startCameraServer();
}

// ==================================================
// LOOP
// ==================================================
void loop() {
  if (mode == 0) {
    detectColorCenter();
    delay(1000);
  } else {
    delay(10000);
  }
}

// ==================================================
// Beep helper
// ==================================================
void beep(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(150);
    digitalWrite(BUZZER_PIN, LOW);
    delay(150);
  }
}

// ==================================================
// ESP-NOW setup
// ==================================================
void setupESPNow() {
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  dataOut.slot = 6;
  dataOut.type = 3;
  dataOut.data_flow = 0;
  Serial.println("ESP-NOW ready");
}

// ==================================================
// Send via ESP-NOW
// ==================================================
void sendESPNow(int rgbValue) {
  dataOut.data = rgbValue;
  esp_err_t result = esp_now_send(broadcastAddress,
                                  (uint8_t *)&dataOut,
                                  sizeof(dataOut));
  if (result == ESP_OK)
    Serial.println("ESP-NOW send OK");
  else
    Serial.println("ESP-NOW send FAIL");
}

// ==================================================
// Color-Detection Function (fixed 9-digit format)
// ==================================================
void detectColorCenter() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Frame capture failed");
    return;
  }

  uint16_t w = fb->width, h = fb->height;
  uint8_t *buf = fb->buf;
  uint16_t roiW = w / 10, roiH = h / 10;
  uint16_t x0 = (w - roiW) / 2, y0 = (h - roiH) / 2;

  uint32_t rT = 0, gT = 0, bT = 0, n = 0;
  for (int y = y0; y < y0 + roiH; y++) {
    for (int x = x0; x < x0 + roiW; x++) {
      int i = (y * w + x) * 2;
      uint16_t pix = (buf[i + 1] << 8) | buf[i];
      uint8_t r = ((pix >> 11) & 0x1F) << 3;
      uint8_t g = ((pix >> 5) & 0x3F) << 2;
      uint8_t b = (pix & 0x1F) << 3;
      rT += r; gT += g; bT += b; n++;
    }
  }
  esp_camera_fb_return(fb);
  if (n == 0) return;

  int r = rT / n, g = gT / n, b = bT / n;

  // --- Convert to 9-digit fixed string ---
  char rgbStr[10];  // 9 digits + null
  sprintf(rgbStr, "%03d%03d%03d", r, g, b);  // Always 3 digits each
  int rgbInt = atoi(rgbStr);  // Convert back to int if you want numeric send

  Serial.printf("Center RGB: R=%d G=%d B=%d → %s\n", r, g, b, rgbStr);
  sendESPNow(rgbInt);
}
