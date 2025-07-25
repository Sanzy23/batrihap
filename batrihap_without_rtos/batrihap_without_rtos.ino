#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_INA219.h>
#include <Batrihap_image.h>
#include <Wire.h>

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_INA219 ina219;

// === LTC4150 Setup ===
const int ltcInterruptPin = 16;
const int polPin          = 17;

volatile double battery_mAh     = 0.0;
volatile double battery_percent = 100.0;
volatile long int timestamp, lasttimestamp;
volatile double mA;

const double ah_quanta = 0.17067759;
double percent_quanta;
volatile boolean isrflag = false;

// Waktu task
unsigned long lastSensorRead   = 0;
unsigned long lastOledUpdate   = 0;
unsigned long lastSerialPrint  = 0;

const unsigned long SENSOR_INTERVAL  = 500;  // ms
const unsigned long OLED_INTERVAL    = 500;  // ms
const unsigned long SERIAL_INTERVAL  = 1000; // ms

// Data INA219
float busVoltage_V    = 0.0;
float shuntVoltage_mV = 0.0;
float current_mA      = 0.0;
float loadVoltage     = 0.0;

// Estimasi awal kapasitas baterai
float initial_mAh = 0.0;
float mAh_used    = 0.0;

// === Estimasi mAh awal berdasarkan tegangan ===
float estimateInitialmAhFromVoltage(float voltage) {
  voltage = constrain(voltage, 6.0, 8.4);
  return map(voltage * 100, 600, 840, 0, 6800); // tegangan dikali 100 untuk map()
}

// === ISR ===
void IRAM_ATTR myISR() {
  static bool polarity;
  lasttimestamp = timestamp;
  timestamp = micros();

  polarity = digitalRead(polPin);
  battery_mAh += polarity ? ah_quanta : -ah_quanta;
  battery_percent += polarity ? percent_quanta : -percent_quanta;

  mA = 614.4 / ((timestamp - lasttimestamp) / 1000000.0);
  if (polarity) mA *= -1;

  isrflag = true;
}

// === Splash Screen ===
void splashscreen() {
  display.clearDisplay();
  display.drawBitmap(0, 0, epd_bitmap_batrihap, 128, 64, WHITE);
  display.display();
  delay(2000);
  display.clearDisplay();
}

// === Serial Debug ===
void printStatusToSerial() {
  float SoC_coulomb = 100.0 * (battery_mAh) / initial_mAh;
  float SoC_voltage = map(busVoltage_V * 100, 600, 840, 0, 100);
  SoC_coulomb = constrain(SoC_coulomb, 0, 100);
  SoC_voltage = constrain(SoC_voltage, 0, 100);
  float SoC = min(SoC_coulomb, SoC_voltage);

  Serial.println("---------------------------------");
  Serial.printf("Tegangan               : %.2f V\n", loadVoltage);
  Serial.printf("Arus                   : %.2f mA\n", current_mA);
  Serial.printf("mAh (Coulomb count)    : %.1f\n", battery_mAh);
  Serial.printf("SoC berdasarkan coulomb: %.1f %%\n", SoC_coulomb);
  Serial.printf("SoC berdasarkan tegangan: %.1f %%\n", SoC_voltage);
  Serial.printf("SoC akhir (gabungan)   : %.1f %%\n", SoC);
  Serial.println("---------------------------------");
}

// === OLED Display ===
void displayStatusToOLED() {
  float SoC_coulomb = 100.0 * (battery_mAh) / initial_mAh;
  float SoC_voltage = map(busVoltage_V * 100, 600, 840, 0, 100);
  SoC_coulomb = constrain(SoC_coulomb, 0, 100);
  SoC_voltage = constrain(SoC_voltage, 0, 100);
  float SoC = min(SoC_coulomb, SoC_voltage);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);  display.println("BATRIHAP MONITOR");
  display.setCursor(0, 10); display.printf("Tegangan : %.2f V\n", loadVoltage);
  display.setCursor(0, 22); display.printf("Arus     : %.2f mA\n", current_mA);
  display.setCursor(0, 34); display.printf("mAh      : %.1f mAh\n", battery_mAh);
  display.setCursor(0, 46); display.printf("SoC      : %.1f %%\n", SoC);
  display.display();
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED gagal diinisialisasi"));
    while (1);
  }

  if (!ina219.begin()) {
    Serial.println(F("INA219 gagal diinisialisasi"));
    while (1);
  }

  pinMode(ltcInterruptPin, INPUT);
  pinMode(polPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(ltcInterruptPin), myISR, FALLING);

  splashscreen();

  // === Baca tegangan awal dan hitung mAh awal
  shuntVoltage_mV = ina219.getShuntVoltage_mV();
  busVoltage_V    = ina219.getBusVoltage_V();
  loadVoltage     = busVoltage_V + (shuntVoltage_mV / 1000.0);
  initial_mAh     = estimateInitialmAhFromVoltage(loadVoltage);
  battery_mAh     = initial_mAh;

  // Hitung kuanta persen
  percent_quanta = 1.0 / (initial_mAh / 1000.0 * 5859.0 / 100.0);
}

void loop() {
  unsigned long now = millis();

  if (now - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = now;
    shuntVoltage_mV = ina219.getShuntVoltage_mV();
    busVoltage_V    = ina219.getBusVoltage_V();
    current_mA      = ina219.getCurrent_mA();
    loadVoltage     = busVoltage_V + (shuntVoltage_mV / 1000.0);
  }

  if (now - lastOledUpdate >= OLED_INTERVAL) {
    lastOledUpdate = now;
    displayStatusToOLED();
  }

  if (now - lastSerialPrint >= SERIAL_INTERVAL) {
    lastSerialPrint = now;
    printStatusToSerial();
  }
}
