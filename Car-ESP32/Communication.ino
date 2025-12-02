#include <esp_now.h>
#include <WiFi.h>

//================================================================
// TASK 3: COMMUNICATION
// - Mengirim data sensor dari 'sensorQueue' ke Gateway via ESP-NOW.
//================================================================
void TaskComm(void *pvParameters) {
  Message dataToSend;
  for (;;) {
    // Cek apakah ada data sensor di queue yang perlu dikirim
    // Tunda task selama 100ms jika tidak ada data.
    if (xQueueReceive(sensorQueue, &dataToSend, pdMS_TO_TICKS(100)) == pdPASS) {
      
      // Kirim data via ESP-NOW
      esp_err_t result = esp_now_send(gatewayAddress, (uint8_t *) &dataToSend, sizeof(dataToSend));
      
      // Beri feedback di Serial Monitor
      if (result == ESP_OK) {
        Serial.println("Sensor data sent via ESP-NOW successfully.");
      } else {
        Serial.println("Error sending sensor data via ESP-NOW.");
      }
    }
    // Beri jeda singkat agar task lain bisa berjalan
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}


//================================================================
// CALLBACK FUNCTION ESP-NOW
// - Fungsi ini dipanggil secara otomatis (interrupt) setiap kali 
//   ada data masuk dari device yang terdaftar (Gateway).
// - Tugasnya hanya satu: ambil data dan masukkan ke 'commandQueue'.
//================================================================
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  Message packet;
  // Salin data yang masuk ke dalam sebuah struct
  memcpy(&packet, incomingData, sizeof(packet));
  
  // Validasi sederhana: cek tipe paket (harus 'C' untuk Command)
  if (packet.type == 'C') {
    // Kirim struct ke 'commandQueue' agar bisa diolah oleh TaskControl.
    // Gunakan 'xQueueSendFromISR' karena fungsi ini berjalan dalam konteks interrupt.
    xQueueSendFromISR(commandQueue, &packet, NULL);
  }
}
