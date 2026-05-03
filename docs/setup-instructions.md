# GD-1 Integration Setup Instructions

This document provides step-by-step instructions for setting up the hardware and flashing firmware for the GD-1 Drone project.

## Hardware Requirements
1. **ESP32 Glove Controller**: 1x standard ESP32, 1x MPU6050 IMU, 1x NRF24L01 PA+LNA module.
2. **ESP32 Drone Receiver**: 1x standard ESP32 for the drone receiver.
3. **Sensors for Drone Receiver**:
   - 2x HC-SR04 Ultrasonic Distance Sensors
   - 4x VL53L1X Time-of-Flight sensors (Left, Right, Rear, Up)
   - 1x LD2410 mmWave Radar module
   - 1x NRF24L01 PA+LNA module

## Hardware Wiring

### Glove (Transmitter)
- **MPU6050 (I2C):** SDA to GPIO 21, SCL to GPIO 22.
- **NRF24L01 (SPI):** CE to GPIO 5, CSN to GPIO 15, SCK to GPIO 18, MOSI to GPIO 23, MISO to GPIO 19.

### Drone Receiver
- **NRF24L01 (SPI):** CE to GPIO 5, CSN to GPIO 15, SCK to GPIO 18, MOSI to GPIO 23, MISO to GPIO 19.
- **Front Ultrasonic (HC-SR04):** TRIG to GPIO 14, ECHO to GPIO 27. **Warning:** HC-SR04 outputs a 5V signal. You MUST use a 5V to 3.3V voltage divider on the ECHO pin before connecting to the ESP32.
- **Bottom Ultrasonic (HC-SR04):** TRIG to GPIO 25, ECHO to GPIO 26. **Warning:** HC-SR04 outputs a 5V signal. You MUST use a 5V to 3.3V voltage divider on the ECHO pin before connecting to the ESP32.
- **LD2410 Radar (UART):** RX to GPIO 16, TX to GPIO 4.
- **Flight Controller (SBUS):** TX to GPIO 17 (Hardware Serial 2). No external hardware inverter needed.
- **VL53L1X Array (I2C):** All sensors SDA to GPIO 21, SCL to GPIO 22. VCC to 3.3V, GND to GND.
  - XSHUT_LEFT to GPIO 13
  - XSHUT_RIGHT to GPIO 12
  - XSHUT_REAR to GPIO 32
  - XSHUT_UP to GPIO 33
  - *Note: All 4 sensors share one I2C bus. XSHUT pins are mandatory — do not leave them floating.*

## Software Installation
1. **Clone the Repository**:
   ```bash
   git clone <your-repo-url>
   cd gd1-drone
   ```

2. **Set up PlatformIO**:
   Ensure you have PlatformIO installed. If not, you can install it via a python virtual environment:
   ```bash
   python3 -m venv .venv
   source .venv/bin/activate
   pip install platformio
   ```

3. **Flash the Glove**:
   Connect the ESP32 for the glove.
   ```bash
   cd firmware/glove
   pio run -e glove_esp32 --target upload
   ```

4. **Flash the Drone Receiver**:
   Connect the ESP32 for the drone.
   ```bash
   cd firmware/drone_receiver
   pio run -e drone_receiver_esp32 --target upload
   ```

## End-to-End Test
Ensure both the glove and receiver are powered. The glove will transmit gestures over NRF24L01, and the receiver will output SBUS commands based on safety states evaluated from ultrasonic and radar sensors.
