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

  sensorData.temp = dht.readTemperature();
  sensorData.hum = dht.readHumidity();

  sensorArmServo.write(0); // Naik
  vTaskDelay(pdMS_TO_TICKS(1000));

  if (!isnan(sensorData.temp) && !isnan(sensorData.hum)) {
    if (xQueueSend(sensorQueue, &sensorData, pdMS_TO_TICKS(100)) == pdPASS) {
      Serial.println("Patrol: Queued Data");
    }
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
        
        // Baca sensor tanpa turun-naikkan servo arm
        sensorData.temp = dht.readTemperature();
        sensorData.hum = dht.readHumidity();
        
        if (!isnan(sensorData.temp) && !isnan(sensorData.hum)) {
          if (xQueueSend(sensorQueue, &sensorData, pdMS_TO_TICKS(100)) == pdPASS) {
            Serial.print("Manual Mode - Temp: ");
            Serial.print(sensorData.temp);
            Serial.print("°C, Hum: ");
            Serial.print(sensorData.hum);
            Serial.println("%");
          }
        }
        
        lastSensorRead = now;
      }
      
      // Tidur sebentar untuk hemat CPU
      vTaskDelay(pdMS_TO_TICKS(500));
    }
  }
}