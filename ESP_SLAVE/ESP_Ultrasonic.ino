#include <esp_now.h>
#include <WiFi.h>

// ==== Receiver MAC Address ====
uint8_t broadcastAddress[] = {0xFC, 0xE8, 0xC0, 0x7B, 0x73, 0x50};

// ==== Structure for ESP-NOW ====
typedef struct struct_message {
  int slot;
  int type;
  int data_type;
  int data;
} struct_message;

struct_message dataOut;
struct_message dataIn;

// ==== Peer Info ====
esp_now_peer_info_t peerInfo;

// ==== Ultrasonic Pins ====
const int pingPin = 2;  // Trigger
const int inPin   = 3;  // Echo

// ==== Previous Data Storage ====
int lastData = -1;  // store last sent value
const int changeThreshold = 1; // cm tolerance (change >= 1cm to send)

// ==== Convert pulse to cm ====
long microsecondsToCentimeters(long microseconds) {
  return microseconds / 29 / 2;
}

// ==== Callback when data is sent ====
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// ==== ✅ Fixed Callback for new ESP-IDF ====
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  memcpy(&dataIn, incomingData, sizeof(dataIn));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("Slot: "); Serial.print(dataIn.slot);
  Serial.print(" | Type: "); Serial.print(dataIn.type);
  Serial.print(" | Data: "); Serial.println(dataIn.data);
}

// ==== Setup ====
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);  // ✅ correct callback type

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  // ==== Data Setup ====
  dataOut.slot = 3;
  dataOut.type = 2;       // Ultrasonic
  dataOut.data_type = 0;  // cm
  Serial.println("ESP-NOW Sender Ready!");
}

// ==== Loop ====
void loop() {
  long duration, cm;

  pinMode(pingPin, OUTPUT);
  digitalWrite(pingPin, LOW);
  delayMicroseconds(2);
  digitalWrite(pingPin, HIGH);
  delayMicroseconds(5);
  digitalWrite(pingPin, LOW);

  pinMode(inPin, INPUT);
  duration = pulseIn(inPin, HIGH, 30000);  // 30ms timeout
  cm = microsecondsToCentimeters(duration);

  if (cm == 0 || cm > 400) {
    delay(100);
    return;
  }

  Serial.print("Measured: ");
  Serial.print(cm);
  Serial.println(" cm");

  // === Send only when changed significantly ===
  if (abs(cm - lastData) >= changeThreshold) {
    dataOut.data = cm;
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &dataOut, sizeof(dataOut));

    if (result == ESP_OK) {
      Serial.println("Sent with success ✅");
      lastData = cm;
    } else {
      Serial.println("Error sending data ❌");
    }
  } else {
    Serial.println("No significant change, not sending.");
  }

  delay(100);
}
