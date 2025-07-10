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
volatile double battery_mAh     = 6800.0;
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

// === ISR ===
void IRAM_ATTR myISR() {
  static bool polarity;
  lasttimestamp = timestamp;
  timestamp = micros();

  polarity = digitalRead(polPin);
  battery_mAh     += polarity ? ah_quanta     : -ah_quanta;
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
  Serial.println("---------------------------------");
  Serial.printf("Volt    : %.2f V\n", loadVoltage);
  Serial.printf("Current : %.2f mA\n", current_mA);
  Serial.printf("mAh     : %.1f\n", battery_mAh);
  Serial.printf("SoC     : %.1f %%\n", battery_percent);
  Serial.printf("Time    : %.2f s\n", (timestamp - lasttimestamp) / 1000000.0);
  Serial.println("---------------------------------");
}

// === OLED Display ===
void displayStatusToOLED() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);  display.println("BATRIHAP MONITOR");
  display.setCursor(0, 10); display.printf("Tegangan : %.2f V\n", loadVoltage);
  display.setCursor(0, 22); display.printf("Arus     : %.2f mA\n", current_mA);
  display.setCursor(0, 34); display.printf("mAh      : %.1f mAh\n", battery_mAh);
  display.setCursor(0, 46); display.printf("SoC      : %.1f %%\n", battery_percent);
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

  percent_quanta = 1.0 / (battery_mAh / 1000.0 * 5859.0 / 100.0);
}

void loop() {
  unsigned long now = millis();

  // === Task: Baca INA219 setiap 500 ms
  if (now - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = now;
    shuntVoltage_mV = ina219.getShuntVoltage_mV();
    busVoltage_V    = ina219.getBusVoltage_V();
    current_mA      = ina219.getCurrent_mA();
    loadVoltage     = busVoltage_V + (shuntVoltage_mV / 1000.0);
  }

  // === Task: LTC
  if (now - lastOledUpdate >= OLED_INTERVAL) {
    isrflag = false;
    lastOledUpdate = now;
    displayStatusToOLED();
  }

  // === Task: Serial debug print setiap 1 detik
  if (now - lastSerialPrint >= SERIAL_INTERVAL) {
    lastSerialPrint = now;
    printStatusToSerial();
  }
}
