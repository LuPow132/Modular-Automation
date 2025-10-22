#define RXD2 16
#define TXD2 17
#include <esp_now.h>
#include <WiFi.h>

// ==== Structure for ESP-NOW ====
typedef struct struct_message {
  int slot;
  int type;
  int data_type;
  int data;
} struct_message;

struct_message dataIn;

// ==== ESP-NOW callback ====
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  memcpy(&dataIn, incomingData, sizeof(dataIn));

  String typeText;
  switch (dataIn.type) {
    case 1: typeText = "IR_distance"; break;
    case 2: typeText = "Ultrasonic"; break;
    case 3: typeText = "Color_sensor"; break;
    default: typeText = "Unknown"; break;
  }
  String flowText = (dataIn.data_type == 0) ? "Input" : "Output";

  String json = "{\"slot\":" + String(dataIn.slot) +
                ",\"type\":\"" + typeText + "\"" +
                ",\"data_type\":\"" + flowText + "\"" +
                ",\"data\":" + String(dataIn.data) + "}";

  String msg = "N>Q " + json;
  msg.trim();
  if (msg.length() > 0) {
    Serial2.println(msg);
    Serial.println(msg);
  }
}

void setup() {
  Serial.begin(115200);                            // for Serial Monitor
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);   // for board-to-board communication
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  // // Send user input to the other ESP
  // if (Serial.available()) {
  //   String msg = "N>Q " + Serial.readStringUntil('\n');
  //   msg.trim();
  //   if (msg.length() > 0) {
  //     Serial2.println(msg);
  //     Serial.println(msg);
  //   }
  // }

  // Receive message from the other ESP
  if (Serial2.available()) {
    String msgFromOther = Serial2.readStringUntil('\n');
    msgFromOther.trim();
    if (msgFromOther.length() > 0) {
      Serial.println(msgFromOther);
    }
  }
}
