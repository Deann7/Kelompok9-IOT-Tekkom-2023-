#include <ESP32Servo.h>
#include <DHT.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

//================================================================
//                    DEFINISI GLOBAL & SETUP
//================================================================
// File ini adalah file utama yang berisi:
// 1. Definisi semua pin hardware.
// 2. Deklarasi semua objek global (servo, sensor, queue).
// 3. Fungsi setup() yang menginisialisasi sistem.
// 4. Task untuk mode kontrol manual (TaskControl).
// 5. Fungsi helper untuk kontrol motor.
//================================================================


// -- Definisi Pin Hardware --
#define IN1 25 // Pin motor
#define IN4 26 // Pin motor
#define STEERING_SERVO_PIN 15 // Pin servo kemudi
#define SENSOR_ARM_SERVO_PIN 12 // Pin servo lengan sensor
#define DHT_PIN 4               // Pin sensor DHT11
#define DHT_TYPE DHT11

// -- Konfigurasi Global --
// GANTI DENGAN ALAMAT MAC GATEWAY ANDA
uint8_t gatewayAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
bool isPatrolMode = false; // Status mode default: Manual

// -- Deklarasi Objek & Handle Global --
Servo steeringServo;
Servo sensorArmServo;
DHT dht(DHT_PIN, DHT_TYPE);
QueueHandle_t commandQueue;
QueueHandle_t sensorQueue;


// -- Prototipe Fungsi dari file lain --
// (Tidak wajib di Arduino IDE, tapi praktik yang baik)
void TaskComm(void *pvParameters);
void TaskPatrol(void *pvParameters);
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len);


// -- Fungsi Setup: Inisialisasi Sistem --
void setup() {
  Serial.begin(115200);
  
  // Inisialisasi Pin Motor
  pinMode(IN1, OUTPUT);
  pinMode(IN4, OUTPUT);
  stopMotors();

  // Inisialisasi Servo dan Sensor
  steeringServo.attach(STEERING_SERVO_PIN);
  sensorArmServo.attach(SENSOR_ARM_SERVO_PIN);
  steeringServo.write(90); // Posisi awal kemudi lurus
  sensorArmServo.write(0);  // Posisi awal lengan sensor naik
  dht.begin();

  // Inisialisasi WiFi & ESP-NOW
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Daftarkan callback function dari Communication.ino
  esp_now_register_recv_cb(OnDataRecv);

  // Daftarkan peer (Gateway)
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, gatewayAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  // Buat Queue FreeRTOS
  commandQueue = xQueueCreate(10, sizeof(Message));
  sensorQueue = xQueueCreate(5, sizeof(Message));
  if (commandQueue == NULL || sensorQueue == NULL) {
      Serial.println("Error creating queues!");
      while(1);
  }

  // Buat Tiga Task FreeRTOS (dari 3 file .ino yang berbeda)
  xTaskCreate(TaskControl, "Manual Control", 2048, NULL, 3, NULL); // Ada di file ini
  xTaskCreate(TaskPatrol,  "Patrol Logic", 4096, NULL, 1, NULL); // Dari Patrol.ino
  xTaskCreate(TaskComm,    "Communication", 4096, NULL, 2, NULL); // Dari Communication.ino
  
  Serial.println("Setup complete. All tasks running.");
}


// -- Loop Utama: Dihapus karena tidak digunakan oleh FreeRTOS --
void loop() {
  vTaskDelete(NULL);
}


// -- Kumpulan Fungsi Helper untuk Kontrol Motor --
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


//================================================================
// TASK 1: MANUAL CONTROL
// - Menunggu perintah dari 'commandQueue'.
// - Mengubah status mode (Manual/Patrol).
// - Menggerakkan motor dan servo jika dalam mode Manual.
//================================================================
void TaskControl(void *pvParameters) {
  Message msg;
  int steerAngle = 90;

  for (;;) {
    // Block task sampai ada pesan di queue
    if (xQueueReceive(commandQueue, &msg, portMAX_DELAY) == pdPASS) {
      
      // Logika untuk ganti mode
      if (msg.cmd == 'P' && !isPatrolMode) {
        isPatrolMode = true;
        Serial.println("Mode switched to: PATROL");
        stopMotors();
        steeringServo.write(90); // Luruskan roda
      } else if (msg.cmd == 'M' && isPatrolMode) {
        isPatrolMode = false;
        Serial.println("Mode switched to: MANUAL");
        stopMotors(); // Hentikan semua gerakan patroli
        steeringServo.write(90);
      }

      // Logika gerak hanya berjalan jika mode MANUAL
      if (!isPatrolMode) {
        // Kontrol Maju/Mundur
        if (msg.cmd == 'F') {
          moveForward();
        } else if (msg.cmd == 'B') {
          moveBackward();
        } else if (msg.cmd != 'L' && msg.cmd != 'R') {
          // Berhenti jika perintah bukan belok
          stopMotors();
        }
        
        // Kontrol Belok (Servo)
        if (msg.cmd == 'L') {
          steerAngle = 60;  // Sudut belok kiri
        } else if (msg.cmd == 'R') {
          steerAngle = 120; // Sudut belok kanan
        } else {
          steerAngle = 90;  // Kembali lurus jika tidak ada perintah belok
        }
        steeringServo.write(steerAngle);
      }
    }
  }
}