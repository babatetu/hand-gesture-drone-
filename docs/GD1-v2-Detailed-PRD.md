# GD-1 v2: Gesture-Controlled Hybrid-Sensing Drone
**Comprehensive Product Requirements Document (PRD)**

---

## 1. Executive Summary
**Project Name:** GD-1 (Gesture Drone v2)
**Description:** A custom-built, F450-class quadcopter controlled entirely by a wearable gesture glove. The system replaces traditional dual-joystick controllers with intuitive human hand motion. To ensure safety without pilot intervention, the drone features a hybrid sensing system: primary physical reflexes using ultrasonic sensors and secondary intelligence using an LD2410 mmWave radar and an array of 4 VL53L1X Time-of-Flight sensors.

---

## 2. System Architecture Overview
The system consists of the glove transmitter and the drone-mounted receiver interacting wirelessly via NRF24L01 radios.

### High-Level Flow
1. **Transmitter:** The pilot tilts the **Gesture Glove**.
2. **Link:** The glove calculates the command and sends it via **NRF24L01 SPI (GD1Protocol)**.
3. **Receiver (Safety Hub):** The **Drone ESP32** receives the command.
4. **Safety Check 1:** The ESP32 checks the **Ultrasonic Sensors**. If an obstacle is < 100cm, forward motion is blocked.
5. **Safety Check 2:** The ESP32 checks the **LD2410 Radar** and **VL53L1X ToF array**. Motion is blocked or scaled based on proximity in 6 directions.
6. **Execution:** If safe, the ESP32 scales the command based on the selected **Speed Mode** (Stable/Fast).
7. **Output:** The ESP32 converts the command to an 11-bit **SBUS** signal and sends it to the **Pixhawk Flight Controller**.
8. **Flight:** The Pixhawk spins the **Motors** to execute the movement while fighting wind pressure.

---

## 3. Hardware Specifications

### 3.1 The Gesture Glove (Transmitter)
*   **Microcontroller:** ESP32 Development Board.
*   **Sensor:** MPU6050 IMU (Gyroscope + Accelerometer) communicating via I2C (SDA: GPIO 21, SCL: GPIO 22).
*   **Radio:** NRF24L01 PA+LNA (SPI: CE=5, CSN=15, SCK=18, MISO=19, MOSI=23).
*   **Input:** BOOT Button (GPIO 0) used to toggle Speed Modes.
*   **Power:** Small 1S (3.7V) LiPo battery with a 5V boost converter.

### 3.2 The Drone Receiver (Safety Hub)
*   **Microcontroller:** ESP32 Development Board.
*   **Ultrasonic Sensors (HC-SR04):**
    *   Front: `TRIG` 14, `ECHO` 27 (Requires Voltage Divider).
    *   Bottom: `TRIG` 25, `ECHO` 26 (Requires Voltage Divider).
*   **Radar (LD2410):** `RX` 16, `TX` 4 (Hardware Serial 1 @ 256000 baud).
*   **ToF Array (4x VL53L1X):** I2C (SDA: 21, SCL: 22). Multiplexed via `XSHUT` pins: Left 13, Right 12, Rear 32, Up 33.
*   **Radio:** NRF24L01 PA+LNA (SPI: CE=5, CSN=15, SCK=18, MISO=19, MOSI=23).
*   **Output:** Hardware Serial 2 TX (GPIO 17) configured for 100kbps inverted SBUS output.
*   **Power:** 5V BEC (Battery Eliminator Circuit) driven by the main drone battery.

### 3.3 The Drone Platform
*   **Frame:** F450 or Q450 Quadcopter frame.
*   **Motors:** 4x 2212 920KV or 1000KV Brushless Motors.
*   **ESCs:** 4x 30A Electronic Speed Controllers.
*   **Propellers:** 1045 (10x4.5) or 1047 (10x4.7) propellers (2x CW, 2x CCW).
*   **Flight Controller:** Pixhawk (running ArduPilot).
*   **Power:** 3S (11.1V) or 4S (14.8V) 3000mAh - 5200mAh LiPo Battery.

---

## 4. Software & Firmware Specifications

### 4.1 Development Environment
*   **Framework:** Arduino Core for ESP32.
*   **Build System:** PlatformIO (via VS Code or CLI).
*   **Language:** C++17 (Firmware).

### 4.2 Module 1: Gesture Processing (`GestureProcessor.cpp`)
*   Reads raw accelerometer/gyroscope data.
*   Applies a Low-Pass Filter (`Alpha = 0.65`) to smooth hand tremors (33ms time constant).
*   Applies a physical Deadband (`6.0 degrees` tilt, `5.0 dps` yaw) requiring deliberate movement to exit the "Hover" state.
*   Calculates relative pitch, roll, and yaw-rate mapped to a normalized `-100 to 100` scale.

### 4.3 Module 2: Communication Link (`GD1Protocol.h`)
*   Custom binary packet (15 bytes).
*   **Data transmitted:** Sequence number, Timestamp, Pitch, Roll, Yaw, Throttle, and Flags.
*   **Flags:** Calibrated, Failsafe, Emergency Stop, and Fast Mode.
*   Transmitted via NRF24L01 PA+LNA radio link at 250kbps.

### 4.4 Module 3: Multi-Tier Safety System
*   **Tier 1 (Ultrasonic):** Non-blocking `micros()` driver. Front triggers absolute hard-block of forward pitch if < 100cm. Bottom provides soft landing guard, throttling descent if < 30cm.
*   **Tier 2 (Radar & ToF):** LD2410 checks for human presence (blocks forward). VL53L1X array checks 4 directions (Left, Right, Rear, Up).
    *   Hard Block (< 30cm side/rear, < 40cm up): Halts command entirely in that vector.
    *   Soft Warn (< 80cm side/rear/up): Scales incoming gesture vector down by 50%.
*   **Strict Hierarchy:** Ultrasonic > Radar/ToF > Gesture.

### 4.5 Module 4: Flight Output (`SBUSGenerator.cpp`)
*   Converts normalized `-100 to 100` commands directly to the standard 11-bit SBUS operational range (`172 to 1811`, center `992`).
*   Maps values to standard AETR channels (Ch1: Roll, Ch2: Pitch, Ch3: Throttle, Ch4: Yaw).
*   Encodes the 16 channels into a 25-byte SBUS frame.
*   Broadcasts at 100kbps using native ESP32 Hardware UART inversion (eliminating the need for external hardware inverters).

### 4.6 Module 5: Flight Dynamics & Features
*   **Speed Modes:**
    *   *Stable Mode:* Default. Scales pitch/roll/yaw inputs by 50%. Limits maximum drone tilt for gentle, safe flying.
    *   *Fast Mode:* Toggled via glove button. Allows 100% command pass-through for aggressive flight and fighting high winds.
*   **Failsafe:** If the drone receiver does not receive a `GD1Protocol` packet for > 500ms, Pitch, Roll, and Yaw zero out, but Throttle is maintained at its last known safe value to prevent drop-outs.
*   **Wind Resistance:** The ESP32 handles command translation and safety arbitration; the Pixhawk + ArduPilot PID loop handles all stabilization, attitude hold, and wind resistance autonomously.

---

## 5. Non-Functional Requirements & Performance Metrics
*   **Latency Target (Control Loop):** < 100ms.
*   **Loop Frequency:** 50 Hz to 100 Hz (ensured via completely non-blocking firmware architecture; absolutely no `delay()` calls used in operational loops).
*   **Flight Time:** 8 to 15 minutes (depending on battery capacity and wind conditions).
*   **Flight Range:** 500m to 1km (Powered by NRF24L01 PA+LNA).
*   **Weight Overhead:** Logic payload (ESP32 + Sensors) < 50 grams.

---

## 6. Setup & Compilation Guide
The repository contains fully compiled, ready-to-flash environments via PlatformIO:
1.  **Glove:** `pio run -e glove_esp32 --target upload`
2.  **Drone Receiver:** `pio run -e drone_receiver_esp32 --target upload`

*See `docs/setup-instructions.md` for wiring diagrams and software installation guides.*
