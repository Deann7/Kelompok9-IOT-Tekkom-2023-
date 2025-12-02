// Durasi untuk setiap gerakan dalam milidetik.
// Nilai ini perlu dikalibrasi sesuai kecepatan motor dan ukuran area.
#define LONG_EDGE_DURATION 3000  // Durasi jalan di sisi panjang
#define SHORT_EDGE_DURATION 1000 // Durasi jalan di sisi pendek
#define TURN_DURATION 1200       // Durasi untuk berbelok 90 derajat

// Sudut untuk servo kemudi
#define ANGLE_STRAIGHT 90
#define ANGLE_LEFT 60
#define ANGLE_RIGHT 120

//================================================================
// HELPER FUNCTION for Patrol Task
// - Fungsi ini membungkus logika untuk berhenti, mengambil data
//   sensor, dan mengirimnya ke queue.
//================================================================
static void take_sensor_reading() {
  Message sensorData;
  sensorData.type = 'S';

  // 1. Berhenti total
  stopMotors();
  Serial.println("Patrol: Stopping for sensor reading...");
  vTaskDelay(pdMS_TO_TICKS(500));

  // 2. Turunkan lengan sensor
  sensorArmServo.write(90);
  vTaskDelay(pdMS_TO_TICKS(1000));

  // 3. Baca sensor
  sensorData.temp = dht.readTemperature();
  sensorData.hum = dht.readHumidity();

  // 4. Naikkan kembali lengan sensor
  sensorArmServo.write(0);
  vTaskDelay(pdMS_TO_TICKS(1000));

  // 5. Kirim data ke queue jika valid
  if (!isnan(sensorData.temp) && !isnan(sensorData.hum)) {
    if (xQueueSend(sensorQueue, &sensorData, pdMS_TO_TICKS(100)) != pdPASS) {
      Serial.println("Patrol: Failed to send sensor data to queue.");
    } else {
      Serial.print("Patrol: Sensor data sent (T:");
      Serial.print(sensorData.temp);
      Serial.print(" H:");
      Serial.print(sensorData.hum);
      Serial.println(")");
    }
  } else {
    Serial.println("Patrol: Failed to read from DHT sensor!");
  }

  // Jeda sejenak sebelum melanjutkan gerakan
  vTaskDelay(pdMS_TO_TICKS(500));
}

//================================================================
// TASK 2: PATROL (BOUSTROPHEDON PATTERN)
// - Menjalankan robot dalam pola "Ox-Turning" untuk menutupi area.
// - Terdiri dari 2 siklus utama: satu dengan belokan kanan, 
//   satu lagi dengan belokan kiri.
//================================================================
void TaskPatrol(void *pvParameters) {
  for (;;) {
    // Task ini hanya berjalan jika mode patroli aktif
    if (isPatrolMode) {

      // Ambil 1 data di titik start
      take_sensor_reading();

      // ====== SIKLUS 1: BELOKAN KANAN ======
      
      // 1. Jalan lurus di sisi panjang pertama
      Serial.println("Patrol: Moving forward (long edge)...");
      steeringServo.write(ANGLE_STRAIGHT);
      moveForward();
      vTaskDelay(pdMS_TO_TICKS(LONG_EDGE_DURATION));
      take_sensor_reading();

      // 2. Belok kanan 90 derajat
      Serial.println("Patrol: Turning right...");
      steeringServo.write(ANGLE_RIGHT);
      moveForward();
      vTaskDelay(pdMS_TO_TICKS(TURN_DURATION));
      stopMotors(); // Berhenti sejenak setelah belok
      vTaskDelay(pdMS_TO_TICKS(200));

      // 3. Jalan lurus di sisi pendek
      Serial.println("Patrol: Moving forward (short edge)...");
      steeringServo.write(ANGLE_STRAIGHT);
      moveForward();
      vTaskDelay(pdMS_TO_TICKS(SHORT_EDGE_DURATION));
      take_sensor_reading();

      // 4. Belok kanan 90 derajat lagi untuk kembali ke jalur seberang
      Serial.println("Patrol: Turning right...");
      steeringServo.write(ANGLE_RIGHT);
      moveForward();
      vTaskDelay(pdMS_TO_TICKS(TURN_DURATION));
      stopMotors();
      vTaskDelay(pdMS_TO_TICKS(200));

      // ====== SIKLUS 2: BELOKAN KIRI ======

      // 5. Jalan lurus di sisi panjang (arah berlawanan)
      Serial.println("Patrol: Moving forward (long edge, return)...");
      steeringServo.write(ANGLE_STRAIGHT);
      moveForward();
      vTaskDelay(pdMS_TO_TICKS(LONG_EDGE_DURATION));
      take_sensor_reading();

      // 6. Belok kiri 90 derajat
      Serial.println("Patrol: Turning left...");
      steeringServo.write(ANGLE_LEFT);
      moveForward();
      vTaskDelay(pdMS_TO_TICKS(TURN_DURATION));
      stopMotors();
      vTaskDelay(pdMS_TO_TICKS(200));

      // 7. Jalan lurus di sisi pendek lagi
      Serial.println("Patrol: Moving forward (short edge)...");
      steeringServo.write(ANGLE_STRAIGHT);
      moveForward();
      vTaskDelay(pdMS_TO_TICKS(SHORT_EDGE_DURATION));
      take_sensor_reading();

      // 8. Belok kiri 90 derajat untuk kembali ke titik awal (secara horizontal)
      Serial.println("Patrol: Turning left...");
      steeringServo.write(ANGLE_LEFT);
      moveForward();
      vTaskDelay(pdMS_TO_TICKS(TURN_DURATION));
      stopMotors();
      vTaskDelay(pdMS_TO_TICKS(200));

      // Pola selesai, akan diulang dari awal.
      Serial.println("Patrol: Full Boustrophedon cycle complete. Repeating.");

    } else {
      // Jika mode manual, task ini "tidur" sejenak untuk efisiensi.
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }
}
