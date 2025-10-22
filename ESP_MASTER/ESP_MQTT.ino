#define RXD2 16
#define TXD2 17
#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "FIBO_Academy";
const char* password = "fiboacademy2568";
const char* mqtt_server = "10.61.24.32";
const int   mqtt_port   = 1883;
const char* topic_pub   = "fibo/sensor/data";
const char* topic_sub   = "fibo/sensor/cmd";

WiFiClient espClient;
PubSubClient client(espClient);

void mqttReconnect() {
  while (!client.connected()) {
    Serial.print("Connecting MQTT...");
    if (client.connect("ESP32_MQTT_Bridge")) {
      Serial.println("connected");
      client.subscribe(topic_sub);
    } else {
      Serial.print("failed rc=");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i=0; i<length; i++) msg += (char)payload[i];
  msg.trim();

  msg = "Q>N " + msg;
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
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.print("\nWi-Fi connected: ");
  Serial.p
rintln(WiFi.localIP());

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);
}

void loop() {
  if (!client.connected()) mqttReconnect();
  client.loop();
  // Receive message from the other ESP
  // if (Serial2.available()) {
  //   String msgFromOther = Serial2.readStringUntil('\n');
  //   msgFromOther.trim();
  //   if (msgFromOther.length() > 0) {
  //     Serial.println(msgFromOther);
  //   }
  // }
}
