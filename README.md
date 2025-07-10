# 🔋 BATRIHAP – Battery Health and Prediction System

**BATRIHAP** (Battery Management System with Predictive Algorithm and IoT) adalah sistem monitoring baterai berbasis ESP32 yang mampu membaca tegangan, arus, kapasitas baterai (mAh), dan status pengisian, serta menampilkan data secara real-time ke OLED dan dashboard mobile. Sistem ini juga dirancang untuk mendukung algoritma prediktif dalam memperkirakan waktu penggunaan dan pengisian baterai secara lebih akurat.

---

## 📱 Tampilan Antarmuka

Antarmuka pengguna (UI) dirancang minimalis dan intuitif, menampilkan:
- Tegangan baterai (Volt)
- Arus baterai (Ampere)
- Status pengisian: Charging / Discharging
- Persentase baterai (SoC – State of Charge)
- Grafik historis SoC dari waktu ke waktu

---

## 🚀 Fitur Utama

- Monitoring tegangan dan arus menggunakan **INA219**
- Penghitungan mAh dan estimasi kapasitas dengan **LTC4150**
- Menampilkan data real-time di OLED display
- Kompatibel dengan **FreeRTOS**
- Dashboard mobile UI responsif
- Siap untuk integrasi algoritma **prediksi SoC / SoH**
- Mendukung analisis tren baterai dari waktu ke waktu

---

## 🛠️ Hardware yang Dibutuhkan

- [x] ESP32 Dev Board  
- [x] Sensor INA219 (Voltage & Current)
- [x] Sensor LTC4150 (Coulomb Counter)
- [x] OLED SSD1306 (0.96" I2C)
- [x] Resistor & kabel jumper
