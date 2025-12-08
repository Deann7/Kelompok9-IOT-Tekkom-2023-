#include <esp_now.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

// -- LED Indicator (Built-in ESP32s) --
#define LED_BUILTIN 2

// -- Konfigurasi Jaringan & MQTT --
#define WIFI_SSID "Galaxy A55 5G D016"
#define WIFI_PASS "alvin2005"

// Server HiveMQ
#define MQTT_SERVER "89288a44b913422cb31b59472927642a.s1.eu.hivemq.cloud"
#define MQTT_PORT 8883

#define MQTT_USER "kelompok9_terbaru123"
#define MQTT_PASS "AgriPatro123l123!"

#define MQTT_PUB_TOPIC "robot/data"           
#define MQTT_SUB_TOPIC_CONTROL "robot/control"// Topik untuk subscribe perintah gerak
#define MQTT_SUB_TOPIC_MODE "robot/mode"      // Topik untuk subscribe perintah mode

// -- Konfigurasi ESP-NOW --
// GANTI DENGAN ALAMAT MAC ESP32 ROBOT (Update sesuai device Alvin/Deandro)
uint8_t carAddress[] = {0x4C, 0xC3, 0x82, 0xBF, 0x9A, 0x50};

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

// -- Prototipe Fungsi (Updated for ESP32 Core v3.x) --
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len);
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();

// -- Fungsi Setup --
void setup() {
  // Init LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  
  Serial.begin(115200);
  delay(2000); // Delay lebih lama untuk stabilitas
  Serial.println("\n========================================");
  Serial.println("    Gateway ESP32 Starting...");
  Serial.println("========================================\n");

  // 1. Connect ke WiFi
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  
  WiFi.mode(WIFI_AP_STA);
  delay(100);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  // Blink LED while connecting
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); // Toggle LED
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, HIGH); // LED ON solid = WiFi OK
    Serial.println("\n\n✓ WiFi connected!");
  } else {
    Serial.println("\n\n✗ WiFi connection FAILED!");
    Serial.println("Check: 1) Hotspot ON? 2) SSID/Password correct?");
    while(1) {
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); // Fast blink = error
      delay(200);
    }
  }
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("WiFi Channel: ");
  Serial.println(WiFi.channel());
  Serial.print("Gateway MAC Address: ");
  Serial.println(WiFi.macAddress());

  // 2. Inisialisasi ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("ESP-NOW Receive Callback registered");
  // Send callback tidak wajib, bisa di-comment jika tidak perlu
  // esp_now_register_send_cb(OnDataSent);

  // Daftarkan Robot sebagai peer
  memcpy(peerInfo.peer_addr, carAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer (Mungkin robot belum nyala/MAC salah)");
  }
  Serial.println("✓ ESP-NOW initialized.\n");

  // 3. Konfigurasi MQTT
  Serial.println("Configuring MQTT...");
  espClient.setInsecure();
  espClient.setTimeout(10);
  
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);
  Serial.println("✓ MQTT configured.\n");
  
  Serial.println("========================================");
  Serial.println("    Gateway Ready!");
  Serial.println("========================================");
  Serial.println("LED Status:");
  Serial.println("  - Solid ON  = WiFi connected");
  Serial.println("  - Fast Blink = WiFi error");
  Serial.println("  - Slow Blink = MQTT connecting");
  Serial.println("========================================\n");
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

void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
  Serial.println(">> ESP-NOW: Data received!"); // Debug: callback dipanggil
  
  Message msg;
  memcpy(&msg, incomingData, sizeof(msg));

  Serial.print(">> Type: ");
  Serial.print(msg.type);
  Serial.print(", Cmd: ");
  Serial.println(msg.cmd);

  if (msg.type == 'S') {
    Serial.print(">> Data from Robot: Temp=");
    Serial.print(msg.temp);
    Serial.print("°C, Hum=");
    Serial.print(msg.hum);
    Serial.println("%");

    JsonDocument doc;
    doc["temperature"] = msg.temp;
    doc["humidity"] = msg.hum;

    char buffer[128];
    serializeJson(doc, buffer);

    if (client.publish(MQTT_PUB_TOPIC, buffer)) {
      Serial.println(">> MQTT: Published to robot/data");
    } else {
      Serial.println(">> MQTT: Publish FAILED!");
    }
  }
}

// Callback ini opsional, tidak wajib dipakai
// void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
//   // Kosong - tidak perlu log
// }

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
    // Blink LED slowly while connecting MQTT
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    
    Serial.print("Attempting MQTT connection...");
    String clientId = "GatewayESP32-";
    clientId += String(random(0xffff), HEX);
    
    // Connect menggunakan USER dan PASS yang baru
    if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
      digitalWrite(LED_BUILTIN, HIGH); // LED ON solid = MQTT OK
      Serial.println("connected ✓");
      Serial.println("Subscribing to topics...");
      client.subscribe(MQTT_SUB_TOPIC_CONTROL);
      client.subscribe(MQTT_SUB_TOPIC_MODE);
      Serial.println("✓ Ready to receive commands!\n");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.print(" | ");
      
      // Print error meaning
      switch(client.state()) {
        case -4: Serial.println("TIMEOUT"); break;
        case -3: Serial.println("CONNECTION_LOST"); break;
        case -2: Serial.println("CONNECT_FAILED"); break;
        case -1: Serial.println("DISCONNECTED"); break;
        case 1: Serial.println("BAD_PROTOCOL"); break;
        case 2: Serial.println("BAD_CLIENT_ID"); break;
        case 3: Serial.println("UNAVAILABLE"); break;
        case 4: Serial.println("BAD_CREDENTIALS"); break;
        case 5: Serial.println("UNAUTHORIZED"); break;
        default: Serial.println("UNKNOWN"); break;
      }
      
      Serial.println("Retrying in 5 seconds...\n");
      delay(5000);
    }
  }
}