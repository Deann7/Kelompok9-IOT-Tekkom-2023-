// File ini akan otomatis digabung di bawah Car-ESP32.ino saat compile.
// Variabel seperti sensorQueue, commandQueue, gatewayAddress sudah dikenali.

//================================================================
// TASK 3: COMMUNICATION
//================================================================
void TaskComm(void *pvParameters) {
  Message dataToSend;
  for (;;) {
    // Tunggu data sensor dari Queue (Timeout 100ms)
    if (xQueueReceive(sensorQueue, &dataToSend, pdMS_TO_TICKS(100)) == pdPASS) {
      
      // Kirim ke Gateway
      esp_err_t result = esp_now_send(gatewayAddress, (uint8_t *) &dataToSend, sizeof(dataToSend));
      
      if (result == ESP_OK) {
        Serial.println(">> Comm: Data sent to Gateway");
      } else {
        Serial.println(">> Comm: Send Failed");
      }
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

//================================================================
// CALLBACK FUNCTION ESP-NOW (Data Masuk)
//================================================================
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
  Message packet;
  memcpy(&packet, incomingData, sizeof(packet));
  
  // Jika tipe Command ('C'), kirim ke queue kontrol
  if (packet.type == 'C') {
    xQueueSendFromISR(commandQueue, &packet, NULL);
  }
}