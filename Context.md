# Proposal Proyek Akhir IoT: Smart Agri-Patrol Bot

## 1\. Pendahuluan

Proyek ini berjudul **"Smart Agri-Patrol"**, sebuah robot *Unmanned Ground Vehicle* (UGV) yang dirancang untuk memantau kondisi lingkungan pertanian (suhu dan kelembaban mikro) secara otomatis maupun manual. Proyek ini bertujuan untuk memenuhi kriteria Proyek Akhir Laboratorium Digital dengan menerapkan konsep *Real-Time Operating System* (FreeRTOS) dan komunikasi *Machine-to-Machine* (M2M) antar mikrokontroler.

Produk ini memiliki nilai guna untuk menyelesaikan masalah aktual, yaitu efisiensi pemantauan lahan pertanian yang luas tanpa perlu pengecekan manual oleh manusia ke setiap titik.

## 2\. Arsitektur Sistem

[cite\_start]Sistem ini menggunakan dua unit ESP32 untuk memenuhi syarat komunikasi antar *device*:

1.  **Node Robot (Mobile Unit):**
      * **Fungsi:** Bergerak di lapangan, mengendalikan aktuator (motor & servo), dan membaca sensor.
      * **Input:** Sensor DHT11 (Suhu & Kelembaban).
      * **Output:** 2x DC Motor (Roda), 1x Servo (Lengan Sensor), Indikator LED.
      * **Power:** Baterai 3.7V - 7.4V.
2.  **Node Gateway (Base Station):**
      * **Fungsi:** Menerima data dari robot secara nirkabel (Lokal) dan meneruskannya ke Internet (MQTT Cloud/Node-RED).
      * **Output:** Tampilan Dashboard pada Node-RED.

## 3\. Implementasi Modul Praktikum

Proyek ini mengintegrasikan lebih dari 6 modul praktikum sesuai syarat minimal:

  **Modul 1-5 (FreeRTOS):** Implementasi *Task Management* (Control, Patrol, Comm), *Queue* untuk pertukaran data antar task, dan *Software Timer* untuk durasi patroli.
  * **Modul 7 (MQTT & WiFi):** Digunakan pada Node Gateway untuk komunikasi ke Node-RED.
  * **Modul 9 (IoT Platform):** Visualisasi data dan kontrol via Node-RED.
  * **Modul 10 (Mesh/Wireless):** Komunikasi *peer-to-peer* (ESP-NOW) antara Robot dan Gateway.

-----

## 4\. Rancangan Algoritma & Logika (FreeRTOS)

Sistem pada **Node Robot** adalah yang paling kompleks karena menangani tiga fungsi utama secara paralel. Kita menggunakan FreeRTOS untuk membagi fungsi tersebut menjadi tiga *Task* independen.

### A. Komunikasi & Manajemen Data (*Communication Logic*)

Bagian ini menangani alur data masuk (Perintah dari Gateway) dan data keluar (Sensor ke Gateway).

  * **Mekanisme:** Menggunakan protokol ESP-NOW (Latensi rendah) untuk komunikasi antar ESP.
  * **Alur Data Masuk:**
    1.  Fungsi *Callback* (`OnDataRecv`) menerima paket data.
    2.  Data divalidasi dan dimasukkan ke dalam `CommandQueue`.
    3.  Penggunaan Queue menjamin data tidak hilang meski CPU sedang sibuk mengurus motor.
  * **Alur Data Keluar:**
    1.  Task menunggu data dari `SensorQueue`.
    2.  Jika data sensor tersedia (dari Task Patrol), data dikirim via ESP-NOW ke Gateway.

### B. Mode Kontrol Manual (*Control Logic*)

Mode ini memungkinkan pengguna mengendalikan robot layaknya mobil remote control melalui Dashboard Node-RED.

  * **Prioritas Task:** Tinggi (High Priority), agar robot responsif dan bisa berhenti mendadak jika diperlukan.
  * **Algoritma:**
    1.  Task membaca `CommandQueue`.
    2.  Jika mode aktif adalah **MANUAL**:
          * Terjemahkan karakter perintah:
              * `'F'` -\> Kedua Motor Maju.
              * `'B'` -\> Kedua Motor Mundur.
              * `'L'` -\> Motor Kanan Maju, Kiri Mundur (Pivot).
              * `'R'` -\> Motor Kiri Maju, Kanan Mundur (Pivot).
              * `'S'` -\> Semua Motor Stop.
    3.  Jika menerima perintah ganti mode (`'P'` atau `'M'`), update variabel global `isPatrolMode`.

### C. Mode Patroli Otomatis (*Patrol Logic*)

Mode ini menjalankan robot dalam pola *Square Tile* (kotak) dan melakukan sampling data secara berkala.

  * **Prioritas Task:** Rendah/Menengah.
  * **Algoritma (State Machine):**
    1.  Cek apakah `isPatrolMode == TRUE`.
    2.  **Fase Navigasi:** Robot bergerak maju selama $t$ detik (menggunakan `vTaskDelay` atau *Software Timer*).
    3.  **Fase Stop & Sampling:**
          * Hentikan motor.
          * **Aktuasi Servo:** Turunkan servo lengan ($0^\circ \to 90^\circ$) untuk mendekatkan DHT11 ke permukaan tanah.
          * **Reading:** Baca data suhu & kelembaban.
          * **Sending:** Masukkan data ke `SensorQueue`.
          * **Recovery:** Naikkan servo kembali ($90^\circ \to 0^\circ$).
    4.  **Fase Turning:** Robot berbelok 90 derajat untuk membentuk pola kotak.
    5.  Ulangi siklus.

-----

## 5\. Implementasi Kode (Pseudocode Struktur)

Berikut adalah gambaran struktur kode untuk **Node Robot** menggunakan Arduino Framework dengan FreeRTOS.

```cpp
#include <esp_now.h>
#include <WiFi.h>
#include <ESP32Servo.h>

// Definisi Queue
QueueHandle_t commandQueue;
QueueHandle_t sensorQueue;

// Struktur Data Paket
typedef struct {
  char type;    // 'C' (Command) atau 'S' (Sensor)
  char cmd;     // F, B, L, R, S, P, M
  float temp;
  float hum;
} Message;

bool isPatrolMode = false;

// 1. TASK CONTROL (Manual)
void TaskControl(void *pvParameters) {
  Message msg;
  for (;;) {
    // Tunggu perintah dari Queue
    if (xQueueReceive(commandQueue, &msg, portMAX_DELAY) == pdPASS) {
      
      // Logika Ganti Mode
      if (msg.cmd == 'P') isPatrolMode = true;
      else if (msg.cmd == 'M') {
        isPatrolMode = false;
        stopMotors();
      }

      // Logika Gerak (Hanya jika Mode Manual)
      if (!isPatrolMode) {
        switch (msg.cmd) {
          case 'F': moveForward(); break;
          case 'B': moveBackward(); break;
          case 'L': turnLeft(); break;
          case 'R': turnRight(); break;
          case 'S': stopMotors(); break;
        }
      }
    }
  }
}

// 2. TASK PATROL (Otomatis)
void TaskPatrol(void *pvParameters) {
  Message sensorData;
  sensorData.type = 'S'; // Tipe data Sensor
  
  for (;;) {
    if (isPatrolMode) {
      // -- Step 1: Jalan --
      moveForward();
      vTaskDelay(2000 / portTICK_PERIOD_MS); // Jalan 2 detik

      // -- Step 2: Sampling --
      stopMotors();
      vTaskDelay(500 / portTICK_PERIOD_MS);
      
      servoLengan.write(90); // Turunkan sensor
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      
      // Baca Sensor
      sensorData.temp = dht.readTemperature();
      sensorData.hum = dht.readHumidity();
      
      // Kirim ke Queue Komunikasi
      xQueueSend(sensorQueue, &sensorData, 0);

      servoLengan.write(0); // Naikkan sensor
      vTaskDelay(1000 / portTICK_PERIOD_MS);

      // -- Step 3: Belok (Pola Kotak) --
      turnRight();
      vTaskDelay(600 / portTICK_PERIOD_MS); // Kalibrasi waktu belok 90 derajat
      stopMotors();
      
    } else {
      // Jika mode manual, task ini "tidur"
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
  }
}

// 3. TASK COMMUNICATION
void TaskComm(void *pvParameters) {
  Message dataToSend;
  for (;;) {
    // Cek apakah ada data sensor yang perlu dikirim ke Gateway
    if (xQueueReceive(sensorQueue, &dataToSend, 100) == pdPASS) {
      esp_now_send(gatewayAddress, (uint8_t *) &dataToSend, sizeof(dataToSend));
    }
    vTaskDelay(10);
  }
}

// Callback ESP-NOW (Dipanggil saat data masuk dari Gateway)
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  Message packet;
  memcpy(&packet, incomingData, sizeof(packet));
  // Kirim ke Queue Control agar diproses Task Control
  xQueueSendFromISR(commandQueue, &packet, NULL);
}

void setup() {
  // Inisialisasi Hardware & WiFi ESP-NOW
  // ...
  
  // Buat Queue
  commandQueue = xQueueCreate(10, sizeof(Message));
  sensorQueue = xQueueCreate(5, sizeof(Message));

  // Buat Task FreeRTOS
  xTaskCreate(TaskControl, "Ctrl", 2048, NULL, 3, NULL); // Prioritas Tinggi
  xTaskCreate(TaskComm,    "Comm", 4096, NULL, 2, NULL); // Prioritas Sedang
  xTaskCreate(TaskPatrol,  "PtrL", 4096, NULL, 1, NULL); // Prioritas Rendah
}

void loop() {
  // Loop kosong karena menggunakan FreeRTOS
}
```

-----

## 6\. Integrasi IoT Platform (Node-RED)

[cite\_start]Sesuai ketentuan penggunaan modul IoT[cite: 9], sistem menggunakan **Node-RED** sebagai antarmuka pengguna (*User Interface*).

  * **Komunikasi:** Node-RED terhubung ke Broker MQTT (misal: HiveMQ atau Mosquitto lokal). Node Gateway (ESP32 ke-2) bertindak sebagai *Publisher* dan *Subscriber* MQTT.
  * **Dashboard UI:**
    1.  **Switch Mode:** Mengirim string "M" (Manual) atau "P" (Patrol) ke topik `robot/mode`.
    2.  **D-Pad Controller:** Tombol Atas/Bawah/Kiri/Kanan mengirim perintah gerak ke topik `robot/control`.
    3.  **Gauge & Chart:** Menampilkan *payload* suhu dan kelembaban yang diterima dari topik `robot/data`.

