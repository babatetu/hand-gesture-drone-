# GD-1 v2: Gesture-Controlled Hybrid-Sensing Drone
**Comprehensive Product Requirements Document (PRD)**

---

## 1. Executive Summary
**Project Name:** GD-1 (Gesture Drone v2)
**Description:** A custom-built, F450-class quadcopter controlled entirely by a wearable gesture glove. The system replaces traditional dual-joystick controllers with intuitive human hand motion. To ensure safety without pilot intervention, the drone features a two-tiered hybrid sensing system: primary physical reflexes using ultrasonic sensors, and secondary intelligent awareness using RuView (a camera-free WiFi CSI sensing system).

---

## 2. System Architecture Overview
The system is distributed across three primary physical environments communicating wirelessly.

### High-Level Flow
1. **Transmitter:** The pilot tilts the **Gesture Glove**.
2. **Link:** The glove calculates the command and sends it via **UDP (GD1Protocol)**.
3. **Receiver (Safety Hub):** The **Drone ESP32** receives the command.
4. **Safety Check 1:** The ESP32 checks the **Ultrasonic Sensor**. If an obstacle is < 100cm, forward motion is blocked.
5. **Safety Check 2:** The ESP32 checks the **RuView Server**. If a person is moving in front, forward motion is blocked.
6. **Execution:** If safe, the ESP32 scales the command based on the selected **Speed Mode** (Stable/Fast).
7. **Output:** The ESP32 converts the command to an 11-bit **SBUS** signal and sends it to the **Pixhawk Flight Controller**.
8. **Flight:** The Pixhawk spins the **Motors** to execute the movement while fighting wind pressure.

---

## 3. Hardware Specifications

### 3.1 The Gesture Glove (Transmitter)
*   **Microcontroller:** ESP32 Development Board.
*   **Sensor:** MPU6050 IMU (Gyroscope + Accelerometer) communicating via I2C (SDA: GPIO 21, SCL: GPIO 22).
*   **Input:** BOOT Button (GPIO 0) used to toggle Speed Modes.
*   **Power:** Small 1S (3.7V) LiPo battery with a 5V boost converter.

### 3.2 The Drone Receiver (Safety Hub)
*   **Microcontroller:** ESP32 Development Board.
*   **Primary Sensor:** HC-SR04 Ultrasonic Distance Sensor.
    *   `TRIG`: GPIO 14
    *   `ECHO`: GPIO 27 (Requires 5V to 3.3V Voltage Divider).
*   **Output:** Hardware Serial 2 TX (GPIO 17) configured for 100kbps inverted SBUS output.
*   **Power:** 5V BEC (Battery Eliminator Circuit) driven by the main drone battery.

### 3.3 The Drone Platform
*   **Frame:** F450 or Q450 Quadcopter frame.
*   **Motors:** 4x 2212 920KV or 1000KV Brushless Motors.
*   **ESCs:** 4x 30A Electronic Speed Controllers.
*   **Propellers:** 1045 (10x4.5) or 1047 (10x4.7) propellers (2x CW, 2x CCW).
*   **Flight Controller:** Pixhawk (running ArduPilot).
*   **Power:** 3S (11.1V) or 4S (14.8V) 3000mAh - 5200mAh LiPo Battery.

### 3.4 The RuView System (Secondary Awareness)
*   **Sensing Nodes:** 3x ESP32-S3 boards (placed around the room).
*   **Network:** Standard 2.4GHz WiFi Router.
*   **Processing Unit:** PC or Raspberry Pi running the Python CSI Aggregator Server.

---

## 4. Software & Firmware Specifications

### 4.1 Development Environment
*   **Framework:** Arduino Core for ESP32.
*   **Build System:** PlatformIO (via VS Code or CLI).
*   **Language:** C++17 (Firmware), Python 3 (RuView Server).

### 4.2 Module 1: Gesture Processing (`GestureProcessor.cpp`)
*   Reads raw accelerometer/gyroscope data.
*   Applies a Low-Pass Filter (`Alpha = 0.85`) to smooth hand tremors.
*   Applies a physical Deadband (`6.0 degrees`) requiring deliberate movement to exit the "Hover" state.
*   Calculates relative pitch and roll mapped to a normalized `-100 to 100` scale.

### 4.3 Module 2: Communication Link (`GD1Protocol.h`)
*   Custom binary packet (15 bytes).
*   **Data transmitted:** Sequence number, Timestamp, Pitch, Roll, Yaw, Throttle, and Flags.
*   **Flags:** Calibrated, Failsafe, Emergency Stop, and Fast Mode.
*   Transmitted via UDP over WiFi (future-proofed for LoRa/NRF24).

### 4.4 Module 3: Dual-Tier Safety System
*   **Tier 1 (Ultrasonic):** Non-blocking `micros()` driver. Pings every 50ms. Triggers an absolute hard-block of forward pitch if distance < 100cm.
*   **Tier 2 (RuView CSI):** Python server analyzes Channel State Information (CSI). Uses "Energy Delta" to estimate position and "Motion Score" to detect presence. Sends JSON over HTTP POST. Only evaluated if Tier 1 is clear.
*   **Strict Hierarchy:** Ultrasonic > RuView > Gesture. RuView *cannot* override an ultrasonic block.

### 4.5 Module 4: Flight Output (`SBUSGenerator.cpp`)
*   Converts normalized `-100 to 100` commands to standard `1000 to 2000` PWM values.
*   Maps values to standard AETR channels (Ch1: Roll, Ch2: Pitch, Ch3: Throttle, Ch4: Yaw).
*   Encodes the 16 channels into an 11-bit, 25-byte SBUS frame.
*   Broadcasts at 100kbps using native ESP32 Hardware UART inversion (eliminating the need for external hardware inverters).

### 4.6 Module 5: Flight Dynamics & Features
*   **Speed Modes:**
    *   *Stable Mode:* Default. Scales pitch/roll/yaw inputs by 50%. Limits maximum drone tilt for gentle, safe flying.
    *   *Fast Mode:* Toggled via glove button. Allows 100% command pass-through for aggressive flight and fighting high winds.
*   **Failsafe:** If the drone receiver does not receive a `GD1Protocol` packet for > 500ms, all movement commands (Pitch, Roll, Yaw, Throttle) default to 0 (neutral/hover).
*   **Wind Resistance:** The ESP32 handles command translation and safety arbitration; the Pixhawk + ArduPilot PID loop handles all stabilization, attitude hold, and wind resistance autonomously.

---

## 5. Non-Functional Requirements & Performance Metrics
*   **Latency Target (Control Loop):** < 100ms.
*   **Latency Target (RuView):** < 200ms.
*   **Loop Frequency:** 50 Hz to 100 Hz (ensured via completely non-blocking firmware architecture; absolutely no `delay()` calls used in operational loops).
*   **Flight Time:** 8 to 15 minutes (depending on battery capacity and wind conditions).
*   **Flight Range:** 30 to 100 meters (Limited by standard 2.4GHz WiFi). Upgradeable to 1km+ by swapping UDP link for NRF24L01/LoRa modules.
*   **Weight Overhead:** Logic payload (ESP32 + Sonar) < 30 grams.

---

## 6. Setup & Compilation Guide
The repository contains fully compiled, ready-to-flash environments via PlatformIO:
1.  **Glove:** `pio run -e glove_esp32 --target upload`
2.  **Drone Receiver:** `pio run -e drone_receiver_esp32 --target upload`
3.  **CSI Nodes:** `pio run -e csi_node_esp32s3 --target upload`
4.  **RuView Server:** `python -m server.ruview_csi_bridge.server`
*See `docs/setup-instructions.md` for wiring diagrams and software installation guides.*
