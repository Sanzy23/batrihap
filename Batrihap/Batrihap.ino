#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_INA219.h>
#include <Batrihap_image.h>
#include <Wire.h>

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define CONFIG_FREERTOS_UNICORE

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_INA219 ina219;

// === LTC4150 Setup ===
const int ltcInterruptPin = 2;
const int polPin          = 4;       
volatile int pulseCount   = 0;

// Kapasitas baterai penuh asumsi
volatile double battery_mAh     = 6800.0;
volatile double battery_percent = 100.0;

volatile boolean isrflag;
volatile long int timestamp, lasttimestamp;
volatile double mA;

const double ah_quanta = 0.17067759;
double percent_quanta;

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

// === Serial Debug Print ===
void printStatusToSerial(float voltage, float current) {
  Serial.println("---------------------------------");
  Serial.printf("Volt    : %.2f V\n", voltage);
  Serial.printf("Current : %.2f mA\n", current);
  Serial.printf("mAh     : %.1f\n", battery_mAh);
  Serial.printf("SoC     : %.1f %%\n", battery_percent);
  Serial.printf("Time    : %.2f s\n", (timestamp - lasttimestamp) / 1000000.0);
  Serial.println("---------------------------------");
}

// === OLED Display ===
void displayStatusToOLED(float voltage, float current) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(0, 0);  display.println("BATRIHAP MONITOR");
  display.setCursor(0, 10); display.printf("Tegangan : %.2f V\n", voltage);
  display.setCursor(0, 22); display.printf("Arus     : %.2f mA\n", current);
  display.setCursor(0, 34); display.printf("mAh      : %.1f mAh\n", battery_mAh);
  display.setCursor(0, 46); display.printf("SoC      : %.1f %%\n", battery_percent);

  display.display();
}

// === RTOS Display Task ===
void TaskDisplay(void *pvParameters) {
  for (;;) {
    float shuntVoltage_mV = ina219.getShuntVoltage_mV();
    float busVoltage_V    = ina219.getBusVoltage_V();
    float current_mA      = ina219.getCurrent_mA();
    float loadVoltage     = busVoltage_V + (shuntVoltage_mV / 1000.0);

    if (isrflag) {
      isrflag = false;
      printStatusToSerial(loadVoltage, current_mA);
      displayStatusToOLED(loadVoltage, current_mA);
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

// === SETUP ===
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

  xTaskCreatePinnedToCore(TaskDisplay, "Display Task", 4096, NULL, 1, NULL, 1);
}

void loop() {
  // Loop kosongan wak, pake RTOS soale
}