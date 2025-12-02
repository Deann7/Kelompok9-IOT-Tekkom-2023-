# 🧪 Panduan Testing Smart Agri-Patrol Bot

Panduan lengkap untuk melakukan testing sistem dari dashboard hingga robot.

---

## 📋 Checklist Sebelum Testing

Pastikan hal-hal berikut sudah siap:

- [ ] Node-RED sudah running (`node-red`)
- [ ] Dashboard bisa diakses (`http://localhost:1880/ui`)
- [ ] EMQX broker terkoneksi (cek log: `Connected to broker: mqtt://broker.emqx.io`)
- [ ] ESP32 Gateway sudah di-upload dan powered on
- [ ] ESP32 Robot sudah di-upload dan powered on
- [ ] Hardware robot sudah terhubung (motor, servo, sensor)
- [ ] Baterai robot terisi

---

## 🎯 Testing Level 1: Dashboard Saja (Tanpa ESP32)

**Tujuan:** Memastikan Node-RED dan MQTT berfungsi dengan baik.

### Test 1.1: Cek Dashboard UI

1. Buka browser: `http://localhost:1880/ui`
2. Pastikan muncul dashboard dengan:
   - ✅ Panel "Robot Control" dengan switch dan 5 tombol
   - ✅ Panel "Sensor Data" dengan 2 gauge dan 2 chart

**✅ Berhasil jika:** Dashboard tampil sempurna tanpa error.

### Test 1.2: Test MQTT Publish Manual

1. **Buka Node-RED Editor**: `http://localhost:1880`
2. **Aktifkan Debug Panel**: Klik ikon 🐞 di kanan
3. **Test publish data sensor manual**:
   - Drag node **inject** ke canvas
   - Double-click node inject
   - Set payload ke JSON:
     ```json
     {"temperature": 28.5, "humidity": 65}
     ```
   - Set topic: `robot/data`
   - Connect ke node MQTT out yang sudah ada
   - Deploy
   - Klik tombol di node inject

4. **Cek Dashboard**: `http://localhost:1880/ui`
   - Gauge Temperature harus menunjukkan 28.5°C
   - Gauge Humidity harus menunjukkan 65%
   - Chart mulai menampilkan data

**✅ Berhasil jika:** Data muncul di dashboard.

### Test 1.3: Test Tombol Dashboard

1. **Buka Debug Panel** di Node-RED editor
2. **Klik tombol** di dashboard (Forward, Left, Right, dll)
3. **Cek Debug Panel** - harus muncul pesan:
   ```
   robot/control : msg.payload : string[1]
   "F"
   ```

**✅ Berhasil jika:** Setiap tombol mengirim karakter yang benar (F/B/L/R/S/P/M).

---

## 🎯 Testing Level 2: Dashboard + ESP32 Gateway (Tanpa Robot)

**Tujuan:** Memastikan Gateway bisa komunikasi dengan Node-RED.

### Test 2.1: Cek Koneksi Gateway

1. **Upload kode ke ESP32 Gateway** (`Communication-ESP32/Communication.ino`)
2. **Buka Serial Monitor** (115200 baud)
3. **Pastikan muncul:**
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

**✅ Berhasil jika:** Gateway connect ke WiFi dan MQTT.

### Test 2.2: Test Gateway Menerima Perintah dari Dashboard

1. **Tetap buka Serial Monitor Gateway**
2. **Buka Dashboard**: `http://localhost:1880/ui`
3. **Klik tombol Forward**
4. **Cek Serial Monitor Gateway** - harus muncul:
   ```
   Message arrived on topic: robot/control. Message: F
   Sent command 'F' to Robot.
   ```

**✅ Berhasil jika:** Gateway menerima perintah dari MQTT dan log muncul.

**❌ Jika gagal:**
- Cek koneksi WiFi Gateway
- Cek MQTT broker di kode (`broker.emqx.io`)
- Restart Gateway dan coba lagi

### Test 2.3: Test Gateway Publish Data Manual

1. **Simulasi ESP-NOW dari Robot** (karena robot belum ada)
2. **Edit kode Gateway sementara** - tambahkan di `loop()`:
   ```cpp
   // TEST CODE - hapus setelah testing
   static unsigned long lastTest = 0;
   if (millis() - lastTest > 5000) { // Setiap 5 detik
     Message testMsg;
     testMsg.type = 'S';
     testMsg.temp = 27.5;
     testMsg.hum = 60.0;
     
     StaticJsonDocument<128> doc;
     doc["temperature"] = testMsg.temp;
     doc["humidity"] = testMsg.hum;
     char buffer[128];
     serializeJson(doc, buffer);
     client.publish(MQTT_PUB_TOPIC, buffer);
     Serial.println("Test data published to MQTT");
     
     lastTest = millis();
   }
   ```
3. **Upload ulang** ke Gateway
4. **Cek Dashboard** - setiap 5 detik harus muncul data baru

**✅ Berhasil jika:** Dashboard update setiap 5 detik dengan data test.

**Jangan lupa hapus kode test setelah selesai!**

---

## 🎯 Testing Level 3: Dashboard + Gateway + Robot (Full System)

**Tujuan:** Memastikan komunikasi end-to-end berfungsi.

### Persiapan Robot

1. **Dapatkan MAC Address kedua ESP32**:
   ```cpp
   #include <WiFi.h>
   void setup() {
     Serial.begin(115200);
     WiFi.mode(WIFI_STA);
     Serial.println(WiFi.macAddress());
   }
   void loop() {}
   ```

2. **Update konfigurasi:**
   - Gateway: Ganti `carAddress[]` dengan MAC Robot
   - Robot: Ganti `gatewayAddress[]` dengan MAC Gateway

3. **Upload kode ke kedua ESP32**

### Test 3.1: Cek Komunikasi ESP-NOW

1. **Buka 2 Serial Monitor** (satu untuk Gateway, satu untuk Robot)
2. **Power on kedua ESP32**
3. **Klik tombol di Dashboard**

**Serial Monitor Gateway harus menunjukkan:**
```
Message arrived on topic: robot/control. Message: F
Sent command 'F' to Robot.
ESP-NOW Send Status: Success
```

**Serial Monitor Robot harus menunjukkan:**
```
Mode switched to: MANUAL
```

**✅ Berhasil jika:** Kedua ESP32 saling komunikasi.

**❌ Jika gagal:**
- Cek MAC address sudah benar
- Pastikan kedua ESP32 dalam jarak < 20 meter
- Restart kedua ESP32
- Cek ESP-NOW initialization di Serial Monitor

### Test 3.2: Test Kontrol Manual Robot

**PERHATIAN:** Pastikan robot di permukaan yang aman, siap untuk bergerak!

1. **Set Mode Manual** (switch di dashboard OFF)
2. **Test setiap tombol:**

| Tombol | Perintah | Hasil yang Diharapkan |
|--------|----------|----------------------|
| ↑ Forward | F | Motor maju |
| ← Left | L | Servo belok kiri (60°) |
| → Right | R | Servo belok kanan (120°) |
| ↓ Backward | B | Motor mundur |
| STOP | S | Motor stop, servo lurus (90°) |

**✅ Berhasil jika:** Robot merespon semua perintah dengan benar.

**❌ Troubleshooting:**

**Motor tidak bergerak:**
- Cek koneksi pin IN1, IN4
- Cek power supply motor (baterai)
- Test manual dengan Serial Monitor:
  ```cpp
  digitalWrite(IN1, HIGH);
  digitalWrite(IN4, LOW);
  ```

**Servo tidak bergerak:**
- Cek koneksi pin servo
- Cek power supply servo (5V)
- Test manual: `steeringServo.write(90);`

**Robot bergerak tapi lambat:**
- Baterai lemah, charge ulang
- Motor kotor, bersihkan

### Test 3.3: Test Mode Patrol Otomatis

1. **Set robot di area yang aman** (minimal 2m x 1m)
2. **Aktifkan Patrol Mode** (switch di dashboard ON)
3. **Amati robot:**
   - Robot harus bergerak dalam pola kotak (Boustrophedon)
   - Berhenti secara periodik
   - Servo lengan turun-naik saat sampling

**Serial Monitor Robot harus menunjukkan:**
```
Mode switched to: PATROL
Patrol: Moving forward (long edge)...
Patrol: Stopping for sensor reading...
Patrol: Sensor data sent (T:28.5 H:65.2)
Patrol: Turning right...
...
```

**Serial Monitor Gateway harus menunjukkan:**
```
Sensor data received from Robot: Temp=28.5C, Hum=65.2%
Published to MQTT topic 'robot/data': {"temperature":28.5,"humidity":65.2}
```

**Dashboard harus menunjukkan:**
- Gauge dan chart update secara real-time
- Data baru setiap kali robot sampling

**✅ Berhasil jika:** Robot patroli otomatis dan data muncul di dashboard.

### Test 3.4: Test Switch Mode Real-time

1. **Saat robot sedang patroli**, aktifkan Mode Manual (switch OFF)
2. **Robot harus:**
   - Stop langsung
   - Servo lurus
   - Menunggu perintah manual

3. **Klik tombol Forward** - robot harus bergerak maju

**✅ Berhasil jika:** Bisa switch mode kapan saja tanpa restart.

---

## 🎯 Testing Level 4: Stress Testing & Kalibrasi

### Test 4.1: Kalibrasi Durasi Gerakan

**Tujuan:** Mendapatkan nilai yang tepat untuk `LONG_EDGE_DURATION`, `SHORT_EDGE_DURATION`, dan `TURN_DURATION`.

1. **Set mode Manual**
2. **Test jarak 1 meter:**
   - Tandai start dan finish (1 meter)
   - Klik Forward
   - Hitung berapa detik sampai finish
   - Update `LONG_EDGE_DURATION` di kode

3. **Test belok 90°:**
   - Tandai orientasi awal (misal: menghadap utara)
   - Klik Right sambil Forward
   - Hitung berapa detik untuk belok tepat 90°
   - Update `TURN_DURATION` di kode

4. **Upload ulang** dan test mode Patrol - sesuaikan sampai pola kotak sempurna

### Test 4.2: Endurance Test

1. **Charge baterai penuh**
2. **Aktifkan mode Patrol**
3. **Biarkan berjalan 10-15 menit**
4. **Amati:**
   - Apakah robot tetap konsisten?
   - Berapa lama baterai bertahan?
   - Apakah data sensor tetap akurat?

### Test 4.3: Range Test ESP-NOW

1. **Letakkan Gateway di posisi tetap**
2. **Jalankan robot menjauhi Gateway**
3. **Cek pada jarak berapa mulai disconnect:**
   - Indoor: biasanya 20-30 meter
   - Outdoor: bisa sampai 100 meter (line of sight)

**Tips:** Jika jarak kurang, pindahkan Gateway lebih dekat ke area patrol.

---

## 📊 Hasil Testing yang Diharapkan

### ✅ Checklist Keberhasilan Full System:

- [ ] Dashboard menampilkan kontrol dan data dengan benar
- [ ] EMQX broker terkoneksi stabil
- [ ] Gateway menerima dan meneruskan perintah dari Dashboard
- [ ] Robot menerima perintah dari Gateway via ESP-NOW
- [ ] Motor dan servo merespon perintah dengan benar
- [ ] Mode Manual berfungsi (5 tombol kontrol)
- [ ] Mode Patrol berfungsi (pola Boustrophedon)
- [ ] Sensor DHT11 membaca data valid
- [ ] Data sensor ditampilkan di Dashboard real-time
- [ ] Switch mode berfungsi tanpa restart
- [ ] Sistem stabil untuk operasi > 10 menit

---

## 🐛 Common Issues & Solutions

### Issue 1: Dashboard tidak muncul
**Solusi:**
```bash
# Install node-red-dashboard
cd ~/.node-red
npm install node-red-dashboard
# Restart Node-RED
```

### Issue 2: Robot tidak menerima perintah
**Solusi:**
1. Cek MAC address di kedua ESP32
2. Cek jarak antar ESP32 (< 20m)
3. Cek Serial Monitor untuk error ESP-NOW
4. Restart kedua ESP32

### Issue 3: Sensor return NaN
**Solusi:**
```cpp
// Tambahkan delay di setup()
dht.begin();
delay(2000); // Tunggu sensor ready
```

### Issue 4: Motor lemah/tidak bergerak
**Solusi:**
1. Cek voltase baterai (min 3.7V)
2. Cek koneksi motor driver
3. Cek ground common antara ESP32 dan motor driver

### Issue 5: Data tidak muncul di Dashboard
**Solusi:**
1. Cek MQTT connection di Node-RED (harus hijau)
2. Cek debug panel untuk error
3. Test publish manual dari Node-RED
4. Cek topik MQTT (`robot/data` vs `robot/control`)

---

## 📝 Log Template untuk Testing

Gunakan template ini untuk mendokumentasikan hasil testing:

```
==========================================
TESTING LOG - Smart Agri-Patrol Bot
Tanggal: ___________
Tester: ___________
==========================================

LEVEL 1: Dashboard Only
[✅/❌] Dashboard tampil
[✅/❌] Manual inject data berhasil
[✅/❌] Tombol dashboard berfungsi

LEVEL 2: Dashboard + Gateway
[✅/❌] Gateway connect WiFi
[✅/❌] Gateway connect MQTT
[✅/❌] Gateway terima perintah dari Dashboard
[✅/❌] Gateway publish data ke MQTT

LEVEL 3: Full System
[✅/❌] ESP-NOW komunikasi berhasil
[✅/❌] Kontrol manual robot berfungsi
[✅/❌] Mode patrol berfungsi
[✅/❌] Data sensor muncul di Dashboard
[✅/❌] Switch mode real-time berfungsi

LEVEL 4: Kalibrasi
LONG_EDGE_DURATION: _____ ms
SHORT_EDGE_DURATION: _____ ms
TURN_DURATION: _____ ms
Battery life: _____ minutes
ESP-NOW range: _____ meters

CATATAN:
_______________________________________
_______________________________________
_______________________________________
==========================================
```

---

## 🎉 Selamat Testing!

Jika semua test berhasil, sistem Anda siap untuk deployment di lapangan!

**Next Steps:**
1. Dokumentasikan nilai kalibrasi
2. Buat backup kode
3. Siapkan baterai cadangan
4. Test di area lapangan sesungguhnya

**Happy Testing! 🚀**