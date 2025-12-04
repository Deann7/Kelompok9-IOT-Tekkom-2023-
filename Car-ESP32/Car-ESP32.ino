#include <ESP32Servo.h>
#include <DHT.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <esp_now.h>
#include <WiFi.h>

//================================================================
//                    DEFINISI GLOBAL & SETUP
//================================================================

// --- 1. DEFINISI STRUKTUR DATA (WAJIB ADA DI FILE UTAMA) ---
typedef struct Message {
  char type;    // 'C' (Command) atau 'S' (Sensor)
  char cmd;     // F, B, L, R, S, P, M
  float temp;
  float hum;
} Message;

// -- Definisi Pin Hardware --
#define IN1 25 // Pin motor
#define IN4 26 // Pin motor
#define STEERING_SERVO_PIN 15 // Pin servo kemudi
#define SENSOR_ARM_SERVO_PIN 12 // Pin servo lengan sensor
#define DHT_PIN 4               // Pin sensor DHT11
#define DHT_TYPE DHT11

// -- Konfigurasi Global --
// GANTI DENGAN ALAMAT MAC GATEWAY ANDA (ESP32 yang connect WiFi)
uint8_t gatewayAddress[] = {0xD0, 0xEF, 0x76, 0x34, 0x12, 0xC4};
bool isPatrolMode = false; // Status mode default: Manual

// -- Deklarasi Objek & Handle Global (Biar file lain bisa baca) --
Servo steeringServo;
Servo sensorArmServo;
DHT dht(DHT_PIN, DHT_TYPE);
QueueHandle_t commandQueue;
QueueHandle_t sensorQueue;

// -- Prototipe Fungsi (Biar compiler tau fungsi ini ada di file lain) --
void TaskComm(void *pvParameters);
void TaskPatrol(void *pvParameters);
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len);
void TaskControl(void *pvParameters);

// -- Helper Functions --
void moveForward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN4, LOW);
}
void moveBackward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN4, HIGH);
}
void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN4, LOW);
}

// -- Setup --
void setup() {
  Serial.begin(115200);
  
  // Init Motor & Servo
  pinMode(IN1, OUTPUT);
  pinMode(IN4, OUTPUT);
  stopMotors();
  steeringServo.attach(STEERING_SERVO_PIN);
  sensorArmServo.attach(SENSOR_ARM_SERVO_PIN);
  steeringServo.write(90); 
  sensorArmServo.write(0);  
  dht.begin();

  // Init ESP-NOW
  WiFi.mode(WIFI_STA);
  WiFi.setChannel(6); // CRITICAL: Force channel 6 (same as Gateway)
  
  Serial.print("Robot MAC Address: ");
  Serial.println(WiFi.macAddress());
  Serial.print("Robot WiFi Channel: ");
  Serial.println(WiFi.channel());
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, gatewayAddress, 6);
  peerInfo.channel = 6; // CRITICAL: Set to channel 6
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
  } else {
    Serial.println("Gateway peer added successfully!");
    Serial.print("Target Gateway MAC: ");
    for(int i=0; i<6; i++){
      Serial.printf("%02X", gatewayAddress[i]);
      if(i<5) Serial.print(":");
    }
    Serial.println();
  }

  // Init Queue
  commandQueue = xQueueCreate(10, sizeof(Message));
  sensorQueue = xQueueCreate(5, sizeof(Message));

  // Create Tasks
  xTaskCreate(TaskControl, "Ctrl", 2048, NULL, 3, NULL);
  xTaskCreate(TaskPatrol,  "Patrol", 4096, NULL, 1, NULL);
  xTaskCreate(TaskComm,    "Comm", 4096, NULL, 2, NULL);
  
  Serial.println("Setup complete.");
}

void loop() {
  vTaskDelete(NULL);
}

// -- TASK CONTROL (Ada di file utama) --
void TaskControl(void *pvParameters) {
  Message msg;
  int steerAngle = 90;
  for (;;) {
    if (xQueueReceive(commandQueue, &msg, portMAX_DELAY) == pdPASS) {
      if (msg.cmd == 'P') { isPatrolMode = true; stopMotors(); steeringServo.write(90); }
      else if (msg.cmd == 'M') { isPatrolMode = false; stopMotors(); steeringServo.write(90); }

      if (!isPatrolMode) {
        if (msg.cmd == 'F') moveForward();
        else if (msg.cmd == 'B') moveBackward();
        else if (msg.cmd != 'L' && msg.cmd != 'R') stopMotors();
        
        if (msg.cmd == 'L') steerAngle = 60;
        else if (msg.cmd == 'R') steerAngle = 120;
        else steerAngle = 90;
        steeringServo.write(steerAngle);
      }
    }
  }
}