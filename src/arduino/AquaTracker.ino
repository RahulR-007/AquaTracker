#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <SPI.h>

// GSM
SoftwareSerial Sim(9, 10); // RX, TX
const String ALERT_RECIPIENT = "+916379061407";

// Pins
const int PH_PIN          = A0;
const int TEMP_PIN        = A1;
const int BATTERY_PIN     = A2;
const int TURBIDITY_PIN   = A3;
const int ULTRASONIC_TRIG = 7;
const int ULTRASONIC_ECHO = 6;

// pH: 2-point calibration (measure voltage in pH 7 and pH 4 buffer solutions)
const float PH7_VOLTAGE = 2.50;
const float PH4_VOLTAGE = 2.03;
const float PH_SLOPE    = (7.0 - 4.0) / (PH7_VOLTAGE - PH4_VOLTAGE);

// Thermistor (NTC 10K, Steinhart-Hart)
const float THERMISTOR_NOMINAL  = 10000.0;
const float TEMPERATURE_NOMINAL = 25.0;
const float B_COEFFICIENT       = 3950.0;
const float SERIES_RESISTOR     = 10000.0;

// Turbidity: voltage -> NTU polynomial
const float TURBIDITY_A = -1120.4;
const float TURBIDITY_B = 5742.3;
const float TURBIDITY_C = -4352.9;

// Tank geometry
const float TANK_HEIGHT_CM = 16.0;
const float MAX_WATER_CM   = 14.0;

// Battery monitor (voltage divider, adjust ratio to your resistors)
const float BATTERY_DIVIDER_RATIO = 6.0;
const float BATTERY_LOW_THRESHOLD = 11.0;

// Timing / alert settings
const unsigned long READ_INTERVAL_MS = 1500;
const unsigned long SMS_COOLDOWN_MS  = 900000; // 15 min
const int BAD_READING_STREAK         = 3;
const float WATER_LEVEL_ALERT_PCT    = 90.0;

unsigned long lastReadMillis = 0;
unsigned long lastSmsMillis  = 0;
int poorQualityStreak = 0;
int highLevelStreak   = 0;

void setup() {
  Serial.begin(9600);
  Sim.begin(57600);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.display();
  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Initializing AquaTracker..."));
  display.display();
  delay(1000);

  Sim.println("AT");
  updateSerial();
  Sim.println("AT+CSQ");
  updateSerial();
  Sim.println("AT+CCID");
  updateSerial();
  Sim.println("AT+CREG?");
  updateSerial();
}

void loop() {
  unsigned long now = millis();

  if (now - lastReadMillis >= READ_INTERVAL_MS) {
    lastReadMillis = now;
    evaluateAndDisplay();
  }

  if (Sim.available() > 0) {
    String command = Sim.readString();
    if (command.indexOf("RING") != -1) {
      getCallerNumber();
    }
  }
}

void evaluateAndDisplay() {
  float phValue             = getPHValue();
  float temperature          = getTemperature();
  float waterLevelPercentage = measureWaterLevel(temperature);
  float ntu                   = getTurbidityNTU();
  int   qualityPercentage      = ntuToQualityPercentage(ntu);
  String qualityString         = classifyWaterQuality(qualityPercentage);
  float batteryVoltage         = getBatteryVoltage();

  display.clearDisplay();
  display.setCursor(0, 0);
  display.print(F("pH: "));
  display.print(phValue, 2);
  display.print(F("  T: "));
  display.print(temperature, 1);
  display.println(F("C"));
  display.print(F("Level: "));
  display.print(waterLevelPercentage, 0);
  display.println(F("%"));
  display.print(F("Quality: "));
  display.print(qualityPercentage);
  display.print(F("% "));
  display.println(qualityString);
  display.display();

  bool waterHigh = waterLevelPercentage >= WATER_LEVEL_ALERT_PCT;
  bool waterPoor = qualityString == "Poor";

  highLevelStreak   = waterHigh ? highLevelStreak + 1   : 0;
  poorQualityStreak = waterPoor ? poorQualityStreak + 1 : 0;

  bool batteryLow = batteryVoltage < BATTERY_LOW_THRESHOLD;

  bool shouldAlert = (highLevelStreak >= BAD_READING_STREAK) ||
                      (poorQualityStreak >= BAD_READING_STREAK) ||
                      batteryLow;

  bool cooldownElapsed = (lastSmsMillis == 0) ||
                          (millis() - lastSmsMillis >= SMS_COOLDOWN_MS);

  if (shouldAlert && cooldownElapsed) {
    sendSMS(phValue, temperature, waterLevelPercentage, qualityPercentage,
            qualityString, batteryVoltage, batteryLow);
    lastSmsMillis = millis();
  }
}

// Median-of-N filter, kills single-sample ADC noise
int readAnalogMedian(int pin, int samples = 9) {
  int values[9];
  for (int i = 0; i < samples; i++) {
    values[i] = analogRead(pin);
    delayMicroseconds(200);
  }
  for (int i = 1; i < samples; i++) {
    int key = values[i];
    int j = i - 1;
    while (j >= 0 && values[j] > key) {
      values[j + 1] = values[j];
      j--;
    }
    values[j + 1] = key;
  }
  return values[samples / 2];
}

float getPHValue() {
  int raw = readAnalogMedian(PH_PIN);
  float voltage = raw * (5.0 / 1023.0);
  float phValue = PH_SLOPE * (voltage - PH7_VOLTAGE) + 7.0;
  return constrain(phValue, 0.0, 14.0);
}

float getTemperature() {
  int raw = readAnalogMedian(TEMP_PIN);
  float voltage = raw * (5.0 / 1023.0);
  if (voltage <= 0.001) voltage = 0.001;

  float resistance = SERIES_RESISTOR * (5.0 / voltage - 1.0);

  float steinhart = resistance / THERMISTOR_NOMINAL;
  steinhart = log(steinhart);
  steinhart /= B_COEFFICIENT;
  steinhart += 1.0 / (TEMPERATURE_NOMINAL + 273.15);
  steinhart = 1.0 / steinhart;
  steinhart -= 273.15;

  return steinhart;
}

float getTurbidityNTU() {
  int raw = readAnalogMedian(TURBIDITY_PIN);
  float voltage = raw * (5.0 / 1023.0);

  float ntu = TURBIDITY_A * voltage * voltage + TURBIDITY_B * voltage + TURBIDITY_C;
  if (ntu < 0) ntu = 0;
  if (ntu > 3000) ntu = 3000;
  return ntu;
}

// NTU -> 0-100% clarity score
int ntuToQualityPercentage(float ntu) {
  int pct;
  if (ntu <= 5)         pct = map(constrain(ntu, 0, 5), 0, 5, 100, 90);
  else if (ntu <= 25)   pct = map(ntu, 5, 25, 90, 70);
  else if (ntu <= 100)  pct = map(ntu, 25, 100, 70, 40);
  else                   pct = map(constrain(ntu, 100, 3000), 100, 3000, 40, 0);
  return constrain(pct, 0, 100);
}

String classifyWaterQuality(int qualityPercentage) {
  if (qualityPercentage < 30)       return "Poor";
  else if (qualityPercentage < 60)  return "Fair";
  else if (qualityPercentage < 85)  return "Good";
  else                                return "Excellent";
}

// Ultrasonic level: median-of-5 filtered, temperature-compensated speed of sound
float measureWaterLevel(float temperatureC) {
  const int SAMPLES = 5;
  long durations[SAMPLES];

  for (int i = 0; i < SAMPLES; i++) {
    pinMode(ULTRASONIC_TRIG, OUTPUT);
    digitalWrite(ULTRASONIC_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(ULTRASONIC_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(ULTRASONIC_TRIG, LOW);

    pinMode(ULTRASONIC_ECHO, INPUT);
    durations[i] = pulseIn(ULTRASONIC_ECHO, HIGH, 30000UL);
    delay(10);
  }

  for (int i = 1; i < SAMPLES; i++) {
    long key = durations[i];
    int j = i - 1;
    while (j >= 0 && durations[j] > key) {
      durations[j + 1] = durations[j];
      j--;
    }
    durations[j + 1] = key;
  }
  long duration = durations[SAMPLES / 2];

  float speedOfSoundMS   = 331.4 + 0.606 * temperatureC;
  float speedOfSoundCMuS = speedOfSoundMS / 10000.0;
  float distanceCM        = (duration * speedOfSoundCMuS) / 2.0;

  float waterColumnCM = TANK_HEIGHT_CM - distanceCM;
  float percentage     = (waterColumnCM / MAX_WATER_CM) * 100.0;

  return constrain(percentage, 0.0, 100.0);
}

float getBatteryVoltage() {
  int raw = readAnalogMedian(BATTERY_PIN);
  float pinVoltage = raw * (5.0 / 1023.0);
  return pinVoltage * BATTERY_DIVIDER_RATIO;
}

void getCallerNumber() {
  Sim.println("AT+CLCC");
  delay(500);
  Serial.println(F("Call detected"));
  String response = Sim.readString();
  int index1 = response.indexOf("\"", 1);
  int index2 = response.indexOf("\"", index1 + 1);
  String callerNumber = response.substring(index1 + 1, index2);

  Sim.println("AT+CMGF=1");
  delay(500);
  Sim.println("AT+CMGS=\"" + callerNumber + "\"");
  delay(500);
  sendCurrentReadingsToSim();
  Sim.write(26);
  delay(500);
}

void updateSerial() {
  delay(500);
  while (Serial.available()) Sim.write(Serial.read());
  while (Sim.available())     Serial.write(Sim.read());
}

void sendSMS(float phValue, float temperature, float waterLevelPercentage,
             int qualityPercentage, String qualityString,
             float batteryVoltage, bool batteryLow) {
  Sim.println("AT+CMGF=1");
  delay(500);
  Sim.println("AT+CMGS=\"" + ALERT_RECIPIENT + "\"\r");
  delay(500);
  Sim.println("AquaTracker Alert!");
  sendReadings(phValue, temperature, waterLevelPercentage, qualityPercentage, qualityString);
  if (batteryLow) {
    Sim.print("LOW BATTERY: ");
    Sim.print(batteryVoltage, 1);
    Sim.println("V");
  }
  delay(500);
  Sim.write(char(26));
  delay(4000);
  Serial.println(F("SMS sent successfully"));
}

void sendCurrentReadingsToSim() {
  float phValue             = getPHValue();
  float temperature          = getTemperature();
  float waterLevelPercentage = measureWaterLevel(temperature);
  float ntu                   = getTurbidityNTU();
  int   qualityPercentage      = ntuToQualityPercentage(ntu);
  String qualityString         = classifyWaterQuality(qualityPercentage);
  sendReadings(phValue, temperature, waterLevelPercentage, qualityPercentage, qualityString);
}

void sendReadings(float phValue, float temperature, float waterLevelPercentage,
                   int qualityPercentage, String qualityString) {
  Sim.println("******************");
  Sim.print("pH value    : "); Sim.println(phValue, 2);
  Sim.print("Temp        : "); Sim.print(temperature, 1); Sim.println(" C");
  Sim.print("Water level : "); Sim.print(waterLevelPercentage, 0); Sim.println(" %");
  Sim.print("Quality     : "); Sim.print(qualityPercentage); Sim.print(" % - "); Sim.println(qualityString);
}
