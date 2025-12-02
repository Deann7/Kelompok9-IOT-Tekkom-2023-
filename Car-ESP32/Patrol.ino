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

// TASK 2: PATROL
void TaskPatrol(void *pvParameters) {
  for (;;) {
    if (isPatrolMode) {
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
      // Jika mode manual, tidur lama biar hemat CPU
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }
}