#include <SoftwareSerial.h>
#include <SPI.h>

SoftwareSerial Sim(9, 10); // RX, TX

// Pin setup
int turbidityPin = A3;
int phPin = A0;
const int ringPin = 2; 

// pH calibration
const float PH_ACID_VALUE     = 4.01;
const float PH_NEUTRAL_VALUE  = 6.86; 
const float PH_ALKALINE_VALUE = 9.18;


const float PH_ACID_VOLTAGE     = 3030.0; 
const float PH_NEUTRAL_VOLTAGE  = 2500.0;
const float PH_ALKALINE_VOLTAGE = 1950.0; 

const float CLEAR_WATER_VOLTAGE = 4.2; 
const float MURKY_WATER_VOLTAGE = 2.5; 

// Water level
const int pingPin = 7;
const int echoPin = 6;
const int tankHeight = 16;
const int maxWater = 14;

unsigned long lastSensorRead = 0;
const unsigned long SENSOR_INTERVAL = 2000;

// ISR-driven call flag
volatile bool incomingCall = false;

void ringISR() {
  incomingCall = true;
}

void setup() {
  Serial.begin(9600);
  Sim.begin(57600);

  Sim.println("AT");
  updateSerial();
  Sim.println("AT+CSQ");
  updateSerial();
  Sim.println("AT+CCID");
  updateSerial();
  Sim.println("AT+CREG?");
  updateSerial();

  pinMode(ringPin, INPUT_PULLUP); 
  attachInterrupt(digitalPinToInterrupt(ringPin), ringISR, FALLING);
}

void loop() {
  if (incomingCall) {
    incomingCall = false; 
    getCallerNumber();
  }

  if (millis() - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = millis();
    checkSensorsAndAlert();
  }
}

void checkSensorsAndAlert() {
  float phValue = getPHValue();
  float waterLevelPercentage = measureWaterLevel();
  int qualityPercentage = getWaterQualityPercentage();
  String qualityString = calculateWaterQuality(qualityPercentage);

  Serial.print("pH value     : ");
  Serial.println(phValue, 2);
  Serial.print("Water level : ");
  Serial.print((int)waterLevelPercentage);
  Serial.println("%");
  Serial.print("Quality %:");
  Serial.println(qualityPercentage);
  Serial.println(qualityString);

  if (waterLevelPercentage >= 90.0 || qualityString == "Poor") {
    sendSMS();
  }
}

void getCallerNumber() {
  Sim.println("AT+CLCC");
  delay(500);
  Serial.println("call detected");
  String response = Sim.readString();
  int index1 = response.indexOf("\"", 1);
  int index2 = response.indexOf("\"", index1 + 1);

  if (index1 == -1 || index2 == -1) {
    Serial.println("Could not parse caller number");
    return; 
  }

  String callerNumber = response.substring(index1 + 1, index2);
  Sim.println("AT+CMGF=1");
  delay(500);
  Sim.println("AT+CMGS=\"" + callerNumber + "\"");
  delay(500);
  printData();
  Sim.write(26);
  delay(500);
}

void updateSerial() {
  delay(500);
  while (Serial.available()) {
    Sim.write(Serial.read());
  }
  while (Sim.available()) {
    Serial.write(Sim.read());
  }
}

void sendSMS() {
  Sim.println("AT+CMGF=1");
  delay(500);
  Sim.println("AT+CMGS=\"+91xxxxxxxxxx\"\r"); // recipient number
  delay(500);
  Sim.println("Alert..!!!");
  printData();
  delay(500);
  Sim.write(char(26));
  delay(4000);
}

float getPHValue() {
  int raw = analogRead(phPin);
  float voltage = raw / 1024.0 * 5000.0;

  if (voltage <= PH_NEUTRAL_VOLTAGE) {
    float slope = (PH_NEUTRAL_VALUE - PH_ACID_VALUE) / (PH_NEUTRAL_VOLTAGE - PH_ACID_VOLTAGE);
    return PH_NEUTRAL_VALUE + slope * (voltage - PH_NEUTRAL_VOLTAGE);
  } else {
    float slope = (PH_ALKALINE_VALUE - PH_NEUTRAL_VALUE) / (PH_ALKALINE_VOLTAGE - PH_NEUTRAL_VOLTAGE);
    return PH_NEUTRAL_VALUE + slope * (voltage - PH_NEUTRAL_VOLTAGE);
  }
}

float measureWaterLevel() {
  double duration, cm;
  pinMode(pingPin, OUTPUT);
  digitalWrite(pingPin, LOW);
  delayMicroseconds(2);
  digitalWrite(pingPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(pingPin, LOW);
  pinMode(echoPin, INPUT);
  duration = pulseIn(echoPin, HIGH);
  cm = microsecondsToCentimeters(duration);

  float waterLevel = tankHeight - cm;
  float percentage = (waterLevel * 100.0) / maxWater;
  return percentage;
}

long microsecondsToCentimeters(long microseconds) {
  return microseconds / 29 / 2;
}

int getWaterQualityPercentage() {
  int raw = analogRead(turbidityPin);
  float voltage = raw / 1024.0 * 5.0; 

  float percentage = (voltage - MURKY_WATER_VOLTAGE) / (CLEAR_WATER_VOLTAGE - MURKY_WATER_VOLTAGE) * 100.0;
  int qualityPercentage = constrain((int)percentage, 0, 100);
  return qualityPercentage;
}

String calculateWaterQuality(int qualityPercentage) {
  if (qualityPercentage < 30) {
    return "Poor";
  } else if (qualityPercentage < 60) {
    return "Fair";
  } else if (qualityPercentage < 90) {
    return "Good";
  } else {
    return "Excellent";
  }
}

void printData() {
  float phValue = getPHValue();
  float waterLevelPercentage = measureWaterLevel();
  int qualityPercentage = getWaterQualityPercentage();
  String qualityString = calculateWaterQuality(qualityPercentage);

  Sim.println("******************");
  Sim.print("pH value     : ");
  Sim.print(phValue, 2);
  Sim.println();
  Sim.print("Water level : ");
  Sim.print((int)waterLevelPercentage);
  Sim.println("%");
  Sim.print("Water Quality Percentage: ");
  Sim.print(qualityPercentage);
  Sim.println("%");
  Sim.println("Water Quality is");
  Sim.println(qualityString);
  Serial.println("Sms sent successfully");
}
