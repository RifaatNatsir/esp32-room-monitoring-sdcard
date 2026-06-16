#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include "time.h"
#include <WiFiManager.h> 
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <SD.h>

// === CONFIG HARDWARE ===
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define SD_CS_PIN 5

// === CONFIG TOMBOL TACTILE ===
#define BUTTON_PIN 14
int halamanOLED = 0;
bool statusTombolTerakhir = HIGH;
unsigned long waktuDebounceTerakhir = 0;
const unsigned long jedaDebounce = 50;

unsigned long waktuTombolMulaiDitekan = 0;
bool sedangDitekan = false;
bool sudahEksekusiLongPress = false;
int rotasiLayar = 0; // 0 = Normal, 2 = Rotasi 180 Derajat

// === CONFIG LOG LOKAL (SD CARD) ===
const unsigned long jedaSimpanSD = 60000;
unsigned long waktuTerakhirSimpan = 0;

const long gmtOffset_sec = 28800; // GMT+8
const int daylightOffset_sec = 0;
const char* ntpServer = "id.pool.ntp.org";

WebServer server(80);
String globalIPAddress = "0.0.0.0";

// === FUNGSI WAKTU ===
String getFormattedTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "00:00:00";
  }
  char timeStringBuff[50];
  strftime(timeStringBuff, sizeof(timeStringBuff), "%H:%M:%S", &timeinfo);
  return String(timeStringBuff);
}

String getFormattedDate() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "00-00-00";
  }
  char dateStringBuff[20];
  strftime(dateStringBuff, sizeof(dateStringBuff), "%d-%m-%y", &timeinfo);
  return String(dateStringBuff);
}

// === FUNGSI MENYIMPAN KE MICRO SD (*PER HARI*)===
void simpanKeMicroSD(String waktu, float temp, float hum) {
  String tanggalHariIni = getFormattedDate();
  
  if (tanggalHariIni == "00-00-00") {
    Serial.println("[\xE2\x9D\x8C] Gagal simpan ke SD: Menunggu sinkronisasi waktu internet...");
    return;
  }

  String namaFile = "/" + tanggalHariIni + ".csv";
  
  if (!SD.exists(namaFile)) {
    File buatFile = SD.open(namaFile, FILE_WRITE);
    if (buatFile) {
      buatFile.println("Waktu,Suhu,Kelembaban");
      buatFile.close();
      Serial.println("[\xE2\x9C\x94] File baru dibuat untuk hari ini: " + namaFile);
    }
  }

  File dataFile = SD.open(namaFile, FILE_APPEND);
  
  if (dataFile) {
    dataFile.print(waktu);
    dataFile.print(",");
    dataFile.print(temp, 1);
    dataFile.print(",");
    dataFile.println(hum, 1);
    dataFile.close();
    Serial.println("[\xE2\x9C\x94] Data berhasil ditulis ke: " + namaFile + " pada " + getFormattedTime());
  } else {
    Serial.println("[\xE2\x9D\x8C] Gagal membuka file " + namaFile + " pada " + getFormattedTime());
  }
}

// === FUNGSI LAYAR OLED (MENDUKUNG MULTI-HALAMAN) ===
void updateOLEDDisplay(float temp, float hum, String waktu) {
  display.clearDisplay();
  display.setRotation(rotasiLayar);
  display.setTextColor(SSD1306_WHITE);
  
  if (halamanOLED == 0) {
    // === HALAMAN 1: MONITORING SENSOR ===
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("ROOM MONITORING");
    display.drawFastHLine(0, 10, 128, SSD1306_WHITE);
    
    display.setCursor(0, 18);
    display.print("Suhu: ");
    if (isnan(temp)) display.print("--");
    else { display.print(temp, 1); display.print(" C"); }
    
    display.setCursor(0, 32);
    display.print("Lembab: ");
    if (isnan(hum)) display.print("--");
    else { display.print(hum, 1); display.print(" %"); }
    
    display.drawRect(0, 46, 128, 18, SSD1306_WHITE);
    display.setCursor(40, 51);
    display.print(waktu);
    
  } else {
    // === HALAMAN 2: INFORMASI IP ADDRESS ===
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("NETWORK INFO");
    display.drawFastHLine(0, 10, 128, SSD1306_WHITE);
    
    display.setCursor(0, 20);
    display.print("Status: Connected");
    
    display.setCursor(0, 35);
    display.print("IP Address:");
    
    display.setCursor(0, 48);
    display.setTextSize(1);
    display.print(globalIPAddress);
  }
  
  display.display();
}

// === WEB SERVER ENDPOINT DATA JSON ===
void handleData() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  if (isnan(temp) || isnan(hum)) {
    server.send(500, "application/json", "{\"error\":true}");
    return;
  }

  float heatIndex = dht.computeHeatIndex(temp, hum, false);
  String json = "{";
  json += "\"temp\":" + String(temp, 1) + ",";
  json += "\"hum\":" + String(hum, 1) + ",";
  json += "\"hi\":" + String(heatIndex, 1) + ",";
  json += "\"time\":\"" + getFormattedTime() + "\"";
  json += "}";

  server.send(200, "application/json", json);
}

// === WEB SERVER ENDPOINT STREAM CSV HARI INI ===
void handleGetCSV() {
  String tanggalHariIni = getFormattedDate();
  
  if (tanggalHariIni == "00-00-00") {
    server.send(500, "text/plain", "Gagal sinkronisasi waktu internet. Hubungkan ESP32 ke WiFi.");
    return;
  }

  String namaFile = "/" + tanggalHariIni + ".csv";

  File dataFile = SD.open(namaFile, FILE_READ);
  
  if (!dataFile) {
    server.send(404, "text/plain", "File " + namaFile + " belum tersedia di Micro SD.");
    return;
  }
  
  server.streamFile(dataFile, "text/csv");
  dataFile.close();
}

// === WEB SERVER ROOT (MEMBACA HTML DARI MICRO SD) ===
void handleRoot() {
  File webFile = SD.open("/index.html", FILE_READ);
  
  if (webFile) {
    server.streamFile(webFile, "text/html");
    webFile.close();
  } else {
    server.send(404, "text/plain", "Kritis: File /index.html tidak ditemukan di Micro SD!");
  }
}

void setup() {
  delay(1000); 
  Serial.begin(115200);
  dht.begin();

  // === INISIALISASI TOMBOL ===
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // === INISIALISASI OLED ===
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("OLED tidak terdeteksi. Lanjut tanpa layar OLED..."));
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 10);
    display.println("Inisialisasi... Buka 192.169.4.1");
    display.display();
  }

  // === INISIALISASI MICRO SD ===
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Gagal mendeteksi Micro SD! Periksa kabel SPI.");
  } else {
    Serial.println("Micro SD berhasil diinisialisasi.");
  }

  WiFiManager wm;

  if(!wm.autoConnect("ESP32_RoomMonitor")) {
      delay(3000);
      ESP.restart();
  }

  globalIPAddress = WiFi.localIP().toString(); 
  Serial.print("ESP32 Terhubung! Alamat IP: ");
  Serial.println(globalIPAddress);

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/get-csv", handleGetCSV);
  
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  server.begin();
  
  waktuTerakhirSimpan = millis(); 
}

unsigned long waktuTerakhirAktivitasTombol = 0;
bool layarSedangMenyala = true;
bool statusFadeOutAktif = false;
const unsigned long jedaScreenSaver = 30000; // 30 detik (30.000 ms)
const unsigned long durasiFadeOut = 5000; // 5 detik terakhir (5.000 ms)

void loop() {
  server.handleClient();

  bool pembacaanTombol = digitalRead(BUTTON_PIN);
  
  if (pembacaanTombol != statusTombolTerakhir) {
    waktuDebounceTerakhir = millis(); 
  }

  if ((millis() - waktuDebounceTerakhir) > jedaDebounce) {
    // Deteksi saat tombol baru mulai ditekan (transisi ke LOW)
    if (pembacaanTombol == LOW && !sedangDitekan) {
      sedangDitekan = true;
      waktuTombolMulaiDitekan = millis();
      sudahEksekusiLongPress = false;

      // Jika layar sedang mati atau sedang meredup, kembalikan ke terang maksimal
      if (!layarSedangMenyala || statusFadeOutAktif) {
        display.ssd1306_command(SSD1306_SETCONTRAST);
        display.ssd1306_command(127); // 50% dari 100%
        
        if (!layarSedangMenyala) {
          display.ssd1306_command(SSD1306_DISPLAYON);
          layarSedangMenyala = true;
        }
        
        statusFadeOutAktif = false;
        waktuTerakhirAktivitasTombol = millis(); 
        sudahEksekusiLongPress = true; // Kunci agar tekanan pertama tidak memicu ganti halaman
        
        updateOLEDDisplay(dht.readTemperature(), dht.readHumidity(), getFormattedTime());
      }

      waktuTerakhirAktivitasTombol = millis();
    }
    
    // Deteksi saat tombol masih ditahan (mengecek apakah sudah lewat 5 detik)
    if (pembacaanTombol == LOW && sedangDitekan && !sudahEksekusiLongPress) {
      if (millis() - waktuTombolMulaiDitekan >= 5000) {
        // Balik rotasi layar (jika 0 jadi 2, jika 2 jadi 0)
        rotasiLayar = (rotasiLayar == 0) ? 2 : 0; 
        sudahEksekusiLongPress = true;
        Serial.println("[\xE2\x9C\x94] Layar dirotasikan 180 derajat!");
        
        float t = dht.readTemperature();
        float h = dht.readHumidity();
        updateOLEDDisplay(t, h, getFormattedTime());
      }
    }

    // Deteksi saat tombol dilepas (transisi kembali ke HIGH)
    if (pembacaanTombol == HIGH && sedangDitekan) {
      sedangDitekan = false;
      
      // Jika tombol dilepas SEBELUM mencapai 5 detik, hitung sebagai klik biasa (ganti halaman)
      if (!sudahEksekusiLongPress) {
        halamanOLED = !halamanOLED; 
        Serial.print("Tombol dilepas (Klik Biasa)! Halaman: ");
        Serial.println(halamanOLED);
        
        float t = dht.readTemperature();
        float h = dht.readHumidity();
        updateOLEDDisplay(t, h, getFormattedTime());
      }
    }
  }
  statusTombolTerakhir = pembacaanTombol;

  // === LOGIKA SCREEN SAVER ===
  if (layarSedangMenyala) {
    unsigned long waktuBerjalan = millis() - waktuTerakhirAktivitasTombol;

    if (waktuBerjalan >= jedaScreenSaver) {
      // Matikan layar total dan reset kontras ke normal
      display.ssd1306_command(SSD1306_DISPLAYOFF);
      layarSedangMenyala = false;
      statusFadeOutAktif = false;
      
      display.ssd1306_command(SSD1306_SETCONTRAST);
      display.ssd1306_command(127); // 50% dari 255
      
      Serial.println("[💤] Screen Saver Aktif: Layar OLED dimatikan untuk mencegah Burn-In.");
      
    } else if (waktuBerjalan >= (jedaScreenSaver - 5000)) { 
      // 5 detik sebelum mati, langsung set kecerahan ke 10%
      if (!statusFadeOutAktif) { 
        statusFadeOutAktif = true;
        
        display.ssd1306_command(SSD1306_SETCONTRAST);
        display.ssd1306_command(25);  // 10% dari 255
        display.display();
        
        Serial.println("[⚠️] Layar diredupkan ke 10%!");
      }
    }
  }

  // === REFRESH LAYAR OLED TIAP 1 DETIK ===
  static unsigned long waktuTerakhirOLED = 0;
  if ((layarSedangMenyala || statusFadeOutAktif) && (millis() - waktuTerakhirOLED >= 1000)) {
    waktuTerakhirOLED = millis();
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    String jamSekarang = getFormattedTime();
    
    updateOLEDDisplay(t, h, jamSekarang);
  }

  // === LENGKAP: DATA LOGGING MICRO SD (1 MENIT) ===
  unsigned long waktuSekarang = millis();
  if (waktuSekarang - waktuTerakhirSimpan >= jedaSimpanSD) {
    waktuTerakhirSimpan = waktuSekarang; 
    
    String jamSekarang = getFormattedTime();

    if (jamSekarang == "00:00:00") {
      Serial.println("[\xE2\x9A\xA0] Menunda penyimpanan: Memperbarui waktu internet...");
      return; 
    }

    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (!isnan(t) && !isnan(h)) {
      simpanKeMicroSD(jamSekarang, t, h);
    }
  }
}