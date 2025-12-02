/*
  ================================================================
         GATEWAY ESP32 CODE (FIXED SSL/TLS for HiveMQ)
  ================================================================
  Fungsi utama:
  1. Menerima data sensor dari Robot via ESP-NOW.
  2. Meneruskan data sensor tersebut ke MQTT Broker (Secure Port 8883).
  3. Menerima perintah dari MQTT Broker.
  4. Meneruskan perintah tersebut ke Robot via ESP-NOW.
*/

#include <esp_now.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h> 

// -- Konfigurasi Jaringan & MQTT --
#define WIFI_SSID "iPhone (4)"
#define WIFI_PASS "chibii26"

// Server HiveMQ 
#define MQTT_SERVER "175c5384e334440fbed6f0490545823f.s1.eu.hivemq.cloud"
#define MQTT_PORT 8883

#define MQTT_USER "kelompok9"      
#define MQTT_PASS "AgriPatrol123!" 

#define MQTT_PUB_TOPIC "robot/data"           // Topik untuk publikasi data sensor
#define MQTT_SUB_TOPIC_CONTROL "robot/control"// Topik untuk subscribe perintah gerak
#define MQTT_SUB_TOPIC_MODE "robot/mode"      // Topik untuk subscribe perintah mode

// -- Konfigurasi ESP-NOW --
// GANTI DENGAN ALAMAT MAC ESP32 ROBOT (Update sesuai device Alvin/Deandro)
uint8_t carAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// -- Deklarasi Objek Global --
// Menggunakan WiFiClientSecure agar bisa connect ke port 8883 (SSL)
WiFiClientSecure espClient; 
PubSubClient client(espClient);
esp_now_peer_info_t peerInfo;

// -- Struktur Data untuk Komunikasi --
typedef struct Message {
  char type;    // 'C' (Command) atau 'S' (Sensor)
  char cmd;     // F, B, L, R, S, P, M
  float temp;
  float hum;
} Message;

// -- Prototipe Fungsi (Updated for ESP32 Core v2.x) --
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len);
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();

// -- Fungsi Setup --
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Gateway ESP32 Starting...");

  // 1. Connect ke WiFi
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_AP_STA); 
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // 2. Inisialisasi ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
  esp_now_register_send_cb(OnDataSent);

  // Daftarkan Robot sebagai peer
  memcpy(peerInfo.peer_addr, carAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer (Mungkin robot belum nyala/MAC salah)");
  }
  Serial.println("ESP-NOW initialized.");

  // 3. Konfigurasi MQTT
  // Set Insecure agar sertifikat SSL tidak perlu diverifikasi manual
  espClient.setInsecure();
  espClient.setTimeout(10); // Tambahan: Timeout lebih lama biar koneksi stabil
  
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);
  Serial.println("MQTT configured.");
}

// -- Loop Utama --
void loop() {
  // Jaga koneksi MQTT
  if (!client.connected()) {
    reconnect();
  }
  // Proses pesan masuk dari MQTT
  client.loop();
}

// -- Fungsi-fungsi Callback --

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  Message msg;
  memcpy(&msg, incomingData, sizeof(msg));

  if (msg.type == 'S') {
    Serial.print("Data from Robot: Temp=");
    Serial.print(msg.temp);
    Serial.print(", Hum=");
    Serial.println(msg.hum);

    StaticJsonDocument<128> doc;
    doc["temperature"] = msg.temp;
    doc["humidity"] = msg.hum;

    char buffer[128];
    serializeJson(doc, buffer);

    client.publish(MQTT_PUB_TOPIC, buffer);
  }
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  // Biarkan kosong agar serial monitor tidak penuh
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("MQTT Msg on [");
  Serial.print(topic);
  Serial.print("]: ");
  
  String messageStr;
  for (int i = 0; i < length; i++) {
    messageStr += (char)payload[i];
  }
  Serial.println(messageStr);

  Message cmd_msg;
  cmd_msg.type = 'C'; 
  cmd_msg.temp = 0;
  cmd_msg.hum = 0;

  if (String(topic) == MQTT_SUB_TOPIC_CONTROL || String(topic) == MQTT_SUB_TOPIC_MODE) {
    if (messageStr.length() > 0) {
      cmd_msg.cmd = messageStr[0]; 
      
      esp_err_t result = esp_now_send(carAddress, (uint8_t *) &cmd_msg, sizeof(cmd_msg));

      if (result == ESP_OK) {
        Serial.println(">> Command forwarded to Robot");
      } else {
        Serial.println(">> Forward failed");
      }
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "GatewayESP32-";
    clientId += String(random(0xffff), HEX);
    
    // Connect menggunakan USER dan PASS yang baru
    if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
      Serial.println("connected");
      client.subscribe(MQTT_SUB_TOPIC_CONTROL);
      client.subscribe(MQTT_SUB_TOPIC_MODE);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}