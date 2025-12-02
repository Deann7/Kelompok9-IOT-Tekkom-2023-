# Tutorial Setup Smart Agri-Patrol Bot

Panduan lengkap untuk mengatur dan menjalankan proyek Smart Agri-Patrol Bot dari awal hingga dashboard berjalan.

---

## 📋 Daftar Isi

1. [Persiapan Hardware](#1-persiapan-hardware)
2. [Instalasi Software](#2-instalasi-software)
3. [Setup ESP32 Robot](#3-setup-esp32-robot)
4. [Setup ESP32 Gateway](#4-setup-esp32-gateway)
5. [Setup Node-RED Dashboard](#5-setup-node-red-dashboard)
6. [Testing & Troubleshooting](#6-testing--troubleshooting)

---

## 1. Persiapan Hardware

### Komponen yang Dibutuhkan

**Node Robot (Mobile Unit):**
- 1x ESP32 Development Board
- 2x DC Motor + Driver Motor (L298N/L293D)
- 2x Servo Motor (SG90 atau sejenisnya)
  - 1x untuk kemudi (steering)
  - 1x untuk lengan sensor
- 1x Sensor DHT11 (Suhu & Kelembaban)
- 1x Baterai 3.7V - 7.4V (LiPo/Li-ion)
- Kabel jumper
- Chasis robot/mobil

**Node Gateway (Base Station):**
- 1x ESP32 Development Board
- 1x Kabel USB untuk power/programming

**Koneksi Pin ESP32 Robot:**
```
Motor Driver:
- IN1 → GPIO 25
- IN4 → GPIO 26

Servo:
- Steering Servo → GPIO 15
- Sensor Arm Servo → GPIO 12

Sensor:
- DHT11 → GPIO 4
```

---

## 2. Instalasi Software

### 2.1 Install Arduino IDE

1. Download Arduino IDE dari [https://www.arduino.cc/en/software](https://www.arduino.cc/en/software)
2. Install sesuai sistem operasi Anda

### 2.2 Setup ESP32 di Arduino IDE

1. Buka Arduino IDE
2. Pergi ke **File → Preferences**
3. Pada "Additional Board Manager URLs", tambahkan:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. Pergi ke **Tools → Board → Boards Manager**
5. Cari "ESP32" dan install **ESP32 by Espressif Systems**

### 2.3 Install Library yang Diperlukan

Pergi ke **Sketch → Include Library → Manage Libraries**, lalu install:

**Untuk Node Robot:**
- `ESP32Servo` by Kevin Harrington
- `DHT sensor library` by Adafruit
- `Adafruit Unified Sensor` (dependency DHT)

**Untuk Node Gateway:**
- `PubSubClient` by Nick O'Leary
- `ArduinoJson` by Benoit Blanchon

### 2.4 Install Node-RED

**Cara 1: Install via NPM (Recommended)**
```bash
# Install Node.js terlebih dahulu dari https://nodejs.org/
# Kemudian install Node-RED
npm install -g --unsafe-perm node-red
```

**Cara 2: Standalone Installer**
- Download dari [https://nodered.org/docs/getting-started/](https://nodered.org/docs/getting-started/)

**Install Node-RED Dashboard:**
```bash
cd ~/.node-red
npm install node-red-dashboard
```

---

## 3. Setup ESP32 Robot

### 3.1 Dapatkan MAC Address ESP32 Robot

Upload sketch berikut untuk mendapatkan MAC Address:

```cpp
#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
}

void loop() {}
```

**Catat MAC Address yang muncul di Serial Monitor!**
Contoh: `AA:BB:CC:DD:EE:FF`

### 3.2 Upload Kode Robot

1. Buka folder `Car-ESP32/Control.ino`
2. **PENTING**: Edit MAC Address Gateway di baris 29:
   ```cpp
   uint8_t gatewayAddress[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
   ```
   Ganti dengan MAC Address ESP32 Gateway Anda (akan didapat di langkah 4.1)

3. **Kalibrasi Pin** jika berbeda dengan konfigurasi default
4. Select Board: **ESP32 Dev Module**
5. Select Port sesuai ESP32 Robot
6. Upload!

### 3.3 Verifikasi Upload

Buka Serial Monitor (115200 baud), Anda harus melihat:
```
Setup complete. All tasks running.
```

---

## 4. Setup ESP32 Gateway

### 4.1 Dapatkan MAC Address ESP32 Gateway

Ulangi langkah 3.1 untuk ESP32 kedua (Gateway). **Catat MAC Address ini!**

### 4.2 Konfigurasi Gateway

1. Buka `Communication-ESP32/Communication.ino`
2. **Edit Konfigurasi WiFi** (baris 22-23):
   ```cpp
   #define WIFI_SSID "Nama_WiFi_Anda"
   #define WIFI_PASS "Password_WiFi_Anda"
   ```

3. **Edit MAC Address Robot** (baris 36):
   ```cpp
   uint8_t carAddress[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
   ```
   Ganti dengan MAC Address ESP32 Robot dari langkah 3.1

4. **MQTT Broker sudah dikonfigurasi** menggunakan EMQX Public Broker:
   ```cpp
   #define MQTT_SERVER "broker.emqx.io"
   ```

5. Upload ke ESP32 Gateway

### 4.3 Verifikasi Koneksi

Serial Monitor harus menampilkan:
```
Gateway ESP32 Starting...
Connecting to [WiFi_Name]...
WiFi connected!
IP address: 192.168.x.x
ESP-NOW initialized, peer added.
MQTT configured.
Attempting MQTT connection...connected
Subscribed to: robot/control and robot/mode
```

---

## 5. Setup Node-RED Dashboard

### 5.1 Import Flow Dashboard

1. **Jalankan Node-RED:**
   ```bash
   node-red
   ```

2. Buka browser, akses: **http://localhost:1880**

3. Import flow:
   - Klik menu (☰) → **Import**
   - Pilih **select a file to import**
   - Browse ke `Node-RED/flow-dashboard.json`
   - Klik **Import**

4. **Deploy** flow dengan klik tombol merah **Deploy** di kanan atas

### 5.2 Akses Dashboard

Buka browser baru dan akses:
```
http://localhost:1880/ui
```

Anda akan melihat dashboard dengan:
- **Robot Control Panel**: Switch mode + tombol kontrol (↑ ← STOP → ↓)
- **Sensor Data Panel**: Gauge dan chart untuk suhu & kelembaban

### 5.3 Struktur Dashboard

**Panel Kontrol:**
- **Patrol Mode Switch**: Toggle antara Manual (OFF) dan Patrol (ON)
- **Forward (↑)**: Maju
- **Left (←)**: Belok kiri
- **Right (→)**: Belok kanan  
- **Backward (↓)**: Mundur
- **STOP**: Stop darurat

**Panel Sensor:**
- **Temperature Gauge**: Real-time suhu (0-50°C)
- **Humidity Gauge**: Real-time kelembaban (0-100%)
- **Temperature Chart**: Grafik historis 10 menit
- **Humidity Chart**: Grafik historis 10 menit

---

## 6. Testing & Troubleshooting

### 6.1 Test Komunikasi ESP-NOW

**Test 1: Gateway → Robot**
1. Pastikan kedua ESP32 powered on
2. Di Node-RED Dashboard, klik tombol kontrol
3. Serial Monitor Robot harus menampilkan:
   ```
   Mode switched to: MANUAL
   ```

**Test 2: Robot → Gateway**
1. Aktifkan mode Patrol di dashboard
2. Gateway Serial Monitor harus menampilkan:
   ```
   Sensor data received from Robot: Temp=28.5C, Hum=65.2%
   Published to MQTT topic 'robot/data': {"temperature":28.5,"humidity":65.2}
   ```

### 6.2 Troubleshooting Umum

#### ❌ **Problem: Robot tidak merespon perintah**

**Solusi:**
1. Cek MAC Address di kedua ESP32 sudah benar
2. Pastikan kedua ESP32 menggunakan WiFi channel yang sama (default: 0)
3. Cek Serial Monitor untuk error ESP-NOW
4. Pastikan jarak antar ESP32 < 100m (ideal < 20m)

#### ❌ **Problem: Gateway tidak connect ke WiFi**

**Solusi:**
1. Cek SSID dan Password WiFi
2. Pastikan WiFi 2.4GHz (ESP32 tidak support 5GHz)
3. Restart ESP32 Gateway

#### ❌ **Problem: Gateway tidak connect ke MQTT**

**Solusi:**
1. Cek koneksi internet (EMQX adalah public broker online)
2. Test koneksi manual:
   ```bash
   ping broker.emqx.io
   ```
3. Jika masih gagal, coba broker alternatif:
   - `test.mosquitto.org`
   - `mqtt.eclipseprojects.io`

#### ❌ **Problem: Dashboard tidak menerima data**

**Solusi:**
1. Cek Node-RED debug panel (klik 🐞 di kanan)
2. Pastikan MQTT broker connected (indikator hijau di node MQTT)
3. Test manual publish via MQTT client:
   ```bash
   # Install mosquitto-clients
   mosquitto_pub -h broker.emqx.io -t robot/data -m '{"temperature":25,"humidity":60}'
   ```

#### ❌ **Problem: Motor tidak bergerak**

**Solusi:**
1. Cek koneksi pin motor driver
2. Cek power supply baterai (minimal 3.7V)
3. Test motor secara manual:
   ```cpp
   digitalWrite(IN1, HIGH);
   digitalWrite(IN4, LOW);
   delay(2000);
   ```

#### ❌ **Problem: Servo tidak bergerak**

**Solusi:**
1. Cek koneksi pin servo
2. Pastikan servo mendapat power yang cukup (5V)
3. Test servo manual:
   ```cpp
   servo.write(90);
   ```

#### ❌ **Problem: DHT11 return NaN**

**Solusi:**
1. Cek koneksi pin DHT11
2. Tambahkan delay setelah `dht.begin()`:
   ```cpp
   dht.begin();
   delay(2000); // Tunggu sensor ready
   ```
3. Pastikan sensor tidak rusak (test dengan multimeter)

### 6.3 Kalibrasi Robot

**Kalibrasi Durasi Gerakan** di `Patrol.ino`:

```cpp
#define LONG_EDGE_DURATION 3000  // Sesuaikan dengan kecepatan motor
#define SHORT_EDGE_DURATION 1000 // Sesuaikan dengan ukuran area
#define TURN_DURATION 1200       // Kalibrasi untuk belok 90°
```

**Cara Kalibrasi:**
1. Set robot di mode manual
2. Test berapa lama motor harus menyala untuk jarak 1 meter
3. Test berapa lama untuk belok 90° tepat
4. Update nilai konstanta di kode

**Kalibrasi Sudut Servo** di `Control.ino`:

```cpp
if (msg.cmd == 'L') {
  steerAngle = 60;  // Ubah nilai ini untuk belok lebih tajam/landai
} else if (msg.cmd == 'R') {
  steerAngle = 120; // Ubah nilai ini untuk belok lebih tajam/landai
}
```

---

## 📝 Catatan Penting

### Keamanan Data dengan Public Broker

⚠️ **EMQX Public Broker tidak terenkripsi!** Siapapun bisa subscribe ke topik Anda jika mengetahui nama topiknya.

**Solusi untuk Production:**
1. Gunakan topik dengan prefix unik:
   ```cpp
   #define MQTT_PUB_TOPIC "agribot/USER123/data"
   ```

2. Install Mosquitto lokal untuk data lebih aman
3. Gunakan MQTT over TLS (port 8883) dengan sertifikat

### Power Management

**Tips Hemat Baterai:**
1. Gunakan mode sleep saat tidak digunakan
2. Kurangi polling rate sensor (delay lebih lama)
3. Matikan LED built-in ESP32
4. Gunakan voltage regulator yang efisien

### Pengembangan Lanjut

**Fitur yang Bisa Ditambahkan:**
1. **Battery Monitor**: Kirim status baterai ke dashboard
2. **GPS Module**: Tracking posisi robot real-time
3. **Camera**: Streaming video untuk monitoring visual
4. **Obstacle Avoidance**: Sensor ultrasonik untuk hindari halangan
5. **Multi-Robot**: Tambah lebih banyak robot dengan topik berbeda
6. **Database**: Simpan data sensor ke InfluxDB/MySQL
7. **Alert System**: Notifikasi telegram jika suhu/kelembaban abnormal

---

## 📚 Referensi

- **FreeRTOS Documentation**: [https://www.freertos.org/](https://www.freertos.org/)
- **ESP-NOW Protocol**: [https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_now.html](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_now.html)
- **MQTT Protocol**: [https://mqtt.org/](https://mqtt.org/)
- **Node-RED Guide**: [https://nodered.org/docs/](https://nodered.org/docs/)
- **PubSubClient Library**: [https://pubsubclient.knolleary.net/](https://pubsubclient.knolleary.net/)

---

## 🆘 Butuh Bantuan?

Jika mengalami masalah:
1. Cek Serial Monitor untuk error messages
2. Cek Node-RED debug panel
3. Pastikan semua library ter-install dengan benar
4. Review koneksi hardware (jumper, power supply)
5. Coba restart semua komponen (ESP32, Node-RED, router)

**Happy Building! 🚀**