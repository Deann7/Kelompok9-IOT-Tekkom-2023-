/*
  ================================================================
                      GATEWAY ESP32 CODE
  ================================================================
  Fungsi utama:
  1. Menerima data sensor dari Robot via ESP-NOW.
  2. Meneruskan data sensor tersebut ke MQTT Broker (untuk Node-RED).
  3. Menerima perintah dari MQTT Broker (dari Node-RED).
  4. Meneruskan perintah tersebut ke Robot via ESP-NOW.

  Library yang dibutuhkan (Install via Library Manager):
  - PubSubClient by Nick O'Leary
  - ArduinoJson by Benoit Blanchon
*/

#include <esp_now.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// -- Konfigurasi Jaringan & MQTT --
#define WIFI_SSID "GANTI_DENGAN_NAMA_WIFI"
#define WIFI_PASS "GANTI_DENGAN_PASSWORD_WIFI"

#define MQTT_SERVER "GANTI_DENGAN_IP_MQTT_BROKER"
#define MQTT_PORT 1883
#define MQTT_USER "" // Kosongkan jika tidak pakai username
#define MQTT_PASS "" // Kosongkan jika tidak pakai password

#define MQTT_PUB_TOPIC "robot/data"           // Topik untuk publikasi data sensor
#define MQTT_SUB_TOPIC_CONTROL "robot/control"// Topik untuk subscribe perintah gerak
#define MQTT_SUB_TOPIC_MODE "robot/mode"      // Topik untuk subscribe perintah mode

// -- Konfigurasi ESP-NOW --
// GANTI DENGAN ALAMAT MAC ESP32 ROBOT
uint8_t carAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// -- Deklarasi Objek Global --
WiFiClient espClient;
PubSubClient client(espClient);
esp_now_peer_info_t peerInfo;

// -- Struktur Data untuk Komunikasi --
// Struct ini harus SAMA PERSIS dengan yang ada di kode Robot
typedef struct Message {
  char type;    // 'C' (Command) atau 'S' (Sensor)
  char cmd;     // F, B, L, R, S, P, M
  float temp;
  float hum;
} Message;

// -- Prototipe Fungsi --
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len);
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
  WiFi.mode(WIFI_STA);
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
    Serial.println("Failed to add peer");
    return;
  }
  Serial.println("ESP-NOW initialized, peer added.");

  // 3. Konfigurasi MQTT
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

// Callback saat data diterima dari Robot (via ESP-NOW)
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  Message msg;
  memcpy(&msg, incomingData, sizeof(msg));

  // Pastikan tipe data adalah 'S' (Sensor)
  if (msg.type == 'S') {
    Serial.print("Sensor data received from Robot: ");
    Serial.print("Temp=");
    Serial.print(msg.temp);
    Serial.print("C, Hum=");
    Serial.print(msg.hum);
    Serial.println("%");

    // Buat JSON object untuk dikirim ke MQTT
    StaticJsonDocument<128> doc;
    doc["temperature"] = msg.temp;
    doc["humidity"] = msg.hum;

    char buffer[128];
    serializeJson(doc, buffer);

    // Publikasikan ke topik MQTT
    client.publish(MQTT_PUB_TOPIC, buffer);
    Serial.print("Published to MQTT topic '");
    Serial.print(MQTT_PUB_TOPIC);
    Serial.print("': ");
    Serial.println(buffer);
  }
}

// Callback saat data selesai dikirim (via ESP-NOW)
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("ESP-NOW Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

// Callback saat data diterima dari MQTT Broker
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  Message cmd_msg;
  cmd_msg.type = 'C'; // Tipe pesan adalah Command

  // Cek topik dan isi pesannya
  if (String(topic) == MQTT_SUB_TOPIC_CONTROL || String(topic) == MQTT_SUB_TOPIC_MODE) {
    if (message.length() > 0) {
      cmd_msg.cmd = message[0]; // Ambil karakter pertama sebagai perintah
      
      // Kirim perintah ke Robot via ESP-NOW
      esp_err_t result = esp_now_send(carAddress, (uint8_t *) &cmd_msg, sizeof(cmd_msg));

      if (result == ESP_OK) {
        Serial.print("Sent command '");
        Serial.print(cmd_msg.cmd);
        Serial.println("' to Robot.");
      } else {
        Serial.println("Error sending command to Robot.");
      }
    }
  }
}

// Fungsi untuk re-koneksi ke MQTT Broker
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Coba konek
    if (client.connect("GatewayESP32", MQTT_USER, MQTT_PASS)) {
      Serial.println("connected");
      // Subscribe ke topik-topik perintah
      client.subscribe(MQTT_SUB_TOPIC_CONTROL);
      client.subscribe(MQTT_SUB_TOPIC_MODE);
      Serial.print("Subscribed to: ");
      Serial.print(MQTT_SUB_TOPIC_CONTROL);
      Serial.print(" and ");
      Serial.println(MQTT_SUB_TOPIC_MODE);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}
