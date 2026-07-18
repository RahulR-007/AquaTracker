# AquaTracker

## Overview

AquaTracker is an Arduino-based water-monitoring prototype that measures pH, turbidity (water quality), temperature, and tank water level. It displays readings on an OLED and sends SMS alerts via a GSM module when thresholds are reached.

## Included files

- `AquaTracker.ino` — Arduino sketch (main project code)
- `AquaTracker_architecture.png` — Architecture & wiring diagram
- `AquaTracker.pptx` — Presentation (generated)
- `README.md` — This file

## Hardware modules and connections

Recommended wiring for Arduino Uno (labels are Arduino pins unless otherwise stated):

- GSM Module (SIM900 / SIM800 series)
  - VCC: 5V (use external power supply rated for GSM current spikes)
  - GND: Common ground
  - TX -> Arduino pin 9 (SoftwareSerial RX)
  - RX -> Arduino pin 10 (SoftwareSerial TX)
  - Use a logic-level converter if module requires different voltage on RX/TX lines.

- OLED Display (SSD1306, I2C)
  - VCC: 3.3V or 5V depending on module
  - GND: Common ground
  - SDA -> A4 (I2C SDA on Uno)
  - SCL -> A5 (I2C SCL on Uno)
  - I2C address: commonly `0x3C` (confirm before wiring)

- pH Sensor
  - Analog output -> A0
  - VCC: 5V or as specified
  - GND: Common ground
  - Note: pH sensors often require calibration and an external amplifier board.

- Temperature Sensor (analog)
  - Analog output -> A1
  - VCC/GND: as sensor requires

- Turbidity Sensor (water quality)
  - Analog output -> A3
  - VCC/GND: as sensor requires

- Ultrasonic Sensor (HC-SR04)
  - VCC: 5V
  - GND: Common ground
  - Trig -> Pin 7
  - Echo -> Pin 6
  - Keep sensor above max waterline; measure distance to water surface

## Assembly steps

1. Prepare a stable power source: GSM modules draw high current on transmit — use a 5V regulated supply capable of 2A or more. For solar/battery setups, include a charge controller and voltage regulator.
2. Mount the Arduino and breadboard on a non-conductive base. Keep wiring short for sensors.
3. Place OLED near the Arduino for easy viewing; route I2C wires cleanly to minimize noise.
4. Install the ultrasonic sensor above the tank, pointing straight down. Secure with a bracket.
5. Route pH and turbidity sensors into the water at representative sampling locations; ensure probe housings prevent dislodging.
6. Connect GSM antenna and verify SIM card has SMS capability and active balance.
7. Power up and open Arduino Serial Monitor at 9600 baud to observe initialization logs and GSM handshake.
8. Calibrate sensors (pH calibration solutions, turbidity baselines) and verify readings match known samples.

## Software notes

- The sketch uses `Adafruit_GFX` and `Adafruit_SSD1306` for the OLED. Install these libraries via the Arduino Library Manager.
- The use of the OLED screen is optional and be used without its presence.
- Update the phone number in the `sendSMS()` function before deploying.


