# AquaTracker

AquaTracker is an Arduino-based water monitoring prototype for checking tank conditions and sending alerts when the water status needs attention. It was built as a showcase project to demonstrate sensing, control, and communication in a practical embedded system.

## What it does

- Measures key water-related readings from the tank setup
- Monitors the water condition continuously through the Arduino
- Sends alerts when the system detects an issue
- Provides a complete demo package with code, diagram, prototype photo, and presentation material

## How it works

1. The sensors collect readings from the water system.
2. The Arduino processes the readings and compares them with alert thresholds.
3. If the readings indicate a problem, the system triggers a notification.
4. The project also includes the wiring diagram, prototype image, and 3D model reference for presentation and documentation.

## Hardware connections

- pH sensor -> `A0`
- Turbidity sensor -> `A3`
- Ultrasonic sensor TRIG -> `D7`
- Ultrasonic sensor ECHO -> `D6`
- GSM Ring Interrupt -> `D2`
- GSM TX -> `D9` (Arduino RX)
- GSM RX -> `D10` (Arduino TX)

## Assembly notes

1. Connect the sensors to the Arduino according to the wiring diagram.
2. Use a stable power supply for the GSM module.
3. Mount the prototype so the water-level sensor has a clear line to the water surface.
4. Update the alert phone number in the sketch before using the project.
5. Use the prototype photo and 3D model reference together when explaining the build.

## Project files

- [src/arduino/AquaTracker.ino](src/arduino/AquaTracker.ino) - main Arduino sketch
- [docs/architecture/AquaTracker_architecture.png](docs/architecture/AquaTracker_architecture.png) - architecture and wiring diagram
- [docs/prototype/Prototype.png](docs/prototype/Prototype.png) - actual prototype photo
- [docs/presentations/AquaTracker_Presentation.pptx](docs/presentations/AquaTracker_Presentation.pptx) - presentation deck
- [Fusion 3D model view](https://gmail3188638.autodesk360.com/g/shares/SHd38bfQT1fb47330c991faf8d40cf933bd1) - external 3D model reference
