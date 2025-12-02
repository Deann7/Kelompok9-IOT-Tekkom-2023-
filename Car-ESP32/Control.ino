// -- Blok Konfigurasi Awal --
// Baris ini digunakan untuk mengaktifkan pengaturan khusus jika diperlukan oleh library.
#define CUSTOM_SETTINGS
// Baris ini secara spesifik menyertakan modul Gamepad dari library Dabble.
#define INCLUDE_GAMEPAD_MODULE
// Menyertakan library utama Dabble untuk komunikasi dengan aplikasi di smartphone.
#include <DabbleESP32.h>
// Menyertakan library untuk mengontrol motor servo pada ESP32.
#include <ESP32Servo.h>
// Menyertakan library FreeRTOS untuk dual core dan task management.
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>


// -- Definisi Pin Hardware --
// Mendefinisikan pin 25 pada ESP32 untuk mengontrol motor agar bergerak maju.
#define IN1 25
// Mendefinisikan pin 26 pada ESP32 untuk mengontrol motor agar bergerak mundur.
#define IN4 26
// Mendefinisikan pin 15 pada ESP32 sebagai pin sinyal untuk motor servo kemudi.
#define SERVO_PIN 15

// -- Konfigurasi Task --
// Priority tinggi untuk Bluetooth task (Core 0)
#define BLUETOOTH_TASK_PRIORITY 5
// Priority normal untuk Motor Control task (Core 1)
#define MOTOR_TASK_PRIORITY 3
// Stack size untuk setiap task
#define TASK_STACK_SIZE 4096

// -- Deklarasi Objek Global --
// Membuat sebuah objek bernama 'steering' dari kelas Servo untuk mengontrol kemudi.
Servo steering;

// -- Struktur Data untuk Komunikasi Antar Task --
// Struktur untuk menyimpan data kontrol dari Bluetooth
struct ControlData {
  bool forward;      // Tombol maju ditekan
  bool backward;     // Tombol mundur ditekan
  bool left;         // Tombol kiri ditekan
  bool right;        // Tombol kanan ditekan
};

// Queue untuk mengirim data kontrol dari Bluetooth task ke Motor task
QueueHandle_t controlQueue;

// -- Task Handles --
TaskHandle_t bluetoothTaskHandle = NULL;
TaskHandle_t motorTaskHandle = NULL;


// -- Fungsi Setup: Dijalankan sekali saat ESP32 pertama kali menyala --
void setup() {
  // Memulai komunikasi serial dengan kecepatan 115200 bps untuk debugging.
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting ESP32 RC Car with Dual Core...");

  // Membuat queue untuk komunikasi antar task (ukuran 5 item)
  controlQueue = xQueueCreate(5, sizeof(ControlData));
  if (controlQueue == NULL) {
    Serial.println("Error creating queue!");
    while(1) delay(1000);
  }

  // Inisialisasi hardware motor
  // Menghubungkan objek 'steering' ke pin yang telah didefinisikan (SERVO_PIN).
  steering.attach(SERVO_PIN);
  // Mengatur posisi awal servo ke 90 derajat (posisi netral atau lurus).
  steering.write(90);

  // Mengatur pin IN1 sebagai pin OUTPUT untuk mengirim sinyal ke driver motor.
  pinMode(IN1, OUTPUT);
  // Mengatur pin IN4 sebagai pin OUTPUT untuk mengirim sinyal ke driver motor.
  pinMode(IN4, OUTPUT);

  // Memanggil fungsi berhenti() untuk memastikan motor tidak bergerak saat pertama kali dinyalakan.
  berhenti();

  // Membuat task Bluetooth di Core 0 dengan priority tinggi
  xTaskCreatePinnedToCore(
    bluetoothTask,              // Function yang akan dijalankan
    "BluetoothTask",            // Nama task
    TASK_STACK_SIZE,           // Stack size
    NULL,                       // Parameter untuk task
    BLUETOOTH_TASK_PRIORITY,   // Priority tinggi
    &bluetoothTaskHandle,       // Task handle
    0                           // Core 0 (untuk Bluetooth)
  );

  // Membuat task Motor Control di Core 1 dengan priority normal
  xTaskCreatePinnedToCore(
    motorControlTask,          // Function yang akan dijalankan
    "MotorControlTask",         // Nama task
    TASK_STACK_SIZE,           // Stack size
    NULL,                       // Parameter untuk task
    MOTOR_TASK_PRIORITY,       // Priority normal
    &motorTaskHandle,          // Task handle
    1                           // Core 1 (untuk kontrol motor)
  );

  Serial.println("Tasks created successfully!");
}


// -- Fungsi Loop: Kosong karena menggunakan FreeRTOS tasks --
void loop() {
  // Loop kosong karena semua logika dipindah ke FreeRTOS tasks
  delay(1000);
}


// -- Task Bluetooth: Berjalan di Core 0 dengan Priority Tinggi --
// Task ini menangani komunikasi Bluetooth dan mengirim data kontrol ke queue
void bluetoothTask(void *parameter) {
  // Memulai library Dabble dan mengatur nama Bluetooth menjadi "bebasdehcar"
  Dabble.begin("bebasdehcar");
  Serial.println("Bluetooth task started on Core 0");
  Serial.println("Bluetooth device name: bebasdehcar");

  ControlData controlData;

  while (true) {
    // Memproses input yang masuk dari aplikasi Dabble secara terus-menerus.
    Dabble.processInput();

    // Membaca status tombol dari gamepad
    controlData.forward = GamePad.isCrossPressed();
    controlData.backward = GamePad.isTrianglePressed();
    controlData.left = GamePad.isLeftPressed();
    controlData.right = GamePad.isRightPressed();

    // Mengirim data kontrol ke queue (non-blocking, jika queue penuh akan diabaikan)
    if (xQueueSend(controlQueue, &controlData, 0) != pdTRUE) {
      // Queue penuh, skip frame ini
    }

    // Delay kecil untuk memberikan waktu CPU ke task lain
    vTaskDelay(pdMS_TO_TICKS(10)); // 10ms delay
  }
}


// -- Task Motor Control: Berjalan di Core 1 dengan Priority Normal --
// Task ini membaca data dari queue dan mengontrol motor
void motorControlTask(void *parameter) {
  Serial.println("Motor control task started on Core 1");

  ControlData controlData;
  int steerAngle = 90;

  while (true) {
    // Membaca data kontrol dari queue (blocking dengan timeout 50ms)
    if (xQueueReceive(controlQueue, &controlData, pdMS_TO_TICKS(50)) == pdTRUE) {
      // === Blok Kontrol Gerak Maju/Mundur Motor ===
      if (controlData.forward) {
        // Tombol maju ditekan, panggil fungsi maju()
        maju();
      }
      else if (controlData.backward) {
        // Tombol mundur ditekan, panggil fungsi mundur()
        mundur();
      }
      else {
        // Tidak ada tombol arah maju atau mundur yang ditekan, berhenti
        berhenti();
      }

      // === Blok Kontrol Kemudi (Servo) ===
      // Reset ke posisi netral
      steerAngle = 90;

      if (controlData.left) {
        // Tombol kiri ditekan, ubah nilai 'steerAngle' menjadi 60 untuk belok ke kiri
        steerAngle = 60;
      }
      else if (controlData.right) {
        // Tombol kanan ditekan, ubah nilai 'steerAngle' menjadi 120 untuk belok ke kanan
        steerAngle = 120;
      }

      // Mengirimkan nilai 'steerAngle' terakhir ke motor servo untuk mengatur arah kemudi
      steering.write(steerAngle);
    }

    // Delay kecil untuk memberikan waktu CPU ke task lain
    vTaskDelay(pdMS_TO_TICKS(20)); // 20ms delay
  }
}


// -- Kumpulan Fungsi untuk Kontrol Motor --

// Fungsi untuk menggerakkan mobil maju.
void maju() {
  // Memberi sinyal HIGH ke pin IN1 dan LOW ke pin IN4.
  digitalWrite(IN1, HIGH);
  digitalWrite(IN4, LOW);
}

// Fungsi untuk menggerakkan mobil mundur.
void mundur() {
  // Memberi sinyal LOW ke pin IN1 dan HIGH ke pin IN4.
  digitalWrite(IN1, LOW);
  digitalWrite(IN4, HIGH);
}

// Fungsi untuk menghentikan mobil.
void berhenti() {
  // Memberi sinyal LOW ke kedua pin (IN1 dan IN4) untuk menghentikan motor.
  digitalWrite(IN1, LOW);
  digitalWrite(IN4, LOW);
}