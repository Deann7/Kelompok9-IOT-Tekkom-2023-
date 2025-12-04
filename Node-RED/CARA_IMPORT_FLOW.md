# 📥 Cara Import Flow Node-RED (Update Credentials)

## ✅ Flow Sudah Terupdate dengan Credentials Baru:
- **Server**: `89288a44b913422cb31b59472927642a.s1.eu.hivemq.cloud`
- **Port**: `8883`
- **Username**: `kelompok9_terbaru123`
- **Password**: `AgriPatro123l123!`

---

## 🚀 Step-by-Step Import

### STEP 1: Jalankan Node-RED
```bash
# Buka Command Prompt (Win + R → cmd → Enter)
node-red
```

Tunggu sampai muncul:
```
Server now running at http://127.0.0.1:1880/
```

---

### STEP 2: Buka Node-RED Editor
- Browser: **http://localhost:1880**

---

### STEP 3: Hapus Flow Lama (Jika Ada)
1. Klik tab flow yang ada (misalnya "Flow 1")
2. Klik menu **☰** (pojok kanan atas) → **Flows** → **Delete**
3. Konfirmasi: **Delete**

---

### STEP 4: Import Flow Baru

#### Cara 1: Import via File (Recommended)
1. Klik menu **☰** (pojok kanan atas)
2. Pilih **Import**
3. Klik tab **"select a file to import"**
4. Browse ke: `Node-RED/flow-dashboard.json`
5. Klik **Open**
6. Klik **Import**

#### Cara 2: Import via Clipboard
1. Buka file `Node-RED/flow-dashboard.json` dengan Notepad
2. Copy **SEMUA** isi file (Ctrl+A → Ctrl+C)
3. Di Node-RED Editor:
   - Menu **☰** → **Import**
   - Paste di kotak text (Ctrl+V)
   - Klik **Import**

---

### STEP 5: Verifikasi Import
Setelah import, harus muncul:
- **Tab baru**: "Smart Agri-Patrol Dashboard"
- **Node MQTT** berwarna biru/hijau (connected)
- **Tidak ada node merah** (error)

---

### STEP 6: Deploy
1. Klik tombol merah **"Deploy"** (pojok kanan atas)
2. Tunggu sampai muncul: "Successfully deployed"

---

### STEP 7: Buka Dashboard
- Browser: **http://localhost:1880/ui**
- Harus tampil:
  - ✅ Tombol **MANUAL** dan **PATROL**
  - ✅ Control buttons (↑ ↓ ← → ⏹)
  - ✅ Gauge Temperature & Humidity
  - ✅ Chart (dengan text "Waiting for data...")

---

## ✅ Verifikasi MQTT Connection

### Check di Node-RED Editor:
1. Lihat node **"🤖 Robot Sensor Data"** (biru)
2. Harus ada tanda **hijau** di bawah node (connected)
3. Jika ada **tanda merah** (disconnected):
   - Double-click node
   - Check server settings
   - Pastikan credentials benar

### Check via Debug:
1. Klik tab **"Debug"** (icon bug 🐛) di panel kanan
2. Klik tombol **PATROL** di dashboard
3. Harus muncul di debug panel:
   ```
   robot/mode : msg.payload : string[1]
   "P"
   ```

---

## 🎯 Test Lengkap

### Test 1: Mode Control
1. Dashboard: http://localhost:1880/ui
2. Klik **MANUAL** → Check Gateway Serial Monitor:
   ```
   MQTT Msg on [robot/mode]: M
   >> Command forwarded to Robot
   ```

### Test 2: Robot Movement
1. Pastikan mode **MANUAL** aktif
2. Klik **↑** (Forward) → Robot harus maju
3. Klik **⏹** (Stop) → Robot harus stop

### Test 3: Patrol & Sensor Data
1. Klik **PATROL**
2. Robot mulai patrol (gerak kotak)
3. Gateway Serial Monitor:
   ```
   Data from Robot: Temp=28.5, Hum=65.2
   ```
4. Dashboard:
   - Temperature gauge update
   - Humidity gauge update
   - Chart tampil garis grafik

---

## 🐛 Troubleshooting

### ❌ Node MQTT warna merah (disconnected)
**Solution:**
1. Double-click node MQTT
2. Click icon **pensil** di sebelah Server
3. Verifikasi:
   ```
   Server: 89288a44b913422cb31b59472927642a.s1.eu.hivemq.cloud
   Port: 8883
   Use TLS: ✅ (checked)
   Username: kelompok9_terbaru123
   Password: AgriPatro123l123!
   ```
4. Click **Update** → **Done** → **Deploy**

### ❌ Dashboard tidak tampil
**Check:**
- Node-RED Dashboard sudah terinstall?
  ```bash
  npm install node-red-dashboard
  ```
- Restart Node-RED setelah install

### ❌ Data tidak muncul di chart
**Check:**
1. Robot sudah dalam mode **PATROL**?
2. Gateway Serial Monitor tampil data?
3. Node-RED Debug panel tampil data JSON?

---

## 📋 Summary Checklist

- [ ] Node-RED running (port 1880)
- [ ] Flow di-import dari `flow-dashboard.json`
- [ ] Deploy sukses (no error)
- [ ] Dashboard accessible (http://localhost:1880/ui)
- [ ] MQTT nodes connected (tanda hijau)
- [ ] ESP32 Gateway connected MQTT
- [ ] ESP32 Robot running
- [ ] Test mode MANUAL → Robot respond
- [ ] Test mode PATROL → Data muncul di dashboard

Jika semua ✅, sistem ready to use! 🚀