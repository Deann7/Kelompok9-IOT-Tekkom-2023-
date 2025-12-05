// File ini akan otomatis digabung di bawah Car-ESP32.ino saat compile.

#define LONG_EDGE_DURATION 3000  
#define SHORT_EDGE_DURATION 1000 
#define TURN_DURATION 1200       

#define ANGLE_STRAIGHT 90
#define ANGLE_LEFT 60
#define ANGLE_RIGHT 120

// Helper untuk ambil data sensor saat patroli
static void take_sensor_reading() {
  Message sensorData;
  sensorData.type = 'S';

  stopMotors();
  Serial.println("Patrol: Sampling...");
  vTaskDelay(pdMS_TO_TICKS(500));

  sensorArmServo.write(90); // Turun
  vTaskDelay(pdMS_TO_TICKS(1000));

  // MOCK DATA: DHT11 sensor rusak, menggunakan data simulasi
  // Simulasi suhu: 25-30°C dengan sedikit variasi
  // Simulasi kelembaban: 50-70% dengan sedikit variasi
  static float baseTemp = 27.5;
  static float baseHum = 60.0;
  static int counter = 0;
  
  // Tambahkan sedikit variasi untuk simulasi
  sensorData.temp = baseTemp + (counter % 10) * 0.3 - 1.5; // 25.0 - 28.5°C
  sensorData.hum = baseHum + (counter % 15) * 0.5 - 3.5;   // 56.5 - 64.0%
  counter++;
  
  Serial.print("Patrol: Mock Data - Temp: ");
  Serial.print(sensorData.temp);
  Serial.print("°C, Hum: ");
  Serial.print(sensorData.hum);
  Serial.println("%");

  sensorArmServo.write(0); // Naik
  vTaskDelay(pdMS_TO_TICKS(1000));

  // Validasi tidak diperlukan untuk mock data, langsung kirim
  if (xQueueSend(sensorQueue, &sensorData, pdMS_TO_TICKS(100)) == pdPASS) {
    Serial.println("Patrol: Queued Data");
  }
  vTaskDelay(pdMS_TO_TICKS(200));
}

// TASK 2: PATROL (Modified - Sensor reading setiap 5 detik di mode manual)
void TaskPatrol(void *pvParameters) {
  TickType_t lastSensorRead = 0;
  const TickType_t sensorInterval = pdMS_TO_TICKS(5000); // 5 detik
  
  for (;;) {
    if (isPatrolMode) {
      // Mode PATROL: Baca sensor + gerak otomatis
      take_sensor_reading();

      // --- Siklus Kanan ---
      steeringServo.write(ANGLE_STRAIGHT); moveForward();
      vTaskDelay(pdMS_TO_TICKS(LONG_EDGE_DURATION));
      
      steeringServo.write(ANGLE_RIGHT); moveForward();
      vTaskDelay(pdMS_TO_TICKS(TURN_DURATION));
      stopMotors(); vTaskDelay(200);

      steeringServo.write(ANGLE_STRAIGHT); moveForward();
      vTaskDelay(pdMS_TO_TICKS(SHORT_EDGE_DURATION));
      take_sensor_reading();

      steeringServo.write(ANGLE_RIGHT); moveForward();
      vTaskDelay(pdMS_TO_TICKS(TURN_DURATION));
      stopMotors(); vTaskDelay(200);

      // --- Siklus Kiri (Balik) ---
      steeringServo.write(ANGLE_STRAIGHT); moveForward();
      vTaskDelay(pdMS_TO_TICKS(LONG_EDGE_DURATION));

      steeringServo.write(ANGLE_LEFT); moveForward();
      vTaskDelay(pdMS_TO_TICKS(TURN_DURATION));
      stopMotors(); vTaskDelay(200);

      steeringServo.write(ANGLE_STRAIGHT); moveForward();
      vTaskDelay(pdMS_TO_TICKS(SHORT_EDGE_DURATION));
      take_sensor_reading();

      steeringServo.write(ANGLE_LEFT); moveForward();
      vTaskDelay(pdMS_TO_TICKS(TURN_DURATION));
      stopMotors(); vTaskDelay(200);

    } else {
      // Mode MANUAL: Tetap baca sensor setiap 5 detik (tanpa servo arm)
      TickType_t now = xTaskGetTickCount();
      if ((now - lastSensorRead) >= sensorInterval) {
        Message sensorData;
        sensorData.type = 'S';
        
        // MOCK DATA: DHT11 sensor rusak, menggunakan data simulasi
        // Simulasi suhu: 25-30°C dengan sedikit variasi
        // Simulasi kelembaban: 50-70% dengan sedikit variasi
        static float baseTemp = 27.5;
        static float baseHum = 60.0;
        static int counter = 0;
        
        // Tambahkan sedikit variasi untuk simulasi
        sensorData.temp = baseTemp + (counter % 10) * 0.3 - 1.5; // 25.0 - 28.5°C
        sensorData.hum = baseHum + (counter % 15) * 0.5 - 3.5;   // 56.5 - 64.0%
        counter++;
        
        // Validasi tidak diperlukan untuk mock data, langsung kirim
        if (xQueueSend(sensorQueue, &sensorData, pdMS_TO_TICKS(100)) == pdPASS) {
          Serial.print("Manual Mode - Mock Data - Temp: ");
          Serial.print(sensorData.temp);
          Serial.print("°C, Hum: ");
          Serial.print(sensorData.hum);
          Serial.println("%");
        }
        
        lastSensorRead = now;
      }
      
      // Tidur sebentar untuk hemat CPU
      vTaskDelay(pdMS_TO_TICKS(500));
    }
  }
}