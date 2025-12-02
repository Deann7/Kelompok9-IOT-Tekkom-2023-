# Cara Menghapus Error "Local Broker Connection Failed"

Error ini muncul karena Node-RED Anda masih memiliki konfigurasi broker MQTT lokal yang tidak terpakai.

## ✅ Solusi: Hapus Node MQTT Local Broker

Ikuti langkah berikut:

### Langkah 1: Buka Node-RED Editor
```
http://localhost:1880
```

### Langkah 2: Buka Configuration Nodes

1. Klik menu hamburger (☰) di kanan atas
2. Pilih **"Configuration nodes"**
3. Anda akan melihat daftar semua broker MQTT

### Langkah 3: Hapus "Local Broker"

1. Cari broker dengan nama **"Local Broker"** yang addressnya `127.0.0.1:1883`
2. Klik pada nama broker tersebut
3. Klik tombol **"Delete"** (ikon tempat sampah)
4. Konfirmasi penghapusan

### Langkah 4: Deploy Ulang

1. Klik tombol **"Deploy"** di kanan atas
2. Error akan hilang!

---

## 🎯 Verifikasi

Setelah deploy, log Node-RED seharusnya hanya menampilkan:

```
✅ [info] [mqtt-broker:EMQX Public Broker] Connected to broker: mqtt://broker.emqx.io:1883
```

**TIDAK ADA lagi error:**
```
❌ [info] [mqtt-broker:Local Broker] Connection failed to broker: mqtt://127.0.0.1:1883
```

---

## 📝 Catatan

- Error ini **TIDAK BERBAHAYA** - hanya informational
- Broker EMQX sudah terkoneksi dengan baik (`Connected to broker`)
- Local broker memang tidak ada karena kita tidak install Mosquitto lokal
- Flow dashboard sudah menggunakan EMQX broker, bukan local broker
- Menghapus local broker **TIDAK AKAN** mempengaruhi fungsi dashboard

---

## 🆘 Jika Masih Ada Error

Jika setelah dihapus masih muncul error, berarti ada node lain yang masih menggunakan local broker:

1. **Cari node yang menggunakan local broker:**
   - Double-click setiap node MQTT (yang berwarna hijau)
   - Lihat field "Server"
   - Jika masih ada yang tertulis "Local Broker", ganti ke "EMQX Public Broker"

2. **Deploy ulang**

3. **Restart Node-RED** (tekan Ctrl+C di terminal, lalu jalankan `node-red` lagi)

---

## ✨ Dashboard Tetap Berfungsi

Yang penting adalah:
```
✅ EMQX Public Broker: Connected
```

Artinya dashboard Anda **sudah bisa digunakan**:
- Buka: `http://localhost:1880/ui`
- Kontrol robot dari sana
- Terima data sensor real-time

**Error local broker bisa diabaikan!** 🎉