# Panduan Setup Lengkap: Smart Agri-Patrol Bot

Dokumen ini berisi panduan lengkap untuk merakit hardware, mengatur software, dan mengkonfigurasi proyek "Smart Agri-Patrol Bot".

## Daftar Isi
1.  [Hardware yang Dibutuhkan](#1-hardware-yang-dibutuhkan)
2.  [Rangkaian & Pinout](#2-rangkaian--pinout)
3.  [Setup Software & Library](#3-setup-software--library)
4.  [Konfigurasi & Upload Kode](#4-konfigurasi--upload-kode)
5.  [Setup MQTT Broker](#5-setup-mqtt-broker)
6.  [Setup Dashboard Node-RED](#6-setup-dashboard-node-red)

---

## 1. Hardware yang Dibutuhkan

Berikut adalah komponen yang Anda perlukan untuk membangun proyek ini:
*   **Mikrokontroler:** 2x Papan ESP32 (model apapun, misal: ESP32 DevKitC V4).
*   **Robot:**
    *   1x Robot Chassis (2WD atau 4WD, tutorial ini menggunakan 2WD).
    *   1x Motor Driver (misal: L298N).
    *   2x DC Motor.
    *   1x Roda Caster (bola).
    *   2x Servo Motor (misal: SG90 atau MG90S). Satu untuk kemudi, satu untuk lengan sensor.
    *   1x Sensor Suhu & Kelembaban (DHT11).
*   **Sumber Daya:**
    *   1x Baterai (misal: 2x 18650 Li-Ion dengan holder, atau Power Bank).
*   **Lain-lain:**
    *   Kabel Jumper secukupnya.
    *   Breadboard (opsional, untuk prototyping).

---

## 2. Rangkaian & Pinout

Hubungkan komponen untuk **Node Robot** sesuai dengan tabel dan diagram di bawah ini. Untuk **Node Gateway**, Anda hanya perlu menyambungkannya ke sumber daya via USB.

### Tabel Pinout Node Robot (Car-ESP32)

| Komponen ESP32         | Terhubung ke Komponen Lain            | Keterangan                               |
| ---------------------- | ------------------------------------- | ---------------------------------------- |
| **Pin 25**             | Motor Driver `IN1`                    | Kontrol Motor Kiri/Kanan (Arah 1)        |
| **Pin 26**             | Motor Driver `IN4`                    | Kontrol Motor Kiri/Kanan (Arah 2)        |
| **Pin 15**             | Kabel Sinyal **Servo Kemudi**         | Servo yang mengarahkan roda depan        |
| **Pin 12**             | Kabel Sinyal **Servo Lengan Sensor**  | Servo yang menaikkan/menurunkan sensor   |
| **Pin 4**              | Kabel Data **Sensor DHT11**           |                                          |
| **VIN / 5V**           | `VCC` pada Motor Driver, Servo, DHT11 | Dihubungkan ke output 5V dari sumber daya |
| **GND**                | `GND` pada Motor Driver, Servo, DHT11 | Ground bersama                           |

**Catatan Penting tentang Power:**
*   **JANGAN** memberi daya motor driver (pin `12V`) langsung dari pin `VIN` ESP32. Gunakan sumber daya eksternal (baterai) untuk motor driver.
*   Pastikan **GND** dari ESP32, motor driver, dan sumber daya eksternal saling terhubung untuk menyamakan level tegangan.

---

## 3. Setup Software & Library

1.  **Install Arduino IDE:** Unduh dan install versi terbaru dari [situs resmi Arduino](https://www.arduino.cc/en/software).
2.  **Install Dukungan ESP32:**
    *   Buka Arduino IDE, pergi ke `File > Preferences`.
    *   Di bagian "Additional Board Manager URLs", tambahkan URL berikut:
        ```
        https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
        ```
    *   Buka `Tools > Board > Boards Manager...`, cari "esp32", dan install paket dari Espressif Systems.
3.  **Install Library yang Dibutuhkan:**
    *   Buka `Tools > Manage Libraries...`.
    *   Cari dan install library berikut satu per satu:
        *   `DHT sensor library` oleh **Adafruit**
        *   `PubSubClient` oleh **Nick O'Leary**
        *   `ArduinoJson` oleh **Benoit Blanchon**
        *   `ESP32Servo` (biasanya sudah termasuk, tapi pastikan ada).

---

## 4. Konfigurasi & Upload Kode

Anda perlu mengetahui alamat MAC dari kedua ESP32 Anda. Upload sketch sederhana berikut ke masing-masing ESP32 untuk melihat alamat MAC mereka di Serial Monitor.

```cpp
// Sketch untuk Mengetahui MAC Address
#include "WiFi.h"
void setup(){
  Serial.begin(115200);
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
}
void loop(){}
```
**Catat kedua alamat MAC tersebut!**

### Langkah 1: Konfigurasi & Upload Kode Gateway

1.  Buka folder proyek `Communication-ESP32` di Arduino IDE.
2.  Buka file `Communication.ino`.
3.  **Edit konfigurasi berikut:**
    *   Ganti `WIFI_SSID` dan `WIFI_PASS` dengan kredensial WiFi Anda.
    *   Ganti `MQTT_SERVER` dengan alamat IP dari komputer tempat Anda akan menjalankan MQTT Broker (lihat langkah 5).
    *   Ganti `carAddress` dengan alamat MAC dari **Node Robot** Anda. Contoh: `uint8_t carAddress[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};`
4.  Pilih Board: `Tools > Board > ESP32 Arduino > ESP32 Dev Module`.
5.  Pilih Port yang benar, lalu klik **Upload**.

### Langkah 2: Konfigurasi & Upload Kode Robot

1.  Buka folder proyek `Car-ESP32` di Arduino IDE.
2.  Buka file `Control.ino`.
3.  **Edit konfigurasi berikut:**
    *   Ganti `gatewayAddress` dengan alamat MAC dari **Node Gateway** Anda.
4.  Klik **Upload**.

---

## 5. Setup MQTT Broker

MQTT Broker adalah server pusat yang menangani pengiriman pesan. Untuk penggunaan lokal, kita bisa menggunakan **Mosquitto**.

*   **Untuk Windows:** Unduh dari [situs resmi Mosquitto](https://mosquitto.org/download/). Jalankan installer dan broker akan berjalan sebagai service di background.
*   **Untuk Linux (Debian/Ubuntu):**
    ```bash
    sudo apt-get update
    sudo apt-get install mosquitto mosquitto-clients
    ```
*   **Untuk macOS (via Homebrew):**
    ```bash
    brew install mosquitto
    ```
Setelah terinstall, broker akan berjalan secara otomatis. Untuk mengetahui IP Address komputer Anda (yang akan menjadi `MQTT_SERVER`), gunakan `ifconfig` (Linux/macOS) atau `ipconfig` (Windows).

---

## 6. Setup Dashboard Node-RED

Node-RED adalah platform visual untuk membuat alur kerja IoT.

### Instalasi Node-RED & Node Dashboard

1.  Pastikan Anda memiliki Node.js. Buka terminal/CMD dan jalankan:
    ```bash
    sudo npm install -g --unsafe-perm node-red
    ```
2.  Jalankan Node-RED:
    ```bash
    node-red
    ```
3.  Buka browser dan pergi ke `http://127.0.0.1:1880`.
4.  Di dalam Node-RED, klik menu hamburger (ikon `≡`) di kanan atas, pilih `Manage palette`.
5.  Pergi ke tab `Install`, cari `node-red-dashboard`, dan install.

### Impor Flow Dashboard

Anda bisa membuat dashboard dari nol atau mengimpor flow yang sudah jadi. Salin seluruh kode JSON di bawah ini.

Di dalam Node-RED, pergi ke menu `≡ > Import`, tempel kode JSON, dan klik **Import**.

```json
[
    {
        "id": "da4a1072.05f63",
        "type": "tab",
        "label": "Agri-Patrol Dashboard",
        "disabled": false,
        "info": ""
    },
    {
        "id": "1d5a7d0c.692453",
        "type": "ui_group",
        "name": "Kontrol Robot",
        "tab": "da4a1072.05f63",
        "order": 1,
        "disp": true,
        "width": "6",
        "collapse": false
    },
    {
        "id": "c1f88775.4852d8",
        "type": "ui_group",
        "name": "Data Sensor",
        "tab": "da4a1072.05f63",
        "order": 2,
        "disp": true,
        "width": "6",
        "collapse": false
    },
    {
        "id": "a4b3d2b2.5c865",
        "type": "mqtt in",
        "name": "Data Sensor dari Robot",
        "topic": "robot/data",
        "qos": "2",
        "datatype": "json",
        "broker": "b1a7f0e6.45265",
        "x": 160,
        "y": 420,
        "wires": [
            [
                "e9e90f2b.073e",
                "8c156f71.c2966"
            ]
        ]
    },
    {
        "id": "e9e90f2b.073e",
        "type": "ui_gauge",
        "name": "Suhu",
        "group": "c1f88775.4852d8",
        "order": 1,
        "width": 0,
        "height": 0,
        "gtype": "gage",
        "title": "Suhu",
        "label": "°C",
        "format": "{{value | number:1}}",
        "min": 0,
        "max": "50",
        "colors": [
            "#00b500",
            "#e6e600",
            "#ca3838"
        ],
        "seg1": "25",
        "seg2": "35",
        "x": 410,
        "y": 380,
        "wires": []
    },
    {
        "id": "8c156f71.c2966",
        "type": "ui_gauge",
        "name": "Kelembaban",
        "group": "c1f88775.4852d8",
        "order": 2,
        "width": 0,
        "height": 0,
        "gtype": "gage",
        "title": "Kelembaban",
        "label": "%",
        "format": "{{value | number:1}}",
        "min": 0,
        "max": "100",
        "colors": [
            "#00b500",
            "#e6e600",
            "#ca3838"
        ],
        "seg1": "",
        "seg2": "",
        "x": 430,
        "y": 460,
        "wires": []
    },
    {
        "id": "b1a7f0e6.45265",
        "type": "mqtt-broker",
        "name": "Local Broker",
        "broker": "127.0.0.1",
        "port": "1883",
        "clientid": "",
        "usetls": false,
        "compatmode": true,
        "keepalive": "60",
        "cleansession": true,
        "birthTopic": "",
        "birthQos": "0",
        "birthPayload": "",
        "closeTopic": "",
        "closeQos": "0",
        "closePayload": "",
        "willTopic": "",
        "willQos": "0",
        "willPayload": ""
    },
    {
        "id": "c62a8a2d.8c6b78",
        "type": "ui_switch",
        "name": "Mode Operasi",
        "label": "Mode",
        "tooltip": "",
        "group": "1d5a7d0c.692453",
        "order": 1,
        "width": 0,
        "height": 0,
        "passthru": false,
        "decouple": "false",
        "topic": "robot/mode",
        "style": "",
        "onvalue": "P",
        "onvalueType": "str",
        "onicon": "",
        "oncolor": "",
        "offvalue": "M",
        "offvalueType": "str",
        "officon": "",
        "offcolor": "",
        "x": 130,
        "y": 80,
        "wires": [
            [
                "e5b8a51.e775158"
            ]
        ]
    },
    {
        "id": "e5b8a51.e775158",
        "type": "mqtt out",
        "name": "Kirim Mode",
        "topic": "",
        "qos": "2",
        "retain": "true",
        "broker": "b1a7f0e6.45265",
        "x": 360,
        "y": 80,
        "wires": []
    },
    {
        "id": "f8173499.7d24c8",
        "type": "ui_button",
        "name": "Forward",
        "group": "1d5a7d0c.692453",
        "order": 2,
        "width": 2,
        "height": 1,
        "passthru": false,
        "label": "↑",
        "tooltip": "",
        "color": "",
        "bgcolor": "",
        "icon": "",
        "payload": "F",
        "payloadType": "str",
        "topic": "robot/control",
        "x": 110,
        "y": 140,
        "wires": [
            [
                "f9b7c84c.3f8208"
            ]
        ]
    },
    {
        "id": "f9b7c84c.3f8208",
        "type": "mqtt out",
        "name": "Kirim Kontrol",
        "topic": "",
        "qos": "0",
        "retain": "false",
        "broker": "b1a7f0e6.45265",
        "x": 370,
        "y": 200,
        "wires": []
    },
    {
        "id": "c9b3d07.039233",
        "type": "ui_button",
        "name": "Left",
        "group": "1d5a7d0c.692453",
        "order": 4,
        "width": 2,
        "height": 1,
        "passthru": false,
        "label": "←",
        "tooltip": "",
        "color": "",
        "bgcolor": "",
        "icon": "",
        "payload": "L",
        "payloadType": "str",
        "topic": "robot/control",
        "x": 110,
        "y": 180,
        "wires": [
            [
                "f9b7c84c.3f8208"
            ]
        ]
    },
    {
        "id": "3e9b119b.1491de",
        "type": "ui_button",
        "name": "Stop",
        "group": "1d5a7d0c.692453",
        "order": 5,
        "width": 2,
        "height": 1,
        "passthru": false,
        "label": "STOP",
        "tooltip": "",
        "color": "",
        "bgcolor": "#a80000",
        "icon": "",
        "payload": "S",
        "payloadType": "str",
        "topic": "robot/control",
        "x": 110,
        "y": 220,
        "wires": [
            [
                "f9b7c84c.3f8208"
            ]
        ]
    },
    {
        "id": "bde4446b.d515a8",
        "type": "ui_button",
        "name": "Right",
        "group": "1d5a7d0c.692453",
        "order": 6,
        "width": 2,
        "height": 1,
        "passthru": false,
        "label": "→",
        "tooltip": "",
        "color": "",
        "bgcolor": "",
        "icon": "",
        "payload": "R",
        "payloadType": "str",
        "topic": "robot/control",
        "x": 110,
        "y": 260,
        "wires": [
            [
                "f9b7c84c.3f8208"
            ]
        ]
    },
    {
        "id": "c62f2757.f52e28",
        "type": "ui_button",
        "name": "Backward",
        "group": "1d5a7d0c.692453",
        "order": 7,
        "width": 2,
        "height": 1,
        "passthru": false,
        "label": "↓",
        "tooltip": "",
        "color": "",
        "bgcolor": "",
        "icon": "",
        "payload": "B",
        "payloadType": "str",
        "topic": "robot/control",
        "x": 120,
        "y": 300,
        "wires": [
            [
                "f9bT7c84c.3f8208"
            ]
        ]
    }
]
```

Setelah mengimpor, klik tombol **Deploy** di kanan atas. Buka tab baru di browser dan navigasi ke `http://127.0.0.1:1880/ui` untuk melihat dan menggunakan dashboard Anda. Pastikan MQTT Broker Anda berjalan dan Gateway ESP32 sudah terhubung ke WiFi dan MQTT.

Selesai! Anda kini memiliki sistem patroli robot yang berfungsi penuh.
