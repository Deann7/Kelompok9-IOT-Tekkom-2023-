# Smart Agri-Patrol Bot - Laporan Proyek Akhir IoT

## Introduction

Proyek ini berjudul **"Smart Agri-Patrol"**, sebuah robot *Unmanned Ground Vehicle* (UGV) yang dirancang untuk memantau kondisi lingkungan pertanian—khususnya suhu dan kelembaban mikro—secara otomatis maupun manual. Proyek ini dikembangkan sebagai Proyek Akhir Laboratorium Digital, dengan fokus pada implementasi konsep *Real-Time Operating System* (FreeRTOS) untuk manajemen tugas yang kompleks dan komunikasi *Machine-to-Machine* (M2M) antar mikrokontroler.

Tujuan utama dari produk ini adalah untuk meningkatkan efisiensi pemantauan lahan pertanian yang luas, mengurangi kebutuhan akan pengecekan manual oleh manusia di setiap titik, dan menyediakan data lingkungan secara *real-time* melalui dashboard IoT.

## Implementation

Arsitektur sistem ini dirancang secara modular dan terdiri dari dua unit ESP32 yang saling berkomunikasi, yaitu Robot dan Gateway.

### Arsitektur Sistem

1.  **Node Robot (Mobile Unit):** Bertugas sebagai unit eksekutor di lapangan. Robot ini dilengkapi dengan motor untuk pergerakan, dua buah servo (untuk kemudi dan lengan sensor), serta sensor DHT11 untuk membaca data suhu dan kelembaban. Otak dari robot ini adalah sebuah ESP32 yang menjalankan FreeRTOS untuk menangani tiga tugas secara paralel: kontrol manual, patroli otomatis, dan komunikasi.

2.  **Node Gateway (Base Station):** Bertindak sebagai jembatan antara Node Robot dengan jaringan internet. Unit ESP32 ini menerima data dari robot melalui protokol ESP-NOW dan meneruskannya ke MQTT Broker. Sebaliknya, ia juga menerima perintah dari MQTT Broker (yang dikirim oleh pengguna melalui Node-RED) dan meneruskannya ke robot.

### Detail Implementasi Robot Node (`Car-ESP32`)

Struktur kode pada robot dibagi menjadi tiga file `.ino` untuk modularitas, namun disatukan saat kompilasi oleh Arduino IDE dan diatur oleh scheduler FreeRTOS.

*   **`Control.ino` (File Utama):** Berisi fungsi `setup()` yang menginisialisasi semua hardware dan task FreeRTOS. File ini juga mendefinisikan semua variabel global dan berisi `TaskControl`, yaitu task dengan prioritas tertinggi yang bertanggung jawab atas mode kontrol manual.
*   **`Communication.ino`:** Mengimplementasikan `TaskComm` dan *callback* ESP-NOW. `TaskComm` bertugas mengirimkan data sensor yang ada di antrian (`sensorQueue`) ke gateway.
*   **`Patrol.ino`:** Mengimplementasikan `TaskPatrol`. Task ini berjalan dengan prioritas rendah dan hanya aktif saat mode patroli dipilih. Task ini menjalankan robot dalam pola **Boustrophedon (Ox-Turning)** untuk memastikan cakupan area yang efisien. Selama patroli, task ini secara periodik berhenti untuk mengambil data sensor menggunakan lengan servo dan sensor DHT11.

### Detail Implementasi Gateway Node (`Communication-ESP32`)

Gateway ini berfungsi sebagai penerjemah protokol.
*   **ESP-NOW ke MQTT:** Saat data sensor diterima dari robot, gateway membungkusnya dalam format JSON dan mempublikasikannya ke topik `robot/data` di MQTT Broker.
*   **MQTT ke ESP-NOW:** Gateway berlangganan (subscribe) ke topik `robot/mode` dan `robot/control`. Setiap perintah yang diterima dari Node-RED pada topik ini akan langsung dikemas ke dalam struct `Message` dan dikirimkan ke robot menggunakan ESP-NOW.

## Testing and Evaluation

Untuk memastikan fungsionalitas dan keandalan sistem, serangkaian pengujian harus dilakukan:

1.  **Pengujian Unit (Unit Testing):**
    *   **Motor & Servo:** Menguji setiap fungsi pergerakan (`moveForward`, `stopMotors`, `steeringServo.write()`) secara terisolasi untuk memastikan aktuator bekerja sesuai perintah.
    *   **Sensor:** Menguji pembacaan sensor DHT11 dan pergerakan servo lengan untuk memastikan data yang akurat dan aktuasi yang tepat.

2.  **Pengujian Integrasi (Integration Testing):**
    *   **Komunikasi ESP-NOW:** Mengirim dan menerima data antar dua ESP32 untuk memverifikasi bahwa struct `Message` dapat ditransmisikan dan diterima dengan benar.
    *   **Integrasi FreeRTOS:** Memastikan ketiga task (`Control`, `Patrol`, `Comm`) dapat berjalan secara bersamaan tanpa menimbulkan konflik atau *race condition*, serta verifikasi bahwa pertukaran data melalui *Queue* berjalan lancar.

3.  **Pengujian Sistem (System Testing):**
    *   **Alur Kontrol Manual:** Menguji alur lengkap dari menekan tombol di dashboard Node-RED, pengiriman via MQTT, penerimaan oleh Gateway, pengiriman via ESP-NOW, hingga eksekusi gerakan oleh robot.
    *   **Alur Patroli & Data:** Mengaktifkan mode patroli dan memverifikasi bahwa robot menjalankan pola dengan benar, mengirimkan data sensor, dan data tersebut berhasil ditampilkan di dashboard Node-RED.

4.  **Pengujian Lapangan (Field Testing):**
    *   Mengevaluasi kinerja robot di lingkungan sebenarnya. Kalibrasi durasi gerakan (`LONG_EDGE_DURATION`, `SHORT_EDGE_DURATION`, `TURN_DURATION`) diperlukan agar sesuai dengan kondisi permukaan dan ukuran area patroli.

## Conclusion

Proyek "Smart Agri-Patrol" ini berhasil mengintegrasikan beberapa teknologi inti dalam dunia IoT, termasuk sistem operasi *real-time* (FreeRTOS), komunikasi nirkabel M2M (ESP-NOW), dan platform IoT (MQTT & Node-RED) untuk menciptakan solusi pemantauan pertanian yang fungsional. Arsitektur dual-ESP32 yang modular memisahkan tugas lapangan (robot) dan tugas komunikasi (gateway), menghasilkan sistem yang lebih tangguh dan efisien.

**Potensi Pengembangan di Masa Depan:**
*   **Navigasi Lanjut:** Implementasi SLAM (Simultaneous Localization and Mapping) atau GPS untuk navigasi yang lebih presisi.
*   **Sensor Tambahan:** Menambahkan sensor kelembaban tanah, pH, atau kamera untuk pemantauan yang lebih komprehensif.
*   **Manajemen Daya:** Menambahkan sensor tegangan baterai dan mengirimkan datanya ke dashboard untuk memonitor status daya robot.
*   **Jaringan Mesh:** Mengganti ESP-NOW dengan protokol mesh yang sesungguhnya (misalnya, painlessMesh) untuk memperluas jangkauan dengan beberapa robot atau repeater.

## References
*   **FreeRTOS:** Real-time operating system for microcontrollers. [https://www.freertos.org/](https://www.freertos.org/)
*   **ESP-NOW:** A connectionless Wi-Fi communication protocol by Espressif.
*   **MQTT:** Standard messaging protocol for the Internet of Things (IoT). [https://mqtt.org/](https://mqtt.org/)
*   **Node-RED:** Flow-based development tool for visual programming. [https://nodered.org/](https://nodered.org/)
*   **ArduinoJson:** A C++ JSON library for Arduino and IoT.
*   **PubSubClient:** A client library for MQTT messaging in Arduino.
