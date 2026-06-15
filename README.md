# 🌡️ ESP32 Room Monitoring System dengan Micro SD Card

Sistem monitoring suhu dan kelembaban ruangan menggunakan mikrokontroler **ESP32** dan sensor **DHT22**. Proyek ini menggunakan konsep *Static File Hosting*, di mana file tampilan Web Dashboard disimpan di dalam **Micro SD Card** untuk menghemat memori internal ESP32 dan memudahkan modifikasi tampilan web tanpa perlu *flash* ulang kode.

## Fitur Utama
* **Web Dashboard:** Menampilkan grafik tren historis (*real-time* menggunakan Chart.js) serta status kenyamanan suhu ruangan.
* **Penyimpanan Data Lokal:** Menyimpan log data sensor setiap 1 menit ke dalam file `.csv` harian di Micro SD.
* **Sinkronisasi Waktu Akurat:** Menggunakan NTP Server (`id.pool.ntp.org`) untuk pencatatan waktu log yang presisi.
* **Layar OLED:** Menampilkan data suhu, kelembaban, dan jam secara lokal pada perangkat.
* **WiFiManager:** Konfigurasi koneksi Wi-Fi yang mudah via *Captive Portal* tanpa perlu *hardcode* SSID/Password di dalam program.

## 🛠️ Komponen yang Digunakan
1. ESP32 Development Board
2. Sensor Suhu & Kelembaban DHT22
3. Modul Micro SD Card (SPI) + Kartu Micro SD
4. Layar OLED SSD1306 (128x64) I2C
5. Kabel Jumper & Breadboard

## 🔌 Pin Out (Skema Koneksi)
### Modul Micro SD ke ESP32 (SPI Connection)
* **GND** -> GND
* **VCC** -> 5V
* **MISO** -> GPIO 19
* **MOSI** -> GPIO 23
* **SCK** -> GPIO 18
* **CS** -> GPIO 5

### Sensor DHT22 ke ESP32
* **DHT22 VCC (+)** -> 3.3V (3V3)
* **DHT22 DATA OUT** -> GPIO 4
* **OLED GND (-)** -> GND

### Sensor DHT22 & OLED ke ESP32
* **OLED GND** -> GND
* **OLED VCC** -> 3.3V (3V3)
* **OLED SDA** -> GPIO 21
* **OLED SCL** -> GPIO 22

## 📦 Library Arduino IDE yang Diperlukan
Pastikan Anda telah menginstal library berikut melalui *Library Manager* sebelum melakukan *compile*:
* `DHT sensor library` (oleh Adafruit)
* `Adafruit SSD1306` & `Adafruit GFX Library`
* `WiFiManager` (oleh tzapu)

## 📋 Cara Penggunaan
1. Upload file `src/src.ino` ke papan ESP32 menggunakan Arduino IDE.
2. Format kartu Micro SD ke sistem file **FAT32**.
3. Masukkan file `web/index.html` langsung ke direktori utama (root) kartu Micro SD lewat laptop, lalu pasang kembali ke modul Micro SD.
4. Nyalakan ESP32, hubungkan ponsel Anda ke Access Point `ESP32_RoomMonitor` untuk mengatur koneksi Wi-Fi rumah Anda.
5. Setelah terhubung, buka IP address yang tertera pada serial monitor atau layar OLED di browser Anda untuk melihat dashboard.