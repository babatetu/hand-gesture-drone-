# GD-1 Gesture Drone

GD-1 is a gesture-controlled obstacle-avoiding quadcopter project. This repository includes ESP32 glove firmware that reads an MPU6050 IMU and sends commands over NRF24L01 to the drone receiver, which parses those commands and merges them with a 6-direction obstacle detection system (Ultrasonic + VL53L1X + mmWave Radar). The receiver converts these to an SBUS signal for the Pixhawk.

**Range Spec:** 500m–1km (NRF24 PA+LNA)

## Hardware Needed
- 2x ESP32 Development Boards
- 1x MPU6050 IMU
- 2x NRF24L01 PA+LNA modules
- 2x HC-SR04 Ultrasonic Distance Sensors
- 1x LD2410 mmWave Radar
- 4x VL53L1X Time-of-Flight sensors (Left / Right / Rear / Up)

**XSHUT Wiring Note:** GPIO 13/12/32/33 → XSHUT pins for I2C address assignment at boot

## Current Build Scope

Phase 1 is bench-only and should be tested with no drone motors or propellers connected.

- Glove ESP32 firmware
- MPU6050 I2C reading
- Startup neutral calibration
- Pitch and roll calculation
- Low-pass filtering and deadband
- Gesture mapping:
  - Forward
  - Backward
  - Left
  - Right
  - Hover
- CSV serial debug output

## Project Layout

```text
firmware/
  lib/
    GD1Protocol/
      src/
        GD1Protocol.h
        GD1Protocol.cpp
  glove/
    platformio.ini
    include/
      GD1Config.h
      GestureProcessor.h
    src/
      GestureProcessor.cpp
      main.cpp
    test/
      test_gesture_processor/
        test_gesture_processor.cpp
  drone_receiver/
    include/
      DroneReceiverConfig.h
      UltrasonicSensor.h
      SBUSGenerator.h
      LD2410Driver.h
      VL53L1XArray.h
    src/
      UltrasonicSensor.cpp
      SBUSGenerator.cpp
      LD2410Driver.cpp
      VL53L1XArray.cpp
      main.cpp
docs/
  phase-plan.md
  wiring-fr1.md
  setup-instructions.md
```

## Build

Install PlatformIO first, then run:

```powershell
cd firmware/glove
pio run
```

This workspace also has PlatformIO installed locally in `.venv`:

```powershell
cd firmware/glove
..\..\.venv\Scripts\platformio.exe run -e glove_esp32
```

Upload to an ESP32:

```powershell
pio run --target upload
```

Open serial monitor:

```powershell
pio device monitor
```

Run pure gesture logic tests:

```powershell
pio test -e native
```

Note: native tests require a local `gcc/g++` toolchain on Windows. The ESP32 firmware build does not require a separately installed native compiler because PlatformIO downloads the ESP32 cross-compiler.

## Serial Output

The glove prints CSV rows:

```text
ms,seq,pitch_deg,roll_deg,pitch_cmd,roll_cmd,packet_ok,gesture
1234,42,8.42,-1.20,28,0,1,FORWARD
```

Command conventions:

- `pitch_cmd` positive means forward.
- `pitch_cmd` negative means backward.
- `roll_cmd` positive means right.
- `roll_cmd` negative means left.
- Values range from `-100` to `100`.

If your physical glove orientation is reversed, change `GD1_PITCH_COMMAND_SIGN` or `GD1_ROLL_COMMAND_SIGN` in `firmware/glove/include/GD1Config.h`.
