# 🌡️ ESP32 Room Monitoring System dengan Micro SD Card & Smart OLED Display

Sistem monitoring suhu dan kelembaban ruangan cerdas berbasis mikrokontroler **ESP32** dan sensor **DHT22**. Proyek ini menerapkan konsep *Static File Hosting*, di mana aset Web Dashboard diletakkan langsung di dalam **Micro SD Card** untuk menghemat memori *flash* internal ESP32 serta memudahkan modifikasi UI web tanpa perlu melakukan *flash* ulang kode mikrokontroler.

Proyek ini juga dilengkapi dengan manajemen layar OLED interaktif berbasis tombol untuk kenyamanan penggunaan jangka panjang dan pencegahan kerusakan perangkat keras.

---

## ✨ Fitur Utama & Pembaruan Sistem

* **Web Dashboard:** Menampilkan grafik tren historis secara *real-time* memanfaatkan Chart.js serta indikator tingkat kenyamanan ruangan (Heat Index).
* **Data Logger Mandiri (Lokal):** Menyimpan log data sensor secara otomatis setiap 1 menit ke dalam file `.csv` harian terpisah di Micro SD Card.
* **Sinkronisasi NTP Server:** Memanfaatkan `id.pool.ntp.org` melalui koneksi internet untuk penandatanganan stempel waktu (*timestamping*) log data yang presisi.
* **Smart Display OLED (Dual Page):** Menampilkan parameter sensor secara lokal dengan dukungan multi-halaman yang dapat berpindah fungsi menggunakan tombol fisik.
* **Fitur Rotasi Layar 180°:** Orientasi tampilan layar OLED dapat diputar balik secara instan dengan menahan tombol selama 5 detik jika posisi box/hardware terbalik.
* **Anti Burn-In & Smart Screen Saver:** Layar OLED otomatis meredup ke tingkat kecerahan 10% pada 5 detik terakhir sebelum akhirnya mati total (*sleep*) setelah 30 detik tanpa aktivitas tombol guna menjaga umur panel fosfor OLED. Layar dapat dibangunkan kembali secara instan melalui sekali tekanan tombol.
* **WiFiManager Integration:** Sistem konfigurasi jaringan Wi-Fi mandiri melalui *Captive Portal* (AP: `ESP32_RoomMonitor`) tanpa perlu menulis manual (*hardcode*) SSID dan Password pada kode program.

---

## 🛠️ Komponen yang Digunakan

1. **ESP32 Development Board** (NodeMCU ESP32)
2. **Sensor Suhu & Kelembaban DHT22** (AM2302)
3. **Modul Micro SD Card Adapter** (SPI Interface) + Kartu Micro SD (Format FAT32)
4. **Layar OLED SSD1306** (128x64 Pixels, I2C Interface)
5. **1x Tombol Tactile (Push Button) 2-Pin** (Sebagai pusat kendali UI)
6. Kabel Jumper & Breadboard

---

## 🔌 Pin Out & Skema Koneksi Perangkat

### 1. Modul Micro SD ke ESP32 (SPI Bus)

| Pin Modul SD   | Pin ESP32 | Keterangan             |
| :------------- | :-------- | :--------------------- |
| **GND**  | GND       | Ground Bersama         |
| **VCC**  | 5V / VIN  | Jalur Daya Utama Modul |
| **MISO** | GPIO 19   | Master In Slave Out    |
| **MOSI** | GPIO 23   | Master Out Slave In    |
| **SCK**  | GPIO 18   | Serial Clock           |
| **CS**   | GPIO 5    | Chip Select SPI        |

### 2. Sensor DHT22 ke ESP32

| Pin DHT22          | Pin ESP32  | Keterangan               |
| :----------------- | :--------- | :----------------------- |
| **VCC (+)**  | 3.3V (3V3) | Jalur Daya Sensor        |
| **DATA OUT** | GPIO 4     | Pembacaan Sinyal Digital |
| **GND (-)**  | GND        | Ground                   |

### 3. Layar OLED SSD1306 ke ESP32 (I2C Bus)

| Pin OLED      | Pin ESP32  | Keterangan        |
| :------------ | :--------- | :---------------- |
| **GND** | GND        | Ground            |
| **VCC** | 3.3V (3V3) | Jalur Daya Layar  |
| **SDA** | GPIO 21    | Serial Data Line  |
| **SCL** | GPIO 22    | Serial Clock Line |

### 4. Tombol Kendali Tactile (2-Pin)

* **Pin 1 Tombol** -> **GPIO 14** (Dikonfigurasi internal sebagai `INPUT_PULLUP`)
* **Pin 2 Tombol** -> **GND**

---

## 📦 Library Arduino IDE yang Diperlukan

Pastikan Anda telah menginstal beberapa dependensi library berikut melalui *Library Manager* Arduino IDE sebelum melakukan proses kompilasi (*compile*):

* `DHT sensor library` (oleh Adafruit)
* `Adafruit SSD1306`
* `Adafruit GFX Library`
* `WiFiManager` (oleh tzapu)

---

## 📋 Cara Penggunaan & Pemasangan

1. **Persiapan Penyimpanan Lokal:** Format kartu Micro SD Anda ke sistem file **FAT32**. Buat file tampilan dashboard Anda dan beri nama `index.html`. Masukkan file `index.html` tersebut langsung ke direktori luar paling depan (*root*) kartu Micro SD via komputer/laptop, lalu masukkan kartu kembali ke slot modul Micro SD ESP32.
2. **Kompilasi Program:** Buka kode program `.ino` Anda, pastikan semua konfigurasi pin sudah sesuai, lalu *Upload* program ke papan ESP32 melalui Arduino IDE atau VS Code (PlatformIO).
3. **Penyambungan Jaringan Pertama:** Saat pertama kali dinyalakan, layar OLED akan memunculkan instruksi untuk membuka IP `192.168.4.1`. Sambungkan Wi-Fi ponsel atau laptop Anda ke jaringan *Access Point* baru bernama **`ESP32_RoomMonitor`**. Isikan SSID dan Password Wi-Fi lokal rumah/laboratorium Anda pada halaman portal web yang muncul.
4. **Operasional Alat:** Setelah berhasil terhubung, ESP32 akan menyinkronkan waktu internet (NTP) secara otomatis dan mulai melakukan pencatatan data suhu-kelembaban ke dalam file `.csv` per hari (contoh nama file: `/16-06-26.csv`).
5. **Navigasi Tombol Fisik Perangkat:**
   * **Klik 1x:** Mengganti tampilan halaman OLED dari **Halaman 1** (Status Sensor) ke **Halaman 2** (Informasi alamat IP lokal) atau sebaliknya.
   * **Tekan & Tahan 5 Detik:** Memutar balik orientasi tampilan grafik layar OLED sebanyak 180 derajat (sangat berguna jika posisi casing terpasang terbalik).
   * **Klik 1x saat Layar Mati:** Membangunkan kembali layar OLED dari mode hemat daya (*Screen Saver*).
6. **Melihat Hasil Dashboard:** Jalankan browser di ponsel atau komputer yang berada dalam satu jaringan Wi-Fi yang sama, lalu ketikkan alamat IP lokal ESP32 Anda (misal: `192.168.1.50`) untuk masuk ke Web Dashboard utama. Untuk mengunduh data log CSV mentah, Anda bisa mengakses *endpoint* `/get-csv`.
