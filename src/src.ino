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

#define SD_CS_PIN 5 // Pin Chip Select untuk Micro SD

// === CONFIG LOG LOKAL (SD CARD) ===
const unsigned long jedaSimpanSD = 60000;
unsigned long waktuTerakhirSimpan = 0;

const long gmtOffset_sec = 28800; // GMT+8
const int daylightOffset_sec = 0;
const char* ntpServer = "id.pool.ntp.org";

WebServer server(80);

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

// === FUNGSI LAYAR OLED ===
void updateOLEDDisplay(float temp, float hum, String waktu) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
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

  // === INISIALISASI OLED ===
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("OLED tidak terdeteksi. Lanjut tanpa layar OLED..."));
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 10);
    display.println("Menyalakan Sistem...");
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

  String ipAddress = WiFi.localIP().toString(); 
  Serial.print("ESP32 Terhubung! Alamat IP: ");
  Serial.println(ipAddress);

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/get-csv", handleGetCSV);
  
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  server.begin();
}

void loop() {
  server.handleClient();

  static unsigned long waktuTerakhirOLED = 0;
  if (millis() - waktuTerakhirOLED >= 2000) {
    waktuTerakhirOLED = millis();
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    String jamSekarang = getFormattedTime();
    
    updateOLEDDisplay(t, h, jamSekarang);
  }

  unsigned long waktuSekarang = millis();
  if (waktuSekarang - waktuTerakhirSimpan >= jedaSimpanSD) {
    String jamSekarang = getFormattedTime();

    if (jamSekarang == "00:00:00") {
      Serial.println("[\xE2\x9A\xA0] Menunda penyimpanan: Memperbarui waktu internet...");
      return;
    }

    waktuTerakhirSimpan = waktuSekarang;
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (!isnan(t) && !isnan(h)) {
      simpanKeMicroSD(jamSekarang, t, h);
    }
  }
}