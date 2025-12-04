# 📝 Changelog: DHT11 Always Active

## 🔄 Perubahan yang Dilakukan

### File Modified: `Car-ESP32/Patrol.ino`

**Before (Original):**
- DHT11 **hanya dibaca** saat mode **PATROL**
- Mode **MANUAL** = sensor **tidak aktif** (task sleep)
- Dashboard menunggu data saat mode manual

**After (Updated):**
- DHT11 **selalu aktif** di kedua mode
- Mode **PATROL**: Sensor dibaca setiap cycle patrol + servo arm turun-naik
- Mode **MANUAL**: Sensor dibaca setiap **5 detik** tanpa servo arm bergerak
- Dashboard **selalu update** data sensor

---

## 🎯 Behavior Baru:

### Mode PATROL (Tidak Berubah):
```
1. Robot gerak maju
2. Stop
3. Servo arm turun (0° → 90°)
4. Baca DHT11
5. Kirim data ke Gateway → Dashboard
6. Servo arm naik (90° → 0°)
7. Belok kanan/kiri
8. Ulangi cycle
```

### Mode MANUAL (NEW - Sensor tetap aktif):
```
Loop setiap 5 detik:
1. Baca DHT11 (tanpa gerakan servo arm)
2. Kirim data ke Gateway → Dashboard
3. Serial Monitor tampil:
   "Manual Mode - Temp: 28.5°C, Hum: 65.2%"
```

**Keuntungan:**
- User bisa monitoring suhu/kelembaban **realtime** saat kontrol manual
- Dashboard tidak "waiting for data" lagi
- Tidak ganggu kontrol manual (servo arm tidak bergerak)

---

## 🔧 Technical Details:

### Implementasi:
```cpp
// Mode MANUAL: Sensor check setiap 5 detik
TickType_t lastSensorRead = 0;
const TickType_t sensorInterval = pdMS_TO_TICKS(5000);

if (!isPatrolMode) {
  TickType_t now = xTaskGetTickCount();
  if ((now - lastSensorRead) >= sensorInterval) {
    // Baca sensor tanpa servo arm
    sensorData.temp = dht.readTemperature();
    sensorData.hum = dht.readHumidity();
    
    // Kirim ke Queue → Gateway → MQTT → Dashboard
    xQueueSend(sensorQueue, &sensorData, pdMS_TO_TICKS(100));
    
    lastSensorRead = now;
  }
}
```

### Interval Sensor Reading:
- **Mode PATROL**: Setiap cycle (~8-10 detik, tergantung durasi gerak)
- **Mode MANUAL**: Setiap **5 detik** (fixed interval)

---

## 📊 Expected Output:

### ESP32 Robot Serial Monitor:

**Mode PATROL:**
```
Patrol: Sampling...
Patrol: Queued Data
Patrol: Sampling...
Patrol: Queued Data
```

**Mode MANUAL:**
```
Manual Mode - Temp: 28.5°C, Hum: 65.2%
Manual Mode - Temp: 28.6°C, Hum: 65.1%
Manual Mode - Temp: 28.7°C, Hum: 65.3%
```

### ESP32 Gateway Serial Monitor:
```
Data from Robot: Temp=28.5, Hum=65.2
Data from Robot: Temp=28.6, Hum=65.1
Data from Robot: Temp=28.7, Hum=65.3
```

### Node-RED Dashboard:
- Temperature gauge: Update setiap 5 detik
- Humidity gauge: Update setiap 5 detik
- Chart: Garis terus bertambah (tidak stuck)

---

## ✅ Testing Steps:

### Test 1: Mode MANUAL (Sensor Active)
1. Dashboard: Klik **MANUAL**
2. Tunggu **5 detik**
3. **Check**: Gateway Serial Monitor harus tampil data
4. **Check**: Dashboard gauge harus update
5. **Check**: Robot tidak bergerak, servo arm tidak turun

### Test 2: Mode PATROL (Sensor Active + Movement)
1. Dashboard: Klik **PATROL**
2. Robot mulai patrol
3. **Check**: Setiap stop, servo arm turun-naik
4. **Check**: Data sensor terus masuk ke dashboard
5. **Check**: Chart tampil garis kontinu

### Test 3: Switch Mode (Data Persistence)
1. Mode **PATROL** → Data masuk
2. Switch ke **MANUAL** → Data tetap masuk (tidak stop)
3. Switch ke **PATROL** lagi → Data tetap kontinu

---

## 🚀 How to Apply Update:

### Step 1: Upload Updated Code
```
1. Arduino IDE
2. File → Open → Car-ESP32/Car-ESP32.ino
3. Board: ESP32 Dev Module
4. Port: Select ESP32 Robot
5. Upload (→)
```

### Step 2: Verify
```
Serial Monitor (115200 baud):
- Mode PATROL: "Patrol: Sampling..."
- Mode MANUAL: "Manual Mode - Temp: XX.X°C, Hum: XX.X%"
```

### Step 3: Test Dashboard
```
http://localhost:1880/ui
- Klik MANUAL
- Tunggu 5 detik
- Gauge & chart harus update!
```

---

## 📌 Notes:

- **Servo arm hanya bergerak** saat mode **PATROL**
- **Sensor selalu dibaca** di kedua mode
- Interval **5 detik** bisa diubah di line 40: `pdMS_TO_TICKS(5000)`
- Code **backward compatible** (tidak break existing functionality)

---

## 🎯 Summary:

| Feature | Before | After |
|---------|--------|-------|
| Sensor in PATROL | ✅ Active | ✅ Active |
| Sensor in MANUAL | ❌ Inactive | ✅ Active (every 5s) |
| Servo arm in PATROL | ✅ Moves | ✅ Moves |
| Servo arm in MANUAL | - | ✅ Static (no movement) |
| Dashboard updates | Only in PATROL | ✅ Always |

**Result**: Dashboard selalu menampilkan data realtime! 🎉